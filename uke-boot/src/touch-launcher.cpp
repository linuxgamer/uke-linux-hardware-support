// SPDX-License-Identifier: MIT
// Select the matching running-kernel client; retain the reviewed legacy rollback route.
#include "boot-common.hpp"
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace fs=std::filesystem;
static bool touch_compatible(const std::string &value,const std::string &expected) {
    bool found=false;
    for(std::size_t start=0;start<value.size();) {
        const auto end=value.find('\0',start);
        if(end==std::string::npos)return false;
        found|=value.compare(start,end-start,expected)==0;start=end+1;
    }
    return found;
}
static fs::path touch_client(const fs::path &root,const std::string &release) {
    if(!uke_device(read_text(root/"sys/firmware/devicetree/base/compatible")))
        throw std::runtime_error("Touch launch requires exact Uke / SM7675 identity");
    unsigned legacy=0,kernel=0;
    for(const auto &entry:fs::directory_iterator(root/"sys/bus/spi/devices")) {
        const auto value=read_text(entry.path()/"of_node/compatible");
        legacy+=touch_compatible(value,"novatek,NVT-ts-spi");
        kernel+=touch_compatible(value,"novatek,nt36532-uke-thp");
    }
    if(legacy+kernel!=1)throw std::runtime_error("A unique reviewed Uke touch transport is required");
    fs::path selected=root/"usr/libexec/senemos-uke/uke-touch-thp";
    if(kernel) {
        if(release.empty() || release.size()>=128 || !release.starts_with("7.") ||
           release.find("..")!=std::string::npos ||
           release.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._+-")!=std::string::npos)
            throw std::runtime_error("Invalid running Uke kernel release");
        selected=root/"usr/lib/modules"/release/"uke/uke-touch-capture";
    }
    struct stat st{};
    if(lstat(selected.c_str(),&st)<0 || !S_ISREG(st.st_mode) || st.st_uid!=0 ||
       (st.st_mode&0022) || !(st.st_mode&S_IXUSR))
        throw std::runtime_error("Matched touch client is missing or untrusted");
    return selected;
}
int main(int argc,char **argv) {
    try {
        if(geteuid()!=0 || argc!=2 || std::string_view(argv[1])!="--daemon")
            throw std::runtime_error("Usage: root uke-touchscreen --daemon");
        utsname kernel{};
        if(uname(&kernel)<0)throw std::runtime_error("Running kernel release unavailable");
        const auto selected=touch_client("/",kernel.release);
        const char *arguments[]{selected.c_str(),"--daemon",nullptr};
        execv(selected.c_str(),const_cast<char *const *>(arguments));
        throw std::runtime_error("Cannot execute the matched touch client");
    } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
