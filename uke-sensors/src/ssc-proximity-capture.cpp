// SPDX-License-Identifier: MIT
// Bounded proximity transport comparison; raw output belongs in private records.
#define UKE_SSC_TEST
#include "ssc-discovery.cpp"
#include <fcntl.h>
#include <optional>

enum class Mode { metadata, passive, active_small, active_large, active_empty };
static Mode mode(std::string_view name) {
    if (name == "passive") return Mode::passive;
    if (name == "active-small") return Mode::active_small;
    if (name == "active-large") return Mode::active_large;
    if (name == "active-empty") return Mode::active_empty;
    throw std::runtime_error("Expected passive, active-small, active-large or active-empty");
}
static bool large_report(Mode selected) {
    return selected == Mode::active_large || selected == Mode::active_empty;
}
static Bytes lab_request(Uid sensor, unsigned message, View payload, Mode selected = Mode::metadata) {
    require((selected == Mode::metadata && (message == 512 || message == 1)) ||
            (selected != Mode::metadata && message == 514 && payload.empty()), "Unsupported capture request");
    const bool passive = selected == Mode::passive;
    Bytes standard;
    if (selected == Mode::metadata || passive || selected == Mode::active_empty)
        length_field(standard, 2, payload);
    if (passive) standard.insert(standard.end(), {0x18, 1});
    Bytes uid{0x09}; put(uid, sensor.low, 8);
    uid.push_back(0x11); put(uid, sensor.high, 8);
    Bytes client; length_field(client, 1, uid);
    client.push_back(0x15); put(client, message, 4);
    const Bytes suspend{0x08, 1, 0x10, std::uint8_t(passive ? 1 : 0)};
    length_field(client, 3, suspend);
    length_field(client, 4, standard);
    Bytes body{1}; put(body, client.size() + 2, 2); put(body, client.size(), 2);
    body.insert(body.end(), client.begin(), client.end());
    body.insert(body.end(), {0x10,1,0,std::uint8_t(large_report(selected) ? 1 : 0)});
    Bytes packet{0}; put(packet, 1, 2); put(packet, 0x20, 2); put(packet, body.size(), 2);
    packet.insert(packet.end(), body.begin(), body.end());
    return packet;
}
static Bytes default_lookup() {
    const std::string_view type = "proximity";
    Bytes payload;
    length_field(payload, 1, View(reinterpret_cast<const std::uint8_t *>(type.data()), type.size()));
    payload.insert(payload.end(), {0x10, 0, 0x18, 1}); // No updates; only defaults.
    return lab_request(lookup_uid, 512, payload);
}
static void gate_metadata(View event, Uid selected) {
    auto root = fields(event), uid = fields(one(root, 1, 2));
    require(Uid{little(one(uid, 1, 1)), little(one(uid, 2, 1))} == selected, "Foreign metadata SUID");
    bool type_ok = false, available = false, on_change = false, physical = false;
    bool type_seen = false, available_seen = false, stream_seen = false, physical_seen = false;
    for (auto &entry : root) if (entry.number == 2) {
        require(entry.wire == 2, "Invalid metadata event");
        auto e = fields(entry.data);
        if (little(one(e, 1, 5)) != 128) continue;
        for (auto &attr : fields(one(e, 3, 2))) if (attr.number == 1) {
            require(attr.wire == 2, "Invalid attribute");
            auto a = fields(attr.data); auto raw_id = one(a, 1, 0);
            std::size_t offset = 0; auto id = varint(raw_id, offset);
            if (id != 2 && id != 3 && id != 16 && id != 21) continue;
            auto value = fields(one(fields(one(a, 2, 2)), 1, 2));
            if (id == 2) {
                require(!type_seen, "Duplicate type"); type_seen = true;
                auto text = one(value, 2, 2);
                if (!text.empty() && text.back() == 0) text = text.first(text.size() - 1);
                type_ok = std::string_view(reinterpret_cast<const char *>(text.data()), text.size()) == "proximity";
            } else if (id == 3) {
                require(!available_seen, "Duplicate availability"); available_seen = true;
                auto raw = one(value, 5, 0); offset = 0;
                available = varint(raw, offset) == 1;
            } else if (id == 16) {
                require(!stream_seen, "Duplicate stream type"); stream_seen = true;
                on_change = little(one(value, 4, 1)) == 1;
            } else {
                require(!physical_seen, "Duplicate physical flag"); physical_seen = true;
                auto raw = one(value, 5, 0); offset = 0;
                physical = varint(raw, offset) == 1;
            }
        }
    }
    require(type_ok && available && on_change && physical, "Default proximity must be physical, available and on-change");
}
static void record_events(View event, Uid selected, unsigned &count, std::ostream &out) {
    auto root = fields(event), uid = fields(one(root, 1, 2));
    if (!(Uid{little(one(uid, 1, 1)), little(one(uid, 2, 1))} == selected)) return;
    for (auto &entry : root) if (entry.number == 2 && count < 32) {
        require(entry.wire == 2, "Invalid client event");
        auto e = fields(entry.data);
        auto id = little(one(e, 1, 5)); one(e, 2, 1);
        auto payload = one(e, 3, 2);
        out << "event_id=" << std::dec << id << " payload_bytes=" << payload.size() << " tags=";
        try {
            for (auto &f : fields(payload)) out << f.number << ':' << f.wire << ',';
        } catch (const std::exception &) { out << "invalid"; }
        out << " payload_hex=" << std::hex << std::setfill('0');
        for (auto byte : payload) out << std::setw(2) << unsigned(byte);
        out << std::dec << '\n';
        ++count;
    }
}
static std::optional<Bytes> receive_capture(int fd, sockaddr_qrtr &peer, std::chrono::steady_clock::time_point deadline) {
    for (;;) {
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0) return std::nullopt;
        pollfd watch{fd, POLLIN, 0};
        int ready = poll(&watch, 1, remaining);
        if (ready < 0 && errno == EINTR) continue;
        if (ready == 0) return std::nullopt;
        require(ready > 0 && !(watch.revents & (POLLERR | POLLHUP | POLLNVAL)), "QRTR capture transport failed");
        Bytes bytes(limit);
        iovec iov{bytes.data(), bytes.size()};
        msghdr msg{}; msg.msg_name = &peer; msg.msg_namelen = sizeof(peer);
        msg.msg_iov = &iov; msg.msg_iovlen = 1;
        auto length = recvmsg(fd, &msg, MSG_DONTWAIT);
        if (length < 0 && (errno == EAGAIN || errno == EINTR)) continue;
        require(length > 0 && !(msg.msg_flags & MSG_TRUNC) && msg.msg_namelen == sizeof(peer), "Invalid QRTR capture datagram");
        bytes.resize(length); return bytes;
    }
}
static void capture(int fd, Uid selected, Mode selected_mode, std::ostream &out) {
    sockaddr_qrtr local{}; socklen_t size = sizeof(local);
    require(getsockname(fd, reinterpret_cast<sockaddr *>(&local), &size) == 0 && size == sizeof(local), "QRTR local address unavailable");
    local.sq_port = 0;
    require(bind(fd, reinterpret_cast<sockaddr *>(&local), sizeof(local)) == 0, "QRTR bind failed");
    require(getsockname(fd, reinterpret_cast<sockaddr *>(&local), &size) == 0 && size == sizeof(local), "QRTR bound address unavailable");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    sockaddr_qrtr control{AF_QIPCRTR, local.sq_node, QRTR_PORT_CTRL}, server{}, peer{};
    Bytes lookup; put(lookup, QRTR_TYPE_NEW_LOOKUP, 4); put(lookup, 400, 4);
    put(lookup, 1, 4); put(lookup, 0, 4); put(lookup, 0, 4);
    send(fd, control, lookup);
    bool found = false;
    unsigned packets = 0;
    while (packets++ < 64) {
        auto bytes = receive_capture(fd, peer, deadline); if (!bytes) break;
        View data = *bytes;
        if (peer.sq_port != QRTR_PORT_CTRL || peer.sq_node != local.sq_node) continue;
        require(data.size() == sizeof(qrtr_ctrl_pkt), "Invalid QRTR lookup reply");
        if (little(data.first(4)) != QRTR_TYPE_NEW_SERVER) continue;
        if (little(data.subspan(4, 4)) != 400 || little(data.subspan(8, 4)) != 1) continue;
        server = {AF_QIPCRTR, std::uint32_t(little(data.subspan(12, 4))), std::uint32_t(little(data.subspan(16, 4)))};
        require(server.sq_node && server.sq_port && server.sq_port != QRTR_PORT_CTRL, "Invalid SSC service");
        found = true; break;
    }
    require(found, "SSC service lookup failed");
    send(fd, server, lab_request(selected, 514, {}, selected_mode));
    bool accepted = false; std::uint64_t client = 0;
    unsigned count = 0;
    while (packets++ < 64 && count < 32) {
        auto bytes = receive_capture(fd, peer, deadline); if (!bytes) break;
        View data = *bytes;
        if (peer.sq_node != server.sq_node || peer.sq_port != server.sq_port) continue;
        auto items = tlvs(data); auto message = little(data.subspan(3, 2));
        if (data[0] == 2 && message == 0x20) {
            require(little(data.subspan(1, 2)) == 1 && !accepted, "Wrong QMI response transaction");
            auto result = tlv(items, 2);
            require(result.size() == 4 && little(result.first(2)) == 0 && little(result.last(2)) == 0, "SSC rejected capture");
            auto id = tlv(items, 0x10); require(id.size() == 8, "Invalid SSC client identity"); client = little(id);
            for (auto &t : items) if (t.type == 0x11) require(t.data.size() == 4 && little(t.data) == 0, "SSC protobuf request rejected");
            accepted = true; out << "capture_acknowledged\n";
        } else if (data[0] == 4 && message == (large_report(selected_mode) ? 0x22 : 0x21)) {
            require(accepted, "SSC event precedes acknowledged request");
            auto id = tlv(items, 1); require(id.size() == 8 && little(id) == client, "Foreign SSC client event");
            auto payload = tlv(items, 2);
            require(payload.size() >= 2 && little(payload.first(2)) == payload.size() - 2, "SSC payload length mismatch");
            record_events(payload.subspan(2), selected, count, out);
        } else throw std::runtime_error("Unexpected SSC capture message");
    }
    require(accepted, "SSC did not acknowledge passive capture");
    out << "captured_events=" << count << " limit_events=32 limit_seconds=10 passive="
        << (selected_mode == Mode::passive ? "true" : "false") << '\n';
}
#ifndef UKE_PROXIMITY_CAPTURE_TEST
int main(int argc, char **argv) {
    (void)&request; (void)&attributes;
    int fd = -1;
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--help") {
            std::cout << "Usage: uke-proximity-capture [passive|active-small|active-large|active-empty]\n"
                "Root-only Uke diagnostic: ten-second raw capture, no calibration or rate changes.\n"
                "Default is passive/non-wakeup; active modes enable the default proximity sensor temporarily.\n";
            return 0;
        }
        require(argc == 1 || argc == 2, "Usage: uke-proximity-capture [passive|active-small|active-large|active-empty]");
        const auto selected_mode = mode(argc == 1 ? "passive" : argv[1]);
        require(geteuid() == 0, "Root is required for private proximity captures");
        int board = open("/sys/firmware/devicetree/base/compatible", O_RDONLY | O_CLOEXEC);
        require(board >= 0, "Uke device tree required");
        std::array<char,4096> compatible{};
        auto length = ::read(board, compatible.data(), compatible.size()); close(board);
        require(length > 0 && std::size_t(length) < compatible.size(), "Invalid compatible data");
        const std::string identity(compatible.data(), std::size_t(length));
        require(identity.find(std::string("xiaomi,uke\0",11)) != std::string::npos &&
                identity.find(std::string("qcom,sm7675\0",12)) != std::string::npos, "Uke / SM7675 required");
        fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        require(fd >= 0, "QRTR lookup socket unavailable");
        auto found = suids(rpc(fd, default_lookup()), "proximity"); close(fd); fd = -1;
        std::cout << "default_proximity_count=" << found.size() << '\n';
        require(found.size() == 1, "Ambiguous default proximity; refusing subscription");
        fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        require(fd >= 0, "QRTR metadata socket unavailable");
        auto metadata = rpc(fd, lab_request(found[0], 1, {})); close(fd); fd = -1;
        gate_metadata(metadata, found[0]);
        fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        require(fd >= 0, "QRTR capture socket unavailable");
        capture(fd, found[0], selected_mode, std::cout); close(fd); fd = -1;
        return 0;
    } catch (const std::exception &error) {
        if (fd >= 0) close(fd);
        std::cerr << error.what() << '\n'; return 1;
    }
}
#endif
