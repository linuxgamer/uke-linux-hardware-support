// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "touch-common.hpp"
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <unistd.h>
enum class NormalPanel { Csot, Tm };
struct TouchPanelState {
    std::uint64_t generation{};
    bool ready{};
    bool operator==(const TouchPanelState &) const = default;
};
struct TouchPanelTransition : std::runtime_error {
    TouchPanelTransition():std::runtime_error("Panel reset changed during touch initialization"){}
};
[[maybe_unused]] static std::filesystem::path touch_panel_provider(const std::filesystem::path &root="/sys/bus/mipi-dsi/devices") {
    std::filesystem::path selected;
    if(!std::filesystem::exists(root)) return selected;
    for(const auto &entry:std::filesystem::directory_iterator(root)) {
        const auto state=entry.path()/"touch_reset_state";
        const auto id=(entry.path()/"of_node/compatible").string();
        if(!std::filesystem::exists(state) ||
           (!compatible(id.c_str(),"xiaomi,o82-42-02-0b") &&
            !compatible(id.c_str(),"xiaomi,o82-36-02-0a"))) continue;
        require(selected.empty(),"Ambiguous O82 panel reset provider");
        selected=state;
    }
    return selected;
}
[[maybe_unused]] static TouchPanelState touch_panel_state(int descriptor) {
    char bytes[64];ssize_t size;
    do {size=pread(descriptor,bytes,sizeof(bytes),0);}while(size<0 && errno==EINTR);
    require(size>0 && size<ssize_t(sizeof(bytes)),"O82 panel event read failed");
    std::uint64_t value=0;
    const auto parsed=std::from_chars(bytes,bytes+size,value);
    require(parsed.ec==std::errc{} && parsed.ptr>bytes && parsed.ptr-bytes<=20 &&
            bytes+size-parsed.ptr==3 && parsed.ptr[0]==' ' &&
            (parsed.ptr[1]=='0' || parsed.ptr[1]=='1') && parsed.ptr[2]=='\n' &&
            value<=UINT64_MAX/2,"Invalid O82 panel reset state");
    return {value,parsed.ptr[1]=='1'};
}
