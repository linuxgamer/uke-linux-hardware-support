// SPDX-License-Identifier: MIT
#pragma once
#include <string>
namespace uke::diagnostic {
int command(int argc,char** argv);
std::string collect(const std::string& topic);
}
