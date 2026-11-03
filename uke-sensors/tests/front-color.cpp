// SPDX-License-Identifier: MIT
#define UKE_FRONT_COLOR_TEST
#include "../src/front-color.cpp"
#include <cassert>
#include <filesystem>
#include <fstream>

template<class F> static void rejects(F operation) {
    bool rejected = false;
    try { operation(); } catch (const std::exception &) { rejected = true; }
    assert(rejected);
}
static Bytes event(Uid sensor, unsigned id, View payload, std::uint64_t ticks = 1) {
    Bytes uid{9}; put(uid, sensor.low, 8); uid.push_back(0x11); put(uid, sensor.high, 8);
    Bytes e{0x0d}; put(e, id, 4); e.push_back(0x11); put(e, ticks, 8); length_field(e, 3, payload);
    Bytes root; length_field(root, 1, uid); length_field(root, 2, e); return root;
}
static Bytes sample(std::uint64_t ticks, unsigned status = 3) {
    Bytes payload{0x0d}; put(payload, std::bit_cast<std::uint32_t>(4749.0f), 4);
    payload.insert(payload.end(), {0x10,std::uint8_t(status)});
    return event({1,2},1025,payload,ticks);
}
static Bytes metadata(unsigned stream = 1, bool available = true, bool physical = true) {
    const Bytes type{0x12,9,'c','c','t','_','f','r','o','n','t'};
    Bytes stream_value{0x21}; put(stream_value, stream, 8);
    const std::array<Bytes,4> values{type,Bytes{0x28,std::uint8_t(available)},stream_value,Bytes{0x28,std::uint8_t(physical)}};
    const std::array<unsigned,4> ids{2,3,16,21};
    Bytes payload;
    for (unsigned i = 0; i < values.size(); ++i) {
        Bytes value; length_field(value,1,values[i]);
        Bytes attr{8,std::uint8_t(ids[i])}; length_field(attr,2,value);
        length_field(payload,1,attr);
    }
    return event({1,2},128,payload);
}
static std::string contents(const std::filesystem::path &path) {
    std::ifstream in(path); return std::string(std::istreambuf_iterator<char>(in), {});
}
int main() {
    auto lookup_payload = color_lookup_payload();
    auto lookup_fields = fields(lookup_payload);
    std::size_t offset = 0;
    assert(varint(one(lookup_fields,2,0),offset) == 1);
    offset = 0; assert(varint(one(lookup_fields,3,0),offset) == 0);
    const auto lookup_packet = color_client_request(lookup_uid,512,lookup_payload);
    const Bytes wakeup_lookup{
        0,1,0,0x20,0,0x37,0,1,0x34,0,0x32,0,
        0x0a,18,9,0xab,0xab,0xab,0xab,0xab,0xab,0xab,0xab,
        0x11,0xab,0xab,0xab,0xab,0xab,0xab,0xab,0xab,
        0x15,0,2,0,0,0x1a,4,8,1,0x10,0,
        0x22,17,0x12,15,0x0a,9,'c','c','t','_','f','r','o','n','t',0x10,1,0x18,0
    };
    assert(lookup_packet == wakeup_lookup);
    auto lookup_client = fields(tlv(tlvs(lookup_packet),1).subspan(2));
    assert(little(one(fields(one(lookup_client,3,2)),2,0)) == 0);
    const auto sample_packet = color_client_request({1,2},514,{});
    auto sample_client = fields(tlv(tlvs(sample_packet),1).subspan(2));
    assert(little(one(fields(one(sample_client,3,2)),2,0)) == 1);
    const auto metadata_packet = color_client_request({1,2},1,{});
    const Bytes wakeup_metadata{
        0,1,0,0x20,0,40,0,1,37,0,35,0,
        0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x15,1,0,0,0,0x1a,4,8,1,0x10,0,0x22,2,0x12,0
    };
    assert(metadata_packet == wakeup_metadata);
    const auto diagnostic_metadata = request("",{1,2},1);
    const auto diagnostic_client = fields(tlv(tlvs(diagnostic_metadata),1).subspan(2));
    assert(little(one(fields(one(diagnostic_client,3,2)),2,0)) == 1);
    Bytes lookup_result;
    const std::string_view type = "cct_front";
    length_field(lookup_result,1,View(reinterpret_cast<const std::uint8_t *>(type.data()),type.size()));
    assert(!color_endpoint(event(lookup_uid,768,lookup_result)));
    Bytes uid{9}; put(uid,1,8); uid.push_back(0x11); put(uid,2,8);
    length_field(lookup_result,2,uid);
    assert((color_endpoint(event(lookup_uid,768,lookup_result)) == Uid{1,2}));
    struct Lookup {
        std::vector<std::optional<Bytes>> events;
        std::size_t consumed = 0;
        std::optional<Bytes> next() { return events.at(consumed++); }
    };
    Lookup late;
    late.events.resize(45); // Simulated one-second transport polls before readiness.
    late.events.push_back(event(lookup_uid,768,lookup_result));
    std::uint64_t elapsed = 0;
    assert((wait_for_color_endpoint(late,[&] { elapsed += 1'000'000'000ULL; }) == Uid{1,2}));
    assert(elapsed > 30'000'000'000ULL && late.consumed == 46);
    Lookup cancelled;
    cancelled.events.resize(2);
    rejects([&] { wait_for_color_endpoint(cancelled,[] { stopped = 1; }); });
    assert(cancelled.consumed == 1);
    stopped = 0;
    Bytes empty_result;
    length_field(empty_result,1,View(reinterpret_cast<const std::uint8_t *>(type.data()),type.size()));
    Lookup excessive;
    excessive.events.assign(65,event(lookup_uid,768,empty_result));
    rejects([&] { wait_for_color_endpoint(excessive,[] {}); });
    assert(excessive.consumed == 64);
    rejects([&] { color_endpoint(event({1,2},768,lookup_result)); });
    length_field(lookup_result,2,uid);
    rejects([&] { color_endpoint(event(lookup_uid,768,lookup_result)); });
    using uke::color::State;
    using uke::color::Error;
    struct stat public_dir{}; public_dir.st_mode = S_IFDIR | 0755; public_dir.st_uid = 0;
    assert(uke::color::public_directory(public_dir));
    for (auto mode : {0700,0750,0775,0777,01755}) {
        public_dir.st_mode = S_IFDIR | mode;
        assert(!uke::color::public_directory(public_dir));
    }
    public_dir.st_mode = S_IFDIR | 0755; public_dir.st_uid = 1000;
    assert(!uke::color::public_directory(public_dir));
    public_dir.st_uid = 0; public_dir.st_mode = S_IFREG | 0755;
    assert(!uke::color::public_directory(public_dir));
    State state;
    assert(state.json(1).find("\"available\":false") != std::string::npos);
    assert(state.json(1).find("\"status\":null") != std::string::npos);
    rejects([&] { update(state,sample(1),{1,2},10); });
    state.connect();
    gate_metadata(metadata(),{1,2});
    rejects([] { gate_metadata(metadata(0),{1,2}); });
    rejects([] { gate_metadata(metadata(1,false),{1,2}); });
    rejects([] { gate_metadata(metadata(1,true,false),{1,2}); });
    rejects([] { gate_metadata(metadata(),{2,1}); });
    update(state,event({1,2},1022,{}),{1,2},9);
    assert(state.values.empty());
    update(state,sample(1),{1,2},10);
    auto json = state.json(11);
    assert(json.find("\"available\":true") != std::string::npos);
    assert(json.find("\"values\":[4749]") != std::string::npos);
    assert(json.find("\"dsp_clock_hz\":null") != std::string::npos);
    assert(json.find("\"units\":\"unverified\"") != std::string::npos);
    assert(json.find("\"calibration\":\"unverified\"") != std::string::npos);
    assert(json.find("uid") == std::string::npos && json.size() <= 4096);
    rejects([&] { update(state,sample(1),{1,2},11); });
    rejects([&] { update(state,sample(2),{2,1},11); });
    rejects([&] { update(state,event({1,2},130,{}),{1,2},11); });
    rejects([&] { state.accept(2,4,{1},11); });
    rejects([&] { state.accept(2,3,{},11); });
    rejects([&] { state.accept(2,3,std::vector<float>(17,1),11); });
    rejects([&] { state.accept(2,3,{std::numeric_limits<float>::quiet_NaN()},11); });
    rejects([&] { state.accept(2,3,{std::numeric_limits<float>::infinity()},11); });
    rejects([&] { state.accept(2,3,{1},9); });
    assert(state.json(9).find("\"fresh\":false") != std::string::npos);
    assert(state.json(30'000'000'011ULL).find("\"available\":false") != std::string::npos);
    assert(state.json(30'000'000'011ULL).find("\"has_sample\":true") != std::string::npos);
    update(state,sample(2,0),{1,2},12);
    assert(state.json(13).find("\"fresh\":true,\"available\":false") != std::string::npos);
    state.clear(Error::transport);
    json = state.json(14);
    assert(json.find("\"values\":[]") != std::string::npos && json.find("\"timestamp_ticks\":null") != std::string::npos);
    state.connect(); update(state,sample(1),{1,2},15); // New connection resets the DSP epoch.
    auto truncated = sample(2); truncated.pop_back();
    rejects([&] { update(state,truncated,{1,2},16); });
    auto too_many = sample(2);
    const Bytes entry(too_many.begin() + 20,too_many.end());
    for (unsigned i = 0; i < 32; ++i) too_many.insert(too_many.end(),entry.begin(),entry.end());
    rejects([&] { update(state,too_many,{1,2},16); });

    char temporary[] = "/tmp/uke-front-color-test-XXXXXX";
    char *created = mkdtemp(temporary); assert(created);
    const std::filesystem::path directory(created), target = directory/"front-color.json", victim = directory/"victim";
    {
        Fd fd(open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC)); assert(fd.value >= 0);
        uke::color::write_snapshot(fd.value,state.json(16));
        struct stat st{}; assert(lstat(target.c_str(),&st) == 0 && (st.st_mode & 0777) == 0644 && st.st_uid == geteuid());
        auto first = contents(target); assert(first == state.json(16));
        uke::color::write_snapshot(fd.value,state.json(17)); assert(contents(target) == state.json(17));
        rejects([&] { uke::color::write_snapshot(fd.value,std::string(4097,'x')); });
        assert(contents(target) == state.json(17));
        uke::color::remove_snapshot(fd.value);
        { std::ofstream out(victim); out << "preserved"; }
        assert(symlink("victim",target.c_str()) == 0);
        rejects([&] { uke::color::write_snapshot(fd.value,state.json(18)); });
        rejects([&] { uke::color::remove_snapshot(fd.value); });
        assert(contents(victim) == "preserved"); std::filesystem::remove(target);
        assert(link(victim.c_str(),target.c_str()) == 0);
        rejects([&] { uke::color::write_snapshot(fd.value,state.json(18)); });
        assert(contents(victim) == "preserved"); std::filesystem::remove(target);
        assert(std::filesystem::create_directory(target));
        rejects([&] { uke::color::write_snapshot(fd.value,state.json(18)); });
        std::filesystem::remove(target);
        uke::color::write_snapshot(fd.value,state.json(18));
        assert(chmod(target.c_str(),0666) == 0);
        rejects([&] { uke::color::write_snapshot(fd.value,state.json(19)); });
        assert(chmod(target.c_str(),0644) == 0);
        uke::color::remove_snapshot(fd.value);
        assert(!std::filesystem::exists(target));
        for (auto &entry : std::filesystem::directory_iterator(directory)) assert(entry.path() == victim);
    }
    std::filesystem::remove_all(directory);
    (void)&attributes; (void)&discover_color; (void)&state_directory; (void)&check_board;
    (void)&stop_handler; (void)&wait_for_stop;
    std::cout << "Front-color state, framing, metadata, freshness and atomic-file refusal fixtures passed\n";
}
