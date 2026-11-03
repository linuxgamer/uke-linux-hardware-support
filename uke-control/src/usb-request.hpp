// SPDX-License-Identifier: MIT
#pragma once
#include "backend.hpp"
#include <filesystem>
#include <string>
namespace uke::usb {
struct RequestState {
    bool profile=false,port=false,partner=false,role_switch=false;
    bool storage=false,network=false,display=false,gadget=false;
    std::string typec_role,controller_role;
};
std::string request_refusal(const RequestState&,const std::string& role);
Result request_data(const std::string& role);
char profile_pin(const std::string& assignments,const std::string& profile);
std::string display_profiles(const std::filesystem::path& root);
Result request_display_profile(const std::string& profile);
}
