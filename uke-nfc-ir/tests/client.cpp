// SPDX-License-Identifier: MIT
#include "tag.hpp"
#include <cassert>
#include <iostream>
#include <limits>

using namespace uke::nfctag;

struct Fake final : Device {
    fm19511_info info{1, FM19511_CAP_TYPE2_TAG | FM19511_CAP_MEMORY_READ | FM19511_CAP_RF_EVENTS, 256, {}};
    fm19511_events event{};
    unsigned int calls = 0, reads = 0, event_reads = 0, waits = 0;
    unsigned long fail_command = 0;
    int failure = EIO;
    int poll_error = 0;
    unsigned int corrupt_memory = 0;
    short readiness = 0;

    void request(unsigned long command, void *data) override {
        ++calls;
        if (command == fail_command)
            throw std::system_error(failure, std::generic_category());
        if (command == FM19511_IOC_INFO) {
            *static_cast<fm19511_info *>(data) = info;
        } else if (command == FM19511_IOC_READ_MEMORY) {
            ++reads;
            auto &memory = *static_cast<fm19511_memory *>(data);
            assert(!memory.reserved && memory.length && memory.length <= 256);
            for (auto byte : memory.data) assert(byte == 0);
            for (unsigned int i = 0; i < memory.length; ++i) memory.data[i] = i & 0xff;
            if (corrupt_memory == 1) memory.address ^= 1;
            if (corrupt_memory == 2) memory.length--;
            if (corrupt_memory == 3) memory.reserved = 1;
            if (corrupt_memory == 4) memory.data[memory.length] = 1;
        } else {
            assert(command == FM19511_IOC_EVENTS); // No writer or raw I2C fallback.
            ++event_reads;
            *static_cast<fm19511_events *>(data) = event;
        }
    }
    short wait(int timeout_ms) override {
        assert(timeout_ms == 10);
        ++waits;
        if (poll_error)
            throw std::system_error(poll_error, std::generic_category());
        if (readiness & POLLIN) event.pending = 1;
        if (readiness & POLLERR) event.error = -EREMOTEIO;
        return readiness;
    }
};

template<class F> void fails(F fn) {
    bool failed = false;
    try { fn(); } catch (const std::exception &) { failed = true; }
    assert(failed);
}

static void parser() {
    assert(number("65535", 65535) == 65535 && number("0xFFFF", 65535) == 65535);
    assert(number("0Xff", 256) == 255 && number("0", 0) == 0);
    for (const auto text : {"", "0x", "-1", "+1", " 1", "1 ", "1x", "65536", "4294967296"})
        fails([&] { number(text, 65535); });
    const char *good[]{"uke-nfc-tag", "read", "0xffff", "1"};
    auto command = parse(4, good);
    assert(command.operation == Operation::read && command.address == 65535 && command.length == 1);
    const char *wrap[]{"uke-nfc-tag", "read", "0xffff", "2"};
    const char *zero[]{"uke-nfc-tag", "read", "0", "0"};
    const char *wait[]{"uke-nfc-tag", "wait", "61"};
    const char *writer[]{"uke-nfc-tag", "write", "0", "1"};
    fails([&] { parse(4, wrap); }); fails([&] { parse(4, zero); });
    fails([&] { parse(3, wait); }); fails([&] { parse(4, writer); });
}

static void admission() {
    for (unsigned int fault = 0; fault < 5; ++fault) {
        Fake device;
        if (fault == 0) device.info.abi_version++;
        if (fault == 1) device.info.capabilities &= ~FM19511_CAP_TYPE2_TAG;
        if (fault == 2) device.info.max_transfer = 0;
        if (fault == 3) device.info.max_transfer = 257;
        if (fault == 4) device.info.reserved[4] = 1;
        fails([&] { Tag tag(device); });
        assert(device.calls == 1 && !device.reads && !device.waits);
    }
    Fake old;
    old.fail_command = FM19511_IOC_INFO; old.failure = ENOTTY;
    fails([&] { Tag tag(old); }); assert(old.calls == 1);
    Fake future;
    future.info.capabilities |= 1U << 31;
    Tag compatible(future); assert(compatible.info().abi_version == 1);
}

static void memory() {
    Fake device;
    Tag tag(device);
    assert(tag.read(0xffff, 1).length == 1);
    const auto full = tag.read(0xff00, 256);
    assert(full.data[0] == 0 && full.data[255] == 255);
    const auto before = device.calls;
    fails([&] { tag.read(0xffff, 2); }); fails([&] { tag.read(0, 0); });
    fails([&] { tag.read(0, 257); }); fails([&] { tag.read(65536, 1); });
    fails([&] { tag.read(std::numeric_limits<std::uint32_t>::max(), 1); });
    assert(device.calls == before);
    for (unsigned int fault = 1; fault <= 4; ++fault) {
        device.corrupt_memory = fault;
        fails([&] { tag.read(0, 8); });
    }
    device.corrupt_memory = 0; device.fail_command = FM19511_IOC_READ_MEMORY;
    for (const auto error : {EHOSTDOWN, ENODEV, EACCES, EINTR, EREMOTEIO}) {
        device.failure = error; const auto calls = device.calls;
        fails([&] { tag.read(0, 1); }); assert(device.calls == calls + 1);
    }
    Fake smaller;
    smaller.info.max_transfer = 8;
    Tag small(smaller); fails([&] { small.read(0, 9); }); assert(smaller.calls == 1);
    Fake unavailable;
    unavailable.info.capabilities &= ~FM19511_CAP_MEMORY_READ;
    Tag no_read(unavailable); fails([&] { no_read.read(0, 1); }); assert(unavailable.calls == 1);
}

static void events() {
    Fake device;
    Tag tag(device);
    assert(!tag.wait(10) && device.waits == 1);
    device.event.pending = 1;
    assert(tag.wait(10)->pending && device.waits == 1);
    device.event = {}; device.readiness = POLLIN;
    assert(tag.wait(10)->pending && device.waits == 2);
    device.event = {}; device.readiness = POLLERR;
    assert(tag.wait(10)->error == -EREMOTEIO);
    device.event = {}; device.readiness = POLLHUP;
    fails([&] { tag.wait(10); });
    device.readiness = POLLNVAL; fails([&] { tag.wait(10); });
    device.readiness = 0; device.poll_error = EINTR;
    const auto waits = device.waits;
    fails([&] { tag.wait(10); }); assert(device.waits == waits + 1);
    device.poll_error = 0;
    const auto before = device.calls;
    fails([&] { tag.wait(0); }); fails([&] { tag.wait(60001); }); assert(device.calls == before);
    for (unsigned int fault = 0; fault < 3; ++fault) {
        device.event = {};
        if (fault == 0) device.event.pending = 2;
        if (fault == 1) device.event.error = 1;
        if (fault == 2) device.event.reserved[1] = 1;
        fails([&] { tag.events(); });
    }
    Fake unavailable;
    unavailable.info.capabilities &= ~FM19511_CAP_RF_EVENTS;
    Tag no_events(unavailable); fails([&] { no_events.wait(10); });
    assert(unavailable.calls == 1 && !unavailable.waits);
}

int main() {
    parser(); admission(); memory(); events();
    std::cout << "Actual FM19511 client: ABI admission, bounded read-only ioctls, zeroed inputs, corrupted replies, no-retry errors, cached events/poll/removal and argument refusals passed; fake providers only\n";
}
