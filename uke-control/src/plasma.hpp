// SPDX-License-Identifier: MIT
#pragma once
#include "backend.hpp"
namespace uke::desktop {
std::string plasma_status();
Result plasma_set(const std::string& feature, const std::string& value);
}
