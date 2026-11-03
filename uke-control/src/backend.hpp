// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace uke {
struct Result { int status; std::string output; };
Result execute(const std::vector<std::string>& arguments, unsigned seconds=20, size_t limit=16*1024*1024);
uint64_t swap_mib(const std::string& policy, uint64_t memory_bytes);
std::string read(const std::string& path, size_t limit=1024*1024);
std::string status();
std::string inventory(bool save);
void swap_set(const std::string& policy);
void swap_apply();
void boot();
std::string wake_status();
void wake_set(const std::string& source, bool enabled);
void wake_apply();
void display_vrr_set(unsigned hz);
std::string charge_status();
void charge_set(unsigned percent);
Result charge_control_query(const std::string& operation);
Result charge_enable_preconfigured_cap();
Result charge_enable_moisture();
Result charge_apply_preconfigured_cap();
Result charge_control_mode(const std::string& mode);
std::string usb_status();
std::string firmware_status();
std::string hardware_status();
void usb_watch(unsigned seconds);
Result usb_request_data(const std::string& role);
Result display_profile_set(const std::string& profile);
Result display_lane_limit(const std::string& lanes);
Result usb_power_set(const std::string& device,const std::string& policy);
Result ethernet_profile_set(const std::string& interface,const std::string& profile);
Result cdc_set(bool enabled);
Result cdc_persistent(bool enabled);
Result cdc_apply();
std::string report();
void report_github(const std::string& directory,const std::string& repository);
void self_test();
}
