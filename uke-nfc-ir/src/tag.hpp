// SPDX-License-Identifier: MIT
#pragma once

#include "fm19511.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <poll.h>
#include <stdexcept>
#include <string_view>
#include <system_error>

namespace uke::nfctag {

static_assert(sizeof(fm19511_info) == 32);
static_assert(sizeof(fm19511_events) == 32);
static_assert(sizeof(fm19511_memory) == 264);
static_assert(offsetof(fm19511_events, error) == 16);
static_assert(offsetof(fm19511_memory, data) == 8);

class UsageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

inline std::uint32_t number(std::string_view text, std::uint32_t maximum) {
    int base = 10;
    if (text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text.remove_prefix(2);
        base = 16;
    }
    if (text.empty())
        throw UsageError("Expected a bounded unsigned decimal or hexadecimal number");
    std::uint32_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || value > maximum)
        throw UsageError("Expected a bounded unsigned decimal or hexadecimal number");
    return value;
}

inline void check_read(std::uint32_t address, std::uint32_t length, std::uint32_t maximum) {
    if (address > 0xffff || !length || length > maximum || length > FM19511_MAX_TRANSFER ||
        address + length > 0x10000)
        throw UsageError("Read requires a 16-bit address and 1..256 bytes without address wrap");
}

enum class Operation { info, events, read, wait, help };
struct Request {
    Operation operation;
    std::uint32_t address = 0, length = 0, seconds = 10;
};

inline Request parse(int argc, const char *const *argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--help")
        return {Operation::help};
    if (argc == 2 && std::string_view(argv[1]) == "info")
        return {Operation::info};
    if (argc == 2 && std::string_view(argv[1]) == "events")
        return {Operation::events};
    if ((argc == 2 || argc == 3) && std::string_view(argv[1]) == "wait") {
        Request result{Operation::wait};
        if (argc == 3)
            result.seconds = number(argv[2], 60);
        if (!result.seconds)
            throw UsageError("Wait duration must be 1..60 seconds");
        return result;
    }
    if (argc == 4 && std::string_view(argv[1]) == "read") {
        Request result{Operation::read};
        result.address = number(argv[2], 0xffff);
        result.length = number(argv[3], FM19511_MAX_TRANSFER);
        check_read(result.address, result.length, FM19511_MAX_TRANSFER);
        return result;
    }
    throw UsageError("Usage: uke-nfc-tag info|events|read ADDRESS LENGTH|wait [SECONDS]|--help");
}

class Device {
public:
    virtual ~Device() = default;
    virtual void request(unsigned long command, void *data) = 0;
    virtual short wait(int timeout_ms) = 0;
};

class Tag {
    Device &device_;
    fm19511_info info_{};
public:
    explicit Tag(Device &device) : device_(device) {
        try {
            device_.request(FM19511_IOC_INFO, &info_);
        } catch (const std::system_error &error) {
            if (error.code().value() == ENOTTY)
                throw std::runtime_error("The running kernel lacks FM19511 ABI1; update and boot the matching Uke kernel");
            throw;
        }
        if (info_.abi_version != FM19511_ABI_VERSION ||
            !(info_.capabilities & FM19511_CAP_TYPE2_TAG) ||
            !info_.max_transfer || info_.max_transfer > FM19511_MAX_TRANSFER ||
            std::any_of(std::begin(info_.reserved), std::end(info_.reserved), [](auto value) { return value != 0; }))
            throw std::runtime_error("Unsupported or malformed FM19511 ABI information");
    }

    const fm19511_info &info() const { return info_; }

    fm19511_memory read(std::uint32_t address, std::uint32_t length) {
        check_read(address, length, info_.max_transfer);
        if (!(info_.capabilities & FM19511_CAP_MEMORY_READ))
            throw std::runtime_error("This tag provider does not expose memory reads");
        fm19511_memory result{};
        result.address = static_cast<__u16>(address);
        result.length = static_cast<__u16>(length);
        device_.request(FM19511_IOC_READ_MEMORY, &result);
        if (result.address != address || result.length != length || result.reserved ||
            std::any_of(std::begin(result.data) + length, std::end(result.data), [](auto value) { return value != 0; }))
            throw std::runtime_error("Malformed FM19511 memory response");
        return result;
    }

    fm19511_events events() {
        if (!(info_.capabilities & FM19511_CAP_RF_EVENTS))
            throw std::runtime_error("This tag provider does not expose RF notifications");
        fm19511_events result{};
        device_.request(FM19511_IOC_EVENTS, &result);
        if (result.pending > 1 || result.error > 0 ||
            std::any_of(std::begin(result.reserved), std::end(result.reserved), [](auto value) { return value != 0; }))
            throw std::runtime_error("Malformed FM19511 event response");
        return result;
    }

    std::optional<fm19511_events> wait(int timeout_ms) {
        if (timeout_ms < 1 || timeout_ms > 60000)
            throw UsageError("Wait duration must be 1..60000 milliseconds");
        const auto current = events();
        if (current.pending || current.error)
            return current;
        const short result = device_.wait(timeout_ms);
        if (!result)
            return std::nullopt;
        if (result & POLLNVAL)
            throw std::system_error(EBADF, std::generic_category(), "NFC descriptor is invalid");
        if (result & POLLHUP)
            throw std::system_error(ENODEV, std::generic_category(), "NFC tag was removed");
        if (!(result & (POLLIN | POLLRDNORM | POLLERR)))
            throw std::runtime_error("Unexpected NFC poll result");
        return events();
    }
};

} // namespace uke::nfctag
