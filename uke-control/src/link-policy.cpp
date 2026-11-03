// SPDX-License-Identifier: MIT
#include "link-policy.hpp"
#include "sysfs.hpp"
#include <array>
#include <charconv>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <fcntl.h>
#include <linux/magic.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <systemd/sd-json.h>
#include <unistd.h>
namespace fs=std::filesystem;
namespace uke::links {
namespace {
void need(bool condition,const std::string& reason){if(!condition)throw std::runtime_error(reason);}
std::string value(const fs::path& path){auto v=uke::read(path,4096);while(!v.empty()&&(v.back()=='\n'||v.back()=='\0'))v.pop_back();return v;}
struct Fd{int n;~Fd(){if(n>=0)close(n);}};
using Json=std::unique_ptr<sd_json_variant,decltype(&sd_json_variant_unref)>;
Json json(const std::string& text) {
    sd_json_variant* v=nullptr;
    need(sd_json_parse(text.c_str(),SD_JSON_PARSE_MUST_BE_ARRAY,&v,nullptr,nullptr)>=0,"INVALID_ETHTOOL_JSON");
    return Json(v,sd_json_variant_unref);
}
bool named(const std::string& name) {
    return !name.empty()&&name.size()<64&&name!="."&&name!=".."&&
        name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.:")==name.npos;
}
bool controller(const fs::path& root,const fs::path& path) {
    for(auto p=path;p!=root&&!p.empty();p=p.parent_path())
        if(p.filename()=="a600000.ssusb"&&sysfs::driver(root,p)=="\"dwc3-qcom\"")return true;
    return false;
}
void write_attribute(const fs::path& path,const std::string& requested) {
    Fd fd{open(path.c_str(),O_RDWR|O_CLOEXEC|O_NOFOLLOW)};
    struct stat st{};struct statfs super{};
    need(fd.n>=0&&fstat(fd.n,&st)==0&&S_ISREG(st.st_mode)&&st.st_uid==0&&
        !(st.st_mode&0022)&&fstatfs(fd.n,&super)==0&&super.f_type==SYSFS_MAGIC,"UNTRUSTED_DRIVER_CONTROL");
    const auto input=requested+'\n';
    if(write(fd.n,input.data(),input.size())!=ssize_t(input.size()))
        throw std::runtime_error("DRIVER_WRITE_FAILED: "+std::string(strerror(errno)));
    need(lseek(fd.n,0,SEEK_SET)==0,"DRIVER_READBACK_FAILED");
    std::array<char,64> buffer{};const auto count=::read(fd.n,buffer.data(),buffer.size());
    need(count>0&&size_t(count)<buffer.size(),"DRIVER_READBACK_FAILED");
    std::string observed(buffer.data(),size_t(count));while(!observed.empty()&&observed.back()=='\n')observed.pop_back();
    need(observed==requested,"DRIVER_POLICY_MISMATCH");
}
std::string receipt(const std::string& kind,const std::string& requested) {
    return "{\"schema\":\"org.uke.control.link.request.v1\",\"control\":"+sysfs::quote(kind)+
        ",\"requested\":"+sysfs::quote(requested)+",\"driver_readback_matched\":true,\"persisted\":false,\"physical_validation\":\"NOT_ESTABLISHED\"}\n";
}
std::string ethtool() {
    for(const auto* p:{"/usr/sbin/ethtool","/usr/bin/ethtool"})if(access(p,X_OK)==0)return p;
    throw std::runtime_error("ETHTOOL_UNAVAILABLE");
}
Result query(const std::string& name,const std::string& operation) {
    if(operation=="link")return execute({ethtool(),"--json",name},5,256*1024);
    return execute({ethtool(),"--json",operation,name},5,256*1024);
}
unsigned mode_mask(sd_json_variant* modes) {
    need(sd_json_variant_is_array(modes),"LINK_CAPABILITIES_UNAVAILABLE");
    const std::array<std::string,6> known{"10baseT/Half","10baseT/Full","100baseT/Half","100baseT/Full","1000baseT/Half","1000baseT/Full"};
    unsigned mask=0;
    for(size_t i=0;i<sd_json_variant_elements(modes);++i) {
        const char* mode=sd_json_variant_string(sd_json_variant_by_index(modes,i));
        need(mode!=nullptr,"INVALID_LINK_MODE");bool found=false;
        for(size_t bit=0;bit<known.size();++bit)if(known[bit]==mode){mask|=1u<<bit;found=true;}
        need(found,"LINK_MODE_OUTSIDE_RTL8153_PROFILES");
    }
    return mask;
}
std::string mask_hex(unsigned mask) {
    std::array<char,16> hex{};const auto converted=std::to_chars(hex.data(),hex.data()+hex.size(),mask,16);
    return std::string(hex.data(),converted.ptr);
}
}
std::vector<fs::path> lane_controls(const fs::path& root) {
    std::vector<fs::path> out;
    for(const auto& p:sysfs::entries(root,root/"sys/bus/platform/drivers/msm-dp-display")) {
        if(sysfs::driver(root,p)!="\"msm-dp-display\"")continue;
        const auto control=sysfs::resolved(root,p/"uke_dp_lane_limit");
        if(control&&fs::is_regular_file(*control))out.push_back(*control);
    }
    return out;
}
fs::path usb_device(const fs::path& root,const std::string& name) {
    need(named(name)&&name.find(':')==name.npos,"INVALID_USB_DEVICE");
    const auto p=sysfs::resolved(root,root/"sys/bus/usb/devices"/name);
    need(p&&controller(root,*p)&&sysfs::exists(root,*p/"idVendor")&&
        sysfs::attribute(root,*p/"devpath")!="\"0\"","UKE_EXTERNAL_USB_DEVICE_REQUIRED");
    return *p;
}
fs::path ethernet_device(const fs::path& root,const std::string& name) {
    need(named(name)&&name.size()<16,"INVALID_NETWORK_INTERFACE");
    const auto p=sysfs::resolved(root,root/"sys/class/net"/name);
    need(p&&controller(root,*p)&&sysfs::driver(root,*p)=="\"r8152\"","UKE_R8152_INTERFACE_REQUIRED");
    auto device=sysfs::resolved(root,*p/"device");
    need(device.has_value(),"USB_NETWORK_DEVICE_MISSING");
    need(sysfs::attribute(root,device->parent_path()/"idVendor")=="\"0bda\""&&
        sysfs::attribute(root,device->parent_path()/"idProduct")=="\"8153\"","RTL8153_REQUIRED");
    return *p;
}
std::vector<std::string> ethernet_arguments(const std::string& name,const std::string& profile,const std::string& capabilities) {
    need(named(name)&&name.size()<16,"INVALID_NETWORK_INTERFACE");
    const auto data=json(capabilities);
    need(sd_json_variant_elements(data.get())==1,"AMBIGUOUS_ETHTOOL_INTERFACE");
    auto* object=sd_json_variant_by_index(data.get(),0);
    const char* ifname=sd_json_variant_string(sd_json_variant_by_key(object,"ifname"));
    need(ifname&&name==ifname,"ETHTOOL_INTERFACE_MISMATCH");
    auto* autoneg=sd_json_variant_by_key(object,"supports-auto-negotiation");
    need(sd_json_variant_is_boolean(autoneg)&&sd_json_variant_boolean(autoneg),"AUTONEGOTIATION_UNSUPPORTED");
    const unsigned available=mode_mask(sd_json_variant_by_key(object,"supported-link-modes"));
    unsigned mask=0;
    if(profile=="auto")mask=available;
    else if(profile=="1000-full")mask=32;
    else if(profile=="100-full")mask=8;
    else if(profile=="10-full")mask=2;
    else throw std::runtime_error("INVALID_ETHERNET_PROFILE");
    need(mask&&(available&mask)==mask,"LINK_MODE_UNSUPPORTED");
    // Mask only RTL8153 speed bits; retain pause, port and other advertisements.
    return {"--change",name,"autoneg","on","advertise",mask_hex(mask)+"/3f"};
}
bool ethernet_matches(const std::string& name,const std::string& profile,const std::string& capabilities) {
    const auto wanted=ethernet_arguments(name,profile,capabilities);
    const auto data=json(capabilities);
    auto* object=sd_json_variant_by_index(data.get(),0);
    auto* enabled=sd_json_variant_by_key(object,"auto-negotiation");
    if(!sd_json_variant_is_boolean(enabled)||!sd_json_variant_boolean(enabled))return false;
    try{return mask_hex(mode_mask(sd_json_variant_by_key(object,"advertised-link-modes")))+"/3f"==wanted.back();}
    catch(const std::exception&){return false;}
}
std::string ethernet_status(const std::string& name) {
    const auto path=ethernet_device("/",name);
    std::string out="{\"schema\":\"org.uke.control.ethernet.v1\",\"interface\":"+sysfs::quote(name);
    out+=",\"cached\":"+sysfs::object("/",path,{"operstate","carrier","speed","duplex","statistics/rx_errors","statistics/tx_errors","statistics/rx_dropped","statistics/tx_dropped"});
    for(const auto& [key,operation]:std::array<std::pair<const char*,const char*>,7>{{
        {"link","link"},{"features","--show-features"},{"eee","--show-eee"},
        {"pause","--show-pause"},{"rings","--show-ring"},{"coalesce","--show-coalesce"},{"channels","--show-channels"}}}) {
        const auto result=query(name,operation);bool valid=false;
        if(result.status==0){try{(void)json(result.output);valid=true;}catch(const std::exception&){}}
        out+=','+sysfs::quote(key)+":{\"available\":"+(valid?"true":"false")+",\"status\":"+std::to_string(result.status)+
            (valid?",\"data\":"+result.output:",\"detail\":"+sysfs::quote(result.output))+'}';
    }
    return out+",\"profiles\":[\"auto\",\"1000-full\",\"100-full\",\"10-full\"],\"physical_validation\":\"NOT_ESTABLISHED\"}\n";
}
Result lane_limit(const std::string& lanes) {
    need(lanes=="1"||lanes=="2"||lanes=="4","DP_LANE_LIMIT_MUST_BE_1_2_OR_4");
    const auto nodes=lane_controls("/");need(nodes.size()==1,"DP_LANE_LIMIT_UNAVAILABLE_OR_AMBIGUOUS");
    write_attribute(nodes.front(),lanes);
    return {0,receipt("dp_lane_ceiling",lanes)};
}
Result usb_power(const std::string& name,const std::string& policy) {
    need(policy=="performance"||policy=="auto","USB_POWER_MUST_BE_PERFORMANCE_OR_AUTO");
    write_attribute(usb_device("/",name)/"power/control",policy=="performance"?"on":"auto");
    return {0,receipt("usb_runtime_power",policy)};
}
Result ethernet_profile(const std::string& name,const std::string& profile) {
    const auto device=ethernet_device("/",name);
    // Link re-advertisement may disconnect SSH; require administrative shutdown first.
    const auto down=[&] {
        unsigned flags{};auto raw=value(device/"flags");if(raw.starts_with("0x"))raw.erase(0,2);
        const auto parsed=std::from_chars(raw.data(),raw.data()+raw.size(),flags,16);
        need(parsed.ec==std::errc{}&&parsed.ptr==raw.data()+raw.size()&&!(flags&1),"ETHERNET_MUST_BE_ADMINISTRATIVELY_DOWN");
    };
    down();const auto before=query(name,"link");need(before.status==0,"ETHERNET_CAPABILITIES_UNAVAILABLE");
    auto args=ethernet_arguments(name,profile,before.output);args.insert(args.begin(),ethtool());
    need(ethernet_device("/",name)==device,"ETHERNET_DEVICE_CHANGED");down();
    auto result=execute(args,5,256*1024);if(result.status!=0)return result;
    const auto after=query(name,"link");need(after.status==0,"ETHERNET_READBACK_FAILED");
    need(ethernet_device("/",name)==device&&ethernet_matches(name,profile,after.output),"ETHERNET_ADVERTISEMENT_MISMATCH");
    return {0,receipt("ethernet_advertisement",profile)};
}
}
