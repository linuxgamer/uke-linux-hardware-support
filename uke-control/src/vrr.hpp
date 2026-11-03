// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
namespace uke::vrr {
inline constexpr const char* policy_name="vrr-min";
struct Driver {
    std::optional<unsigned> loaded;
    unsigned upper{};
    std::string reason;
};
unsigned minimum(std::string_view value);
std::string policy(unsigned hz);
std::optional<unsigned> configured(const std::filesystem::path& directory="/var/lib/uke-control");
Driver driver();
void validate(unsigned hz,const Driver& current);
std::string status();
}
