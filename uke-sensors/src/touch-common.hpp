// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
static void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
static bool matches(const std::string &data, const std::string &expected) {
    if(data.size()>4096) return false;
    bool found=false;
    for(std::size_t p=0;p<data.size();) {
        auto end=data.find('\0',p);
        if(end==std::string::npos) return false;
        if(data.compare(p,end-p,expected)==0) found=true;
        p=end+1;
    }
    return found;
}
static bool compatible(const char *path, const std::string &expected) {
    std::ifstream input(path, std::ios::binary);
    if(!input) return false;
    std::array<char,4097> data{};
    input.read(data.data(),data.size());
    return input.eof() && !input.bad() && matches(std::string(data.data(),input.gcount()),expected);
}
