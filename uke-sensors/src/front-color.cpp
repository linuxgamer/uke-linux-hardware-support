// SPDX-License-Identifier: MIT
// Explicitly activated raw CCT publisher; independent of the original SensorProxy.
#define UKE_SSC_SAMPLE_TEST
#include "ssc-sample.cpp"
#include "front-color-file.hpp"
#include "front-color-state.hpp"
#include <csignal>
#include <ctime>
#include <optional>
#include <sys/file.h>

static volatile sig_atomic_t stopped = 0;
static void stop_handler(int) { stopped = 1; }
static std::uint64_t boottime_ns() {
    timespec now{};
    require(clock_gettime(CLOCK_BOOTTIME, &now) == 0 && now.tv_sec >= 0, "BOOTTIME clock unavailable");
    return std::uint64_t(now.tv_sec) * 1'000'000'000ULL + std::uint64_t(now.tv_nsec);
}
struct Fd {
    int value = -1;
    Fd() = default;
    explicit Fd(int n) : value(n) {}
    Fd(const Fd &) = delete;
    Fd &operator=(const Fd &) = delete;
    ~Fd() { if (value >= 0) close(value); }
};
static int state_directory() {
    require(geteuid() == 0, "Front-color service requires its root-owned runtime directory");
    int fd = open("/run/uke-sensors", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    require(fd >= 0, "Front-color runtime directory unavailable");
    struct stat st{};
    if (fstat(fd, &st) != 0 || !uke::color::public_directory(st)) {
        close(fd);
        throw std::runtime_error("Front-color runtime directory must be root-owned and mode 0755");
    }
    return fd;
}
static void check_board() {
    Fd compatible(open("/sys/firmware/devicetree/base/compatible", O_RDONLY | O_CLOEXEC));
    require(compatible.value >= 0, "Uke device tree unavailable");
    std::array<char,4096> bytes{};
    auto n = ::read(compatible.value, bytes.data(), bytes.size());
    require(n > 0 && std::size_t(n) < bytes.size(), "Invalid Uke compatible data");
    const std::string value(bytes.data(), std::size_t(n));
    require(value.find(std::string("xiaomi,uke\0", 11)) != std::string::npos &&
            value.find(std::string("qcom,sm7675\0", 12)) != std::string::npos, "Front-color service requires Uke / SM7675");
}
static void gate_metadata(View event, Uid selected) {
    auto root = fields(event), uid = fields(one(root, 1, 2));
    require(Uid{little(one(uid, 1, 1)), little(one(uid, 2, 1))} == selected, "Foreign front-color metadata");
    bool type_ok = false, available = false, physical = false, on_change = false;
    std::array<bool,4> seen{};
    for (auto &entry : root) if (entry.number == 2) {
        require(entry.wire == 2, "Invalid front-color metadata event");
        auto e = fields(entry.data);
        if (little(one(e, 1, 5)) != 128) continue;
        for (auto &attribute : fields(one(e, 3, 2))) if (attribute.number == 1) {
            require(attribute.wire == 2, "Invalid front-color attribute");
            auto a = fields(attribute.data); std::size_t p = 0;
            auto id = varint(one(a, 1, 0), p);
            if (id != 2 && id != 3 && id != 16 && id != 21) continue;
            unsigned slot = id == 2 ? 0 : id == 3 ? 1 : id == 16 ? 2 : 3;
            require(!seen[slot], "Duplicate front-color attribute"); seen[slot] = true;
            auto value = fields(one(fields(one(a, 2, 2)), 1, 2));
            if (id == 2) {
                auto text = one(value, 2, 2);
                if (!text.empty() && text.back() == 0) text = text.first(text.size() - 1);
                type_ok = std::string_view(reinterpret_cast<const char *>(text.data()), text.size()) == "cct_front";
            } else if (id == 16) on_change = little(one(value, 4, 1)) == 1;
            else {
                p = 0; const auto flag = varint(one(value, 5, 0), p);
                require(flag <= 1, "Invalid front-color availability");
                if (id == 3) available = flag == 1; else physical = flag == 1;
            }
        }
    }
    require(type_ok && available && physical && on_change, "Front-color endpoint metadata unavailable");
}
static Bytes color_client_request(Uid sensor, unsigned message, View payload) {
    auto packet = client_request(sensor, message, payload);
    if (message == 512 || message == 1) {
        // Match OEM discovery/attribute delivery; measurement requests stay non-wakeup.
        const auto client = fields(tlv(tlvs(packet), 1).subspan(2));
        const auto delivery = one(fields(one(client, 3, 2)), 2, 0);
        require(delivery.size() == 1 && delivery[0] == 1, "Unexpected metadata delivery encoding");
        packet.at(delivery.data() - packet.data()) = 0;
    }
    return packet;
}
struct Connection {
    Fd fd{socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0)};
    sockaddr_qrtr server{};
    std::uint64_t client = 0;
    std::uint64_t rate_start_ns = boottime_ns();
    std::uint32_t local_node = 0;
    unsigned packet_count = 0;
    explicit Connection(Uid selected, unsigned message_id = 514, View payload = {}) {
        require(fd.value >= 0, "QRTR front-color socket unavailable");
        sockaddr_qrtr local{}, peer{}; socklen_t size = sizeof(local);
        require(getsockname(fd.value, reinterpret_cast<sockaddr *>(&local), &size) == 0 && size == sizeof(local), "QRTR local address unavailable");
        local.sq_port = 0;
        require(bind(fd.value, reinterpret_cast<sockaddr *>(&local), sizeof(local)) == 0, "QRTR bind failed");
        require(getsockname(fd.value, reinterpret_cast<sockaddr *>(&local), &size) == 0 && size == sizeof(local), "QRTR bound address unavailable");
        local_node = local.sq_node;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        const sockaddr_qrtr control{AF_QIPCRTR, local.sq_node, QRTR_PORT_CTRL};
        Bytes lookup; put(lookup, QRTR_TYPE_NEW_LOOKUP, 4); put(lookup, 400, 4);
        put(lookup, 1, 4); put(lookup, 0, 4); put(lookup, 0, 4);
        send(fd.value, control, lookup);
        bool found = false;
        unsigned packets = 0;
        while (!stopped && packets++ < 64) {
            auto data = receive(fd.value, peer, deadline);
            if (peer.sq_port != QRTR_PORT_CTRL || peer.sq_node != local.sq_node) continue;
            require(data.size() == sizeof(qrtr_ctrl_pkt), "Invalid QRTR lookup reply");
            View bytes = data;
            if (little(bytes.first(4)) != QRTR_TYPE_NEW_SERVER) continue;
            if (little(bytes.subspan(4, 4)) != 400 || little(bytes.subspan(8, 4)) != 1) continue;
            server = {AF_QIPCRTR, std::uint32_t(little(bytes.subspan(12, 4))), std::uint32_t(little(bytes.subspan(16, 4)))};
            require(server.sq_node && server.sq_port && server.sq_port != QRTR_PORT_CTRL, "Invalid SSC server");
            found = true; break;
        }
        require(found && !stopped, "SSC service lookup unavailable");
        send(fd.value, server, color_client_request(selected, message_id, payload));
        while (!stopped && packets++ < 64) {
            auto data = receive(fd.value, peer, deadline);
            if (peer.sq_node != server.sq_node || peer.sq_port != server.sq_port) continue;
            auto items = tlvs(data); View bytes = data;
            require(data[0] == 2 && little(bytes.subspan(3, 2)) == 0x20 && little(bytes.subspan(1, 2)) == 1, "Unexpected SSC acknowledgement");
            auto result = tlv(items, 2);
            require(result.size() == 4 && little(result.first(2)) == 0 && little(result.last(2)) == 0, "SSC rejected front-color request");
            auto id = tlv(items, 0x10); require(id.size() == 8, "Invalid SSC client identity"); client = little(id);
            for (auto &item : items) if (item.type == 0x11) require(item.data.size() == 4 && little(item.data) == 0, "SSC protobuf request rejected");
            return;
        }
        throw std::runtime_error("SSC front-color acknowledgement unavailable");
    }
    std::optional<Bytes> next() {
        pollfd watch{fd.value, POLLIN, 0};
        int ready = poll(&watch, 1, 1000);
        if ((ready < 0 && errno == EINTR) || ready == 0) return std::nullopt;
        require(ready > 0 && !(watch.revents & (POLLERR | POLLHUP | POLLNVAL)), "SSC front-color transport failed");
        Bytes data(limit); sockaddr_qrtr peer{};
        iovec iov{data.data(), data.size()};
        msghdr message{}; message.msg_name = &peer; message.msg_namelen = sizeof(peer);
        message.msg_iov = &iov; message.msg_iovlen = 1;
        auto length = recvmsg(fd.value, &message, MSG_DONTWAIT);
        if (length < 0 && (errno == EAGAIN || errno == EINTR)) return std::nullopt;
        require(length > 0 && !(message.msg_flags & MSG_TRUNC) && message.msg_namelen == sizeof(peer), "Invalid front-color datagram");
        data.resize(length);
        const auto now = boottime_ns();
        if (now - rate_start_ns >= 1'000'000'000ULL) { rate_start_ns = now; packet_count = 0; }
        require(++packet_count <= 64, "Front-color packet rate exceeded");
        if (peer.sq_node == local_node && peer.sq_port == QRTR_PORT_CTRL) {
            require(data.size() == sizeof(qrtr_ctrl_pkt), "Invalid QRTR lifecycle event");
            View control = data; const auto command = little(control.first(4));
            if ((command == QRTR_TYPE_NEW_SERVER || command == QRTR_TYPE_DEL_SERVER) &&
                little(control.subspan(4,4)) == 400 && little(control.subspan(8,4)) == 1) {
                const bool same = little(control.subspan(12,4)) == server.sq_node &&
                                  little(control.subspan(16,4)) == server.sq_port;
                require(command != QRTR_TYPE_DEL_SERVER || !same, "SSC server removed");
                require(command != QRTR_TYPE_NEW_SERVER || same, "SSC server replaced");
            }
            return std::nullopt;
        }
        require(peer.sq_node == server.sq_node && peer.sq_port == server.sq_port, "Foreign SSC server event");
        auto items = tlvs(data); View bytes = data;
        require(data[0] == 4 && little(bytes.subspan(3, 2)) == 0x21, "Unexpected front-color QMI message");
        auto id = tlv(items, 1); require(id.size() == 8 && little(id) == client, "Foreign SSC client event");
        auto payload = tlv(items, 2);
        require(payload.size() >= 2 && little(payload.first(2)) == payload.size() - 2, "SSC event length mismatch");
        return Bytes(payload.begin() + 2, payload.end());
    }
};
static Bytes color_lookup_payload() {
    constexpr std::string_view type = "cct_front";
    Bytes payload;
    length_field(payload, 1, View(reinterpret_cast<const std::uint8_t *>(type.data()), type.size()));
    // sns_suid.proto: register_updates=true, default_only=false.
    payload.insert(payload.end(), {0x10, 1, 0x18, 0});
    return payload;
}
static std::optional<Uid> color_endpoint(View event) {
    auto found = suids(event, "cct_front");
    require(found.size() <= 1, "Ambiguous front-color endpoint");
    return found.empty() ? std::nullopt : std::optional<Uid>(found[0]);
}
template<class Lookup, class Progress>
static Uid wait_for_color_endpoint(Lookup &lookup, Progress progress) {
    std::optional<Uid> selected;
    unsigned events = 0;
    while (!stopped && !selected && events < 64) {
        auto event = lookup.next();
        if (event) {
            ++events;
            selected = color_endpoint(*event);
        }
        progress();
    }
    require(!stopped, "Front-color discovery interrupted");
    require(selected.has_value(), "Front-color lookup event limit reached before endpoint readiness");
    return *selected;
}
static Uid discover_color(uke::color::State &state, int directory, std::uint64_t &published) {
    Uid selected;
    {
        const auto payload = color_lookup_payload();
        std::cerr << "Front-color discovery starting\n";
        Connection lookup(lookup_uid, 512, payload);
        std::cerr << "Front-color discovery acknowledged\n";
        // Keep the existing subscription until the endpoint publishes readiness.
        selected = wait_for_color_endpoint(lookup, [&] {
            const auto now = boottime_ns();
            if (now - published >= 1'000'000'000ULL) {
                uke::color::write_snapshot(directory, state.json(now));
                published = now;
            }
        });
    }
    Fd metadata(socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0));
    require(metadata.value >= 0, "QRTR metadata socket unavailable");
    std::cerr << "Front-color endpoint discovered; requesting attributes\n";
    gate_metadata(rpc(metadata.value, color_client_request(selected, 1, {}), 128), selected);
    std::cerr << "Front-color metadata admitted\n";
    return selected;
}
static void update(uke::color::State &state, View event, Uid selected, std::uint64_t now_ns) {
    auto root = fields(event), uid = fields(one(root, 1, 2));
    require(Uid{little(one(uid, 1, 1)), little(one(uid, 2, 1))} == selected, "Foreign front-color sample");
    bool measurement = false;
    unsigned count = 0;
    for (auto &entry : root) if (entry.number == 2) {
        require(entry.wire == 2 && ++count <= 32, "Invalid front-color event count");
        auto e = fields(entry.data); auto id = little(one(e, 1, 5));
        one(e, 2, 1); one(e, 3, 2);
        require(id != 130, "SSC returned a front-color error");
        measurement |= id == 1025;
    }
    if (measurement) for (auto &sample : samples(event, selected, 0))
        state.accept(sample.timestamp, unsigned(sample.status), sample.values, now_ns);
}
static void wait_for_stop() {
    sigset_t blocked{}, old{}; sigemptyset(&blocked);
    sigaddset(&blocked, SIGTERM); sigaddset(&blocked, SIGINT);
    require(sigprocmask(SIG_BLOCK, &blocked, &old) == 0, "Signal mask unavailable");
    while (!stopped) sigsuspend(&old);
    sigprocmask(SIG_SETMASK, &old, nullptr);
}
#ifndef UKE_FRONT_COLOR_TEST
int main(int argc, char **) {
    (void)&attributes;
    (void)&request;
    if (argc != 1) { std::cerr << "Service accepts no arguments; collection requires explicit service activation\n"; return 2; }
    std::signal(SIGTERM, stop_handler); std::signal(SIGINT, stop_handler);
    try {
        check_board();
        Fd directory(state_directory());
        Fd lock(openat(directory.value, "front-color.lock", O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0644));
        struct stat lock_stat{};
        require(lock.value >= 0 && fstat(lock.value, &lock_stat) == 0 && S_ISREG(lock_stat.st_mode) &&
                lock_stat.st_uid == 0 && lock_stat.st_nlink == 1 && !(lock_stat.st_mode & 0022) &&
                flock(lock.value, LOCK_EX | LOCK_NB) == 0, "Front-color publisher already active or lock untrusted");
        uke::color::State state;
        std::uint64_t published = boottime_ns();
        uke::color::write_snapshot(directory.value, state.json(published));
        auto stage = uke::color::Error::metadata;
        try {
            auto selected = discover_color(state, directory.value, published);
            stage = uke::color::Error::transport;
            Connection connection(selected);
            state.connect();
            while (!stopped) {
                auto event = connection.next(); auto now = boottime_ns();
                if (event) {
                    stage = uke::color::Error::invalid_sample;
                    update(state, *event, selected, now);
                    stage = uke::color::Error::transport;
                }
                if (now - published >= 1'000'000'000ULL) {
                    uke::color::write_snapshot(directory.value, state.json(now)); published = now;
                }
            }
        } catch (const std::exception &error) {
            std::cerr << "Front-color unavailable: " << error.what() << '\n';
            // Disconnect invalidates immediately; error publication still obeys 1 Hz.
            state.clear(stage); uke::color::remove_snapshot(directory.value);
            while (!stopped) {
                const auto elapsed = boottime_ns() - published;
                if (elapsed >= 1'000'000'000ULL) break;
                poll(nullptr, 0, int((1'000'000'000ULL - elapsed + 999'999) / 1'000'000));
            }
            if (!stopped) {
                uke::color::write_snapshot(directory.value, state.json(boottime_ns()));
                // Resource/service restart is explicit; never spin or retry a failed DSP.
                wait_for_stop();
            }
        }
        uke::color::remove_snapshot(directory.value);
        return 0;
    } catch (const std::exception &) {
        std::cerr << "Front-color service unavailable; inspect its runtime directory and service dependencies\n";
        return 1;
    }
}
#endif
