// SPDX-License-Identifier: MIT
// The RPM-verified bundle opts into private preparation; legacy bundles do not.
#pragma once
#include <charconv>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
namespace uke_color {
inline bool required(const std::map<std::string, std::string> &manifest) {
    const auto marker = manifest.find("o82_private_color");
    if (marker == manifest.end()) return false;
    const auto target = manifest.find("kernel_release");
    if (marker->second != "5" || target == manifest.end())
        throw std::runtime_error("Unsupported private-color preparation marker");
    const std::string_view release = target->second;
    if (!release.starts_with("7.3.1-") || !release.ends_with("-unstable") || release.size() <= 15)
        throw std::runtime_error("Private-color preparation requires an unstable kernel");
    const auto digits = release.substr(6, release.size() - 15);
    unsigned revision = 0;
    const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), revision);
    if (parsed.ec != std::errc() || parsed.ptr != digits.data() + digits.size() || !revision || digits.front() == '0')
        throw std::runtime_error("Invalid private-color kernel revision");
    return true;
}
template<class Prepare, class Stage>
void with_prepared(const std::map<std::string, std::string> &manifest,
                   const std::string &running, bool dry, Prepare prepare, Stage stage) {
    if (required(manifest)) {
        const auto &release = manifest.at("kernel_release");
        if (release == running)
            throw std::runtime_error("Private-color preparation is confined to a non-running kernel");
        if (!dry) prepare(release);
    }
    stage();
}
} // namespace uke_color
