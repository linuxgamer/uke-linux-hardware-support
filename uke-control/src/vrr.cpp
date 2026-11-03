// SPDX-License-Identifier: MIT
#include "vrr.hpp"
#include "wake.hpp"
#include "display-device.hpp"
#include "sysfs.hpp"
#include <array>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace fs=std::filesystem;
namespace uke::vrr {
struct Fd {int value=-1;~Fd(){if(value>=0)close(value);}};
static void need(bool valid,const char* message) {if(!valid)throw std::runtime_error(message);}
static unsigned decimal(std::string_view value) {
    unsigned result{};const auto parsed=std::from_chars(value.data(),value.data()+value.size(),result);
    need(!value.empty()&&parsed.ec==std::errc{}&&parsed.ptr==value.data()+value.size(),"Expected an integer VRR minimum");
    return result;
}
unsigned minimum(std::string_view value) {
    const auto hz=decimal(value);
    need(hz>=30&&hz<144,"VRR minimum must be 30..143 Hz; the selected display mode supplies the upper limit");
    return hz;
}
std::string policy(unsigned hz) {
    (void)minimum(std::to_string(hz));
    return std::to_string(hz)+'\n';
}
static bool trusted(int fd,bool directory) {
    struct stat st{};
    return fstat(fd,&st)==0&&st.st_uid==0&&!(st.st_mode&0022)&&
        (directory?S_ISDIR(st.st_mode):S_ISREG(st.st_mode)&&st.st_nlink==1&&st.st_size>0&&st.st_size<=4);
}
static std::string bounded(int fd) {
    std::array<char,97> bytes{};size_t size=0;
    for(;;) {
        const auto received=read(fd,bytes.data()+size,bytes.size()-size);
        if(received<0&&errno==EINTR)continue;
        need(received>=0,"Cannot read VRR policy");
        if(!received)break;
        size+=size_t(received);need(size<bytes.size(),"VRR policy exceeds its bound");
    }
    return std::string(bytes.data(),size);
}
std::optional<unsigned> configured(const fs::path& directory) {
    Fd parent{open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
    need(parent.value>=0&&trusted(parent.value,true),"Untrusted VRR policy directory");
    Fd fd{openat(parent.value,policy_name,O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW)};
    if(fd.value<0&&errno==ENOENT)return std::nullopt;
    need(fd.value>=0&&trusted(fd.value,false),"Untrusted VRR policy file");
    const auto value=bounded(fd.value);
    need(value.ends_with('\n'),"Malformed managed VRR policy");
    return minimum(std::string_view(value).substr(0,value.size()-1));
}
static bool capable(int fd,uint32_t connector) {
    std::unique_ptr<drmModeObjectProperties,decltype(&drmModeFreeObjectProperties)> properties(
        drmModeObjectGetProperties(fd,connector,DRM_MODE_OBJECT_CONNECTOR),drmModeFreeObjectProperties);
    if(!properties)return false;
    for(uint32_t i=0;i<properties->count_props;i++) {
        std::unique_ptr<drmModePropertyRes,decltype(&drmModeFreeProperty)> property(
            drmModeGetProperty(fd,properties->props[i]),drmModeFreeProperty);
        if(property&&!strcmp(property->name,"vrr_capable"))return properties->prop_values[i]==1;
    }
    return false;
}
Driver driver() {
    Driver result;
    if(!wake::applicable()){result.reason="not_uke_sm7675";return result;}
    Fd parameter{open("/sys/module/msm/parameters/o82_vrr_test_min_hz",O_RDONLY|O_CLOEXEC|O_NOFOLLOW)};
    if(parameter.value<0){result.reason="driver_floor_unreadable_use_sudo";return result;}
    try {
        auto value=bounded(parameter.value);
        if(value.ends_with('\n'))value.pop_back();
        result.loaded=decimal(value);
        Fd api{open("/sys/module/msm/parameters/o82_vrr_floor_api",O_RDONLY|O_CLOEXEC|O_NOFOLLOW)};
        if(api.value<0||bounded(api.value)!="1\n"){result.reason="kernel_vrr_floor_api_unavailable";return result;}
        if(!fs::exists("/dev/dri")){result.reason="display_unavailable";return result;}
        bool found=false;
        for(const auto& entry:fs::directory_iterator("/dev/dri")) {
            const auto name=entry.path().filename().string();
            if(!name.starts_with("card")||name.size()==4||name.find_first_not_of("0123456789",4)!=name.npos)continue;
            Fd fd{open(entry.path().c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW)};
            struct stat st{};
            if(fd.value<0||fstat(fd.value,&st)||!S_ISCHR(st.st_mode)||st.st_uid||st.st_mode&0002)continue;
            std::unique_ptr<drmVersion,decltype(&drmFreeVersion)> version(drmGetVersion(fd.value),drmFreeVersion);
            if(!version||!version->name||std::string_view(version->name,version->name_len)!="msm-kms")continue;
            std::unique_ptr<drmModeRes,decltype(&drmModeFreeResources)> resources(
                drmModeGetResources(fd.value),drmModeFreeResources);
            const auto active=resources?uke::test::internal_crtc(fd.value,*resources):std::nullopt;
            if(!active)continue;
            std::unique_ptr<drmModeCrtc,decltype(&drmModeFreeCrtc)> crtc(drmModeGetCrtc(fd.value,active->id),drmModeFreeCrtc);
            if(!crtc||!crtc->mode_valid||!crtc->mode.htotal||!crtc->mode.vtotal)continue;
            const uint64_t divisor=uint64_t(crtc->mode.htotal)*crtc->mode.vtotal;
            const auto upper=unsigned((uint64_t(crtc->mode.clock)*1000+divisor/2)/divisor);
            for(int i=0;i<resources->count_connectors;i++) {
                std::unique_ptr<drmModeConnector,decltype(&drmModeFreeConnector)> connector(
                    drmModeGetConnectorCurrent(fd.value,resources->connectors[i]),drmModeFreeConnector);
                if(!connector||connector->connector_type!=DRM_MODE_CONNECTOR_DSI||connector->connection!=DRM_MODE_CONNECTED)continue;
                std::unique_ptr<drmModeEncoder,decltype(&drmModeFreeEncoder)> encoder(
                    drmModeGetEncoder(fd.value,connector->encoder_id),drmModeFreeEncoder);
                if(!encoder||encoder->crtc_id!=active->id||!capable(fd.value,connector->connector_id))continue;
                need(!found,"Ambiguous Uke VRR display");found=true;result.upper=upper;
            }
        }
        if(!found)result.reason="driver_vrr_range_unavailable";
        else if(result.upper<=30)result.reason="select_a_mode_above_30_hz";
    } catch(const std::exception&) {result.reason="display_state_unavailable";}
    return result;
}
void validate(unsigned hz,const Driver& current) {
    (void)minimum(std::to_string(hz));
    need(current.reason.empty()&&current.loaded.has_value(),"A supported active Uke VRR display is required; inspect display vrr-min status");
    need(hz<current.upper,"VRR minimum must be below the selected display mode's refresh rate");
}
std::string status() {
    const auto current=driver();const auto saved=configured();
    const auto number=[](std::optional<unsigned> value){return value?std::to_string(*value):"null";};
    return "{\"schema\":\"org.uke.control.vrr-min.v1\",\"available\":"+
        std::string(current.reason.empty()?"true":"false")+",\"loaded_minimum_hz\":"+number(current.loaded)+
        ",\"configured_minimum_hz\":"+number(saved)+",\"minimum_supported_hz\":30,\"maximum_minimum_hz\":"+
        (current.upper>30?std::to_string(current.upper-1):"null")+",\"reboot_required\":"+
        (saved&&current.loaded&&saved!=current.loaded?"true":"false")+",\"reason\":"+
        (current.reason.empty()?"null":sysfs::quote(current.reason))+"}\n";
}
}
