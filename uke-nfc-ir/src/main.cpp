// SPDX-License-Identifier: MIT
#include "tag.hpp"
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

class NativeDevice final : public uke::nfctag::Device {
    int fd_ = -1;
public:
    NativeDevice() {
        fd_ = ::open("/dev/fm-ntag", O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
        if (fd_ < 0)
            throw std::system_error(errno, std::generic_category(), "Cannot open the NFC tag");
        struct stat status{};
        const int result = ::fstat(fd_, &status);
        const int error = result < 0 ? errno : ENODEV;
        if (result < 0 || !S_ISCHR(status.st_mode)) {
            ::close(fd_);
            fd_ = -1;
            throw std::system_error(error, std::generic_category(), "NFC path is not an accessible character device");
        }
    }
    NativeDevice(const NativeDevice &) = delete;
    NativeDevice &operator=(const NativeDevice &) = delete;
    ~NativeDevice() override { if (fd_ >= 0) ::close(fd_); }

    void request(unsigned long command, void *data) override {
        const int result = ::ioctl(fd_, command, data);
        if (result < 0)
            throw std::system_error(errno, std::generic_category(), "NFC request failed");
        if (result)
            throw std::runtime_error("Unexpected NFC ioctl result");
    }
    short wait(int timeout_ms) override {
        struct pollfd item{fd_, POLLIN, 0};
        if (::poll(&item, 1, timeout_ms) < 0)
            throw std::system_error(errno, std::generic_category(), "NFC wait interrupted or failed");
        return item.revents;
    }
};

void print_events(const fm19511_events &events) {
    std::cout << "{\"irq_sequence\":" << events.irq_sequence
              << ",\"status_sequence\":" << events.status_sequence
              << ",\"pending\":" << (events.pending ? "true" : "false")
              << ",\"error\":" << events.error << ",\"raw_irq_hex\":\""
              << std::hex << std::setfill('0')
              << std::setw(2) << static_cast<unsigned>(events.main_irq)
              << std::setw(2) << static_cast<unsigned>(events.fifo_irq)
              << std::setw(2) << static_cast<unsigned>(events.aux_irq) << "\"}\n";
}

} // namespace

int main(int argc, char **argv) {
    using namespace uke::nfctag;
    try {
        const auto command = parse(argc, argv);
        if (command.operation == Operation::help) {
            std::cout << "Usage: uke-nfc-tag info|events|read ADDRESS LENGTH|wait [SECONDS]\n"
                         "Read-only FM19511 Type 2 tag client; no reader, memory writer or card emulation.\n"
                         "Read: unsigned decimal/hex address and 1..256 bytes, without address wrap.\n"
                         "Wait: 1..60 seconds, default 10; exit 3 on timeout. Events are raw notifications.\n"
                         "Requires the matching kernel ABI and local-session access to /dev/fm-ntag.\n";
            return 0;
        }
        NativeDevice device;
        Tag tag(device);
        if (command.operation == Operation::info) {
            const auto &info = tag.info();
            std::cout << "{\"role\":\"type2-tag\",\"abi_version\":" << info.abi_version
                      << ",\"max_transfer\":" << info.max_transfer
                      << ",\"memory_read\":" << ((info.capabilities & FM19511_CAP_MEMORY_READ) ? "true" : "false")
                      << ",\"rf_notifications\":" << ((info.capabilities & FM19511_CAP_RF_EVENTS) ? "true" : "false")
                      << ",\"wake_capable\":" << ((info.capabilities & FM19511_CAP_WAKEUP) ? "true" : "false")
                      << ",\"memory_write\":false,\"reader\":false}\n";
            return 0;
        }
        if (command.operation == Operation::read) {
            const auto data = tag.read(command.address, command.length);
            std::cout << "{\"address\":" << data.address << ",\"length\":" << data.length << ",\"data_hex\":\""
                      << std::hex << std::setfill('0');
            for (unsigned int i = 0; i < data.length; ++i)
                std::cout << std::setw(2) << static_cast<unsigned>(data.data[i]);
            std::cout << "\"}\n";
            return 0;
        }
        if (command.operation == Operation::events) {
            const auto result = tag.events();
            print_events(result);
            return result.error ? 1 : 0;
        }
        const auto result = tag.wait(static_cast<int>(command.seconds * 1000));
        if (!result) {
            std::cout << "{\"timeout\":true}\n";
            return 3;
        }
        print_events(*result);
        return result->error ? 1 : 0;
    } catch (const UsageError &error) {
        std::cerr << error.what() << '\n';
        return 2;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
