// SPDX-License-Identifier: MIT
#include "link-policy.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace fs=std::filesystem;
static void put(const fs::path& p,const std::string& s){fs::create_directories(p.parent_path());std::ofstream(p)<<s;}
template<class F> static void refuses(F f){try{f();assert(false);}catch(const std::runtime_error&){}}
int main() {
    const auto capabilities=[](const std::string& modes,const std::string& advertised="\"1000baseT/Full\"",bool enabled=true) {
        return "[{\"ifname\":\"eth-test\",\"supports-auto-negotiation\":true,\"auto-negotiation\":"+
            std::string(enabled?"true":"false")+",\"supported-link-modes\":["+modes+"],\"advertised-link-modes\":["+advertised+"]}]";
    };
    const std::string full="\"10baseT/Half\",\"10baseT/Full\",\"100baseT/Half\",\"100baseT/Full\",\"1000baseT/Full\"";
    const auto all=capabilities(full);
    for(const auto& [profile,mask]:std::vector<std::pair<std::string,std::string>>{{"auto","2f"},{"1000-full","20"},{"100-full","8"},{"10-full","2"}}) {
        const auto args=uke::links::ethernet_arguments("eth-test",profile,all);
        assert(args==std::vector<std::string>({"--change","eth-test","autoneg","on","advertise",mask+"/3f"}));
        const unsigned requested=std::stoul(mask,nullptr,16);
        for(unsigned flags:{0u,0x2000u,0x4000u,0x6000u,0xffff0000u}) {
            const unsigned before=flags|0x2f;
            const unsigned after=(before&~0x3fu)|(requested&0x3f);
            assert((after&~0x3fu)==(before&~0x3fu));
        }
    }
    assert(uke::links::ethernet_matches("eth-test","1000-full",all));
    assert(!uke::links::ethernet_matches("eth-test","100-full",all));
    assert(!uke::links::ethernet_matches("eth-test","auto",all));
    assert(uke::links::ethernet_matches("eth-test","auto",capabilities(full,full)));
    assert(!uke::links::ethernet_matches("eth-test","1000-full",capabilities(full,"\"1000baseT/Full\"",false)));
    refuses([&]{uke::links::ethernet_arguments("eth-test","1000-full",capabilities("\"100baseT/Full\""));});
    refuses([&]{uke::links::ethernet_arguments("wrong","auto",all);});
    refuses([&]{uke::links::ethernet_arguments("../eth-test","auto",all);});
    refuses([&]{uke::links::ethernet_arguments("eth-test","2000-full",all);});
    for(const auto* bad:{"[]","{}","warning\n[]","[{},{}]","[{\"ifname\":\"eth-test\"}]"})
        refuses([&]{uke::links::ethernet_arguments("eth-test","auto",bad);});
    refuses([&]{uke::links::ethernet_arguments("eth-test","auto",capabilities("\"2500baseT/Full\""));});
    char pattern[]="/tmp/uke-link-policy-XXXXXX";const auto root=fs::path(mkdtemp(pattern));
    try {
        auto controller=root/"sys/devices/platform/soc/a600000.ssusb";
        const auto driver=root/"sys/bus/platform/drivers/dwc3-qcom";fs::create_directories(driver);
        fs::create_directories(controller);fs::create_directory_symlink(driver,controller/"driver");
        auto usb=controller/"a600000.usb/xhci-hcd.0.auto/usb2/2-1";
        put(usb/"idVendor","0bda\n");put(usb/"idProduct","8153\n");put(usb/"devpath","1\n");
        fs::create_directories(root/"sys/bus/usb/devices");fs::create_directory_symlink(usb,root/"sys/bus/usb/devices/2-1");
        assert(uke::links::usb_device(root,"2-1")==usb);
        refuses([&]{uke::links::usb_device(root,"../../etc");});
        put(usb/"devpath","0\n");refuses([&]{uke::links::usb_device(root,"2-1");});put(usb/"devpath","1\n");
        auto iface=usb/"2-1:1.0";auto net=iface/"net/eth-test";
        fs::create_directories(net);fs::create_directory_symlink(iface,net/"device");
        fs::create_directories(root/"sys/bus/usb/drivers/r8152");
        fs::create_directory_symlink(root/"sys/bus/usb/drivers/r8152",iface/"driver");
        fs::create_directories(root/"sys/class/net");fs::create_directory_symlink(net,root/"sys/class/net/eth-test");
        assert(uke::links::ethernet_device(root,"eth-test")==net);
        put(usb/"idProduct","8156\n");refuses([&]{uke::links::ethernet_device(root,"eth-test");});
        fs::remove(controller/"driver");refuses([&]{uke::links::usb_device(root,"2-1");});
        const auto dp=root/"sys/bus/platform/devices/dp";put(dp/"uke_dp_lane_limit","4\n");
        const auto dpdriver=root/"sys/bus/platform/drivers/msm-dp-display";fs::create_directories(dpdriver);
        fs::create_directory_symlink(dpdriver,dp/"driver");fs::create_directory_symlink(dp,dpdriver/"dp");
        assert(uke::links::lane_controls(root)==std::vector<fs::path>{dp/"uke_dp_lane_limit"});
        fs::remove(dp/"uke_dp_lane_limit");fs::create_symlink("/etc/passwd",dp/"uke_dp_lane_limit");
        assert(uke::links::lane_controls(root).empty());
        fs::remove_all(root);
    }catch(...){fs::remove_all(root);throw;}
    std::cout<<"Link policy: interface/USB topology scope, sysfs containment, speed support, JSON errors and advertisement readback passed; host only\n";
}
