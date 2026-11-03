// SPDX-License-Identifier: MIT
#include "plasma.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>
static std::vector<std::vector<std::string>> calls;
static uke::Result response{0, ""};
namespace uke {
Result execute(const std::vector<std::string>& args, unsigned seconds, size_t limit) {
    assert(seconds == 5 && limit == 65536);
    calls.push_back(args);
    return response;
}
}
int main() {
    const std::string panel = "Output: 1 DSI-1 enabled connected\n"
        "\tAuto Rotate Policy: always\n\tAutomatic brightness: supported, disabled\n";
    response.output = "\033[01;31m" + panel + "\033[0mOutput: 2 DP-1 enabled\n"
        "\tAuto Rotate Policy: never\n\tAutomatic brightness: supported, enabled\n";
    assert(uke::desktop::plasma_status().find("\"rotation\":true,\"automatic_brightness\":false") != std::string::npos);
    uke::desktop::plasma_set("rotation", "off");
    assert(calls.back() == std::vector<std::string>({"/usr/bin/kscreen-doctor", "output.DSI-1.autoRotatePolicy.never"}));
    uke::desktop::plasma_set("brightness", "on");
    assert(calls.back().back() == "output.DSI-1.autoBrightness.enable");
    response.output = "Output: 1 DP-1\nAuto Rotate Policy: never\nAutomatic brightness: supported, disabled\n"
        "Output: 2 DSI-2\nAuto Rotate Policy: inTabletMode\nAutomatic brightness: supported, enabled\n";
    assert(uke::desktop::plasma_status().find("\"output\":\"DSI-2\",\"rotation\":false,\"automatic_brightness\":true") != std::string::npos);
    uke::desktop::plasma_set("rotation", "on");
    assert(calls.back().back() == "output.DSI-2.autoRotatePolicy.always");
    for (const auto& output : {panel + panel, panel + response.output, std::string("Output: 2 DP-1\nAuto Rotate Policy: always\n"),
            std::string("Output: 1 DSI-1\nAuto Rotate Policy: incapable\nAutomatic brightness: unsupported\n"),
            panel + "Auto Rotate Policy: never\nAutomatic brightness: unsupported\n"}) {
        response.output = output;
        assert(uke::desktop::plasma_status().find("\"rotation\":null,\"automatic_brightness\":null") != std::string::npos);
        const auto count = calls.size();
        try { uke::desktop::plasma_set("rotation", "on"); assert(false); } catch (const std::runtime_error&) {}
        assert(calls.size() == count + 1);
    }
    response = {1, panel};
    assert(uke::desktop::plasma_status().find("\"rotation\":null") != std::string::npos);
    const auto count = calls.size();
    try { uke::desktop::plasma_set("rotation", "on;touch /tmp/unsafe"); assert(false); } catch (const std::runtime_error&) {}
    assert(calls.size() == count);
}
