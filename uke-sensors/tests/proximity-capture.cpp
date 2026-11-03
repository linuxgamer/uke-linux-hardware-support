// SPDX-License-Identifier: MIT
#define UKE_PROXIMITY_CAPTURE_TEST
#include "../src/ssc-proximity-capture.cpp"
#include <cassert>
template<class F> static void rejects(F operation) {
    bool rejected = false;
    try { operation(); } catch (const std::exception &) { rejected = true; }
    assert(rejected);
}
static Bytes envelope(Uid sensor, unsigned id, View payload) {
    Bytes uid{9}; put(uid, sensor.low, 8); uid.push_back(0x11); put(uid, sensor.high, 8);
    Bytes event{0x0d}; put(event, id, 4); event.push_back(0x11); put(event, 1, 8);
    length_field(event, 3, payload);
    Bytes root; length_field(root, 1, uid); length_field(root, 2, event); return root;
}
static Bytes metadata(unsigned stream = 1, bool available = true, bool physical = true) {
    Bytes result;
    const Bytes name{0x12,9,'p','r','o','x','i','m','i','t','y'};
    Bytes stream_value{0x21}; put(stream_value, stream, 8);
    const std::array<Bytes,4> values{name, Bytes{0x28, std::uint8_t(available)}, stream_value, Bytes{0x28,std::uint8_t(physical)}};
    const std::array<unsigned,4> ids{2,3,16,21};
    for (unsigned i = 0; i < values.size(); ++i) {
        Bytes value; length_field(value, 1, values[i]);
        Bytes attr{8, std::uint8_t(ids[i])}; length_field(attr, 2, value);
        length_field(result, 1, attr);
    }
    return envelope({1,2}, 128, result);
}
int main() {
    // Independent 53-byte envelope: fixed64 SUID, non-wakeup1, passive1, small0.
    const Bytes expected{
        0,1,0,0x20,0,46,0,1,39,0,37,0,
        0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x15,2,2,0,0,0x1a,4,8,1,0x10,1,0x22,4,0x12,0,0x18,1,
        0x10,1,0,0
    };
    assert(expected.size() == 53 && lab_request({1,2},514,{},Mode::passive) == expected);
    rejects([] { lab_request({1,2},513,{},Mode::passive); });
    rejects([] { lab_request({1,2},514,{},Mode::metadata); });
    rejects([] { mode("metadata"); });
    rejects([] { mode("arbitrary"); });
    assert(mode("passive") == Mode::passive);
    assert(mode("active-small") == Mode::active_small);
    assert(mode("active-large") == Mode::active_large);
    assert(mode("active-empty") == Mode::active_empty);
    auto small = lab_request({1,2},514,{},Mode::active_small);
    auto large = lab_request({1,2},514,{},Mode::active_large);
    assert(small.size() == 49 && large.size() == small.size());
    auto expected_large = small; expected_large.back() = 1;
    assert(large == expected_large);
    for (auto selected : {Mode::active_small,Mode::active_large,Mode::active_empty}) {
        auto packet = lab_request({1,2},514,{},selected);
        auto request_fields = fields(tlv(tlvs(packet),1).subspan(2));
        auto standard = fields(one(request_fields,4,2));
        auto suspend = fields(one(request_fields,3,2));
        assert(little(one(suspend,2,0)) == 0);
        assert(standard.empty() == (selected != Mode::active_empty));
        if (selected == Mode::active_empty) assert(one(standard,2,2).empty());
        assert(tlv(tlvs(packet),0x10)[0] == (large_report(selected) ? 1 : 0));
    }
    auto lookup = default_lookup();
    auto client = fields(tlv(tlvs(lookup),1).subspan(2));
    auto payload = fields(one(fields(one(client,4,2)),2,2));
    assert(little(one(payload,2,0)) == 0 && little(one(payload,3,0)) == 1);
    gate_metadata(metadata(), {1,2});
    rejects([] { gate_metadata(metadata(0), {1,2}); });
    rejects([] { gate_metadata(metadata(1,false), {1,2}); });
    rejects([] { gate_metadata(metadata(1,true,false), {1,2}); });
    rejects([] { gate_metadata(metadata(), {2,1}); });
    rejects([] { gate_metadata(envelope({1,2},128,{}), {1,2}); });
    std::ostringstream out;
    unsigned count = 0;
    const auto event = envelope({1,2},1025,Bytes{8,1,0x10,3});
    record_events(event,{2,1},count,out);
    assert(count == 0 && out.str().empty());
    for (unsigned i = 0; i < 33; ++i) record_events(event,{1,2},count,out);
    auto logged = out.str();
    assert(count == 32 && std::count(logged.begin(),logged.end(),'\n') == 32);
    assert(logged.find("event_id=1025 payload_bytes=4 tags=1:0,2:0, payload_hex=08011003") != std::string::npos);
    assert(logged.find("SUID") == std::string::npos);
    count = 0; out.str(""); out.clear();
    record_events(envelope({1,2},1025,Bytes{0xff}),{1,2},count,out);
    assert(out.str().find("tags=invalid payload_hex=ff") != std::string::npos);
    auto truncated = event; truncated.pop_back();
    rejects([&] { record_events(truncated,{1,2},count,out); });
    (void)&attributes; (void)&request; (void)&rpc; (void)&suids; (void)&capture;
    std::cout << "Proximity capture modes, endpoint gate, privacy and refusal fixtures passed\n";
}
