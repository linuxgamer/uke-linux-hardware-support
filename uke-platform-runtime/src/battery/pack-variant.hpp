// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <initializer_list>
#include <string_view>

namespace uke::battery {
// Standard firmware strings are hints; they never qualify an electrical profile.
inline std::string_view pack_supplier_hint(std::string_view manufacturer, std::string_view model) {
    unsigned int suppliers = 0;
    for (const auto value : {manufacturer, model}) {
        if (value.size() > 128) return "unknown";
        for (const unsigned char c : value)
            if (c < 32 || c >= 127) return "unknown";
        const auto letter = [](char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        };
        for (std::size_t begin = 0; begin < value.size();) {
            while (begin < value.size() && !letter(value[begin])) ++begin;
            std::size_t end = begin;
            while (end < value.size() && letter(value[end])) ++end;
            const auto token = value.substr(begin, end - begin);
            const auto matches = [token](std::string_view expected) {
                if (token.size() != expected.size()) return false;
                for (std::size_t i = 0; i < token.size(); ++i) {
                    char c = token[i];
                    if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
                    if (c != expected[i]) return false;
                }
                return true;
            };
            if (matches("NVT")) suppliers |= 1;
            if (matches("SUNWODA")) suppliers |= 2;
            begin = end;
        }
    }
    return suppliers == 1 ? "nvt" : suppliers == 2 ? "sunwoda" : suppliers == 3 ? "conflicting" : "unknown";
}
}
