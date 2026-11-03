// SPDX-License-Identifier: MIT
#include "backend.hpp"
#include "usb-request.hpp"
#include "link-policy.hpp"
#include "rpm-api.hpp"
#include "sysfs.hpp"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <fcntl.h>
#include <linux/magic.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

namespace fs=std::filesystem;
namespace uke::usb {
std::string request_refusal(const RequestState& s,const std::string& role) {
    if(role!="host"&&role!="device")return "INVALID_DATA_ROLE";
    if(!s.profile)return "FULL_TYPEC_PROFILE_REQUIRED";
    if(!s.port||!s.role_switch)return "PORT_ASSOCIATION_AMBIGUOUS";
    if(!s.partner)return "NO_ATTACHED_PARTNER";
    if(s.typec_role==role&&s.controller_role==role)return {};
    if(s.storage)return "USB_STORAGE_ATTACHED";
    if(s.network)return "USB_NETWORK_IN_USE";
    if(s.display)return "EXTERNAL_DISPLAY_IN_USE";
    if(s.gadget)return "GADGET_REQUIRES_OWNED_TEARDOWN";
    if((s.typec_role!="host"&&s.typec_role!="device")||
       (s.controller_role!="host"&&s.controller_role!="device"&&s.controller_role!="none"))return "ROLE_STATE_UNAVAILABLE";
    return {};
}
char profile_pin(const std::string& assignments,const std::string& profile) {
    // shortcut: Uke DP has no MST implementation; enable this profile after that driver path exists.
    if(profile=="dual-display")throw std::runtime_error("MST_DRIVER_SUPPORT_UNAVAILABLE");
    if(profile!="usb3-display"&&profile!="display4")throw std::runtime_error("INVALID_DISPLAY_PROFILE");
    std::string supported;char selected=0;
    std::istringstream tokens(assignments);std::string token;
    while(tokens>>token) {
        const bool current=token.size()==3&&token.front()=='['&&token.back()==']';
        if(current)token=token.substr(1,1);
        if(token.size()!=1||token[0]<'A'||token[0]>'F'||supported.find(token[0])!=supported.npos||
           (current&&selected))throw std::runtime_error("INVALID_PIN_ASSIGNMENTS");
        supported+=token;if(current)selected=token[0];
    }
    if(!selected)throw std::runtime_error("ACTIVE_PIN_ASSIGNMENT_UNAVAILABLE");
    const auto desired=profile=="display4"?std::string("CE"):std::string("DF");
    if(desired.find(selected)!=desired.npos)return selected;
    for(char pin:desired)if(supported.find(pin)!=supported.npos)return pin;
    throw std::runtime_error("PROFILE_PIN_ASSIGNMENT_UNSUPPORTED");
}
namespace {
struct Fd {int n=-1;~Fd(){if(n>=0)close(n);} Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;explicit Fd(int number):n(number){}};
void need(bool value,const std::string& reason){if(!value)throw std::runtime_error(reason);}
std::string value(const fs::path& path) {
    auto out=uke::read(path.string(),4096);
    while(!out.empty()&&(out.back()=='\n'||out.back()=='\0'))out.pop_back();
    return out;
}
std::string selected(const std::string& v) {
    const auto begin=v.find('['),end=v.find(']');
    return begin!=std::string::npos&&end>begin?v.substr(begin+1,end-begin-1):v;
}
std::vector<fs::path> children(const fs::path& path) {
    std::vector<fs::path> out;std::error_code error;
    for(fs::directory_iterator i(path,error),end;!error&&i!=end;i.increment(error)) {
        need(out.size()<256,"CLASS_INVENTORY_TOO_LARGE");out.push_back(i->path());
    }
    if(error&&error!=std::errc::no_such_file_or_directory)throw std::runtime_error("CLASS_INVENTORY_FAILED");
    std::sort(out.begin(),out.end());return out;
}
bool port_name(const std::string& name) {
    return name.starts_with("port")&&name.size()>4&&name.find_first_not_of("0123456789",4)==std::string::npos;
}
bool usb_ancestor(fs::path path) {
    std::error_code error;path=fs::canonical(path,error);
    if(error)return false;
    while(path!="/"&&!path.empty()) {
        if(fs::exists(path/"idVendor"))return true;
        path=path.parent_path();
    }
    return false;
}
bool full_controller(fs::path path) {
    std::error_code error;path=fs::canonical(path,error);
    if(error)return false;
    while(path!="/"&&!path.empty()) {
        const auto compatible=uke::read((path/"of_node/compatible").string(),4096);
        if(compatible.find(std::string("qcom,sm7675-dwc3-typec\0",23))!=std::string::npos)return true;
        path=path.parent_path();
    }
    return false;
}
struct Graph {
    RequestState state;
    fs::path port,role,partner;
    dev_t partner_device{};ino_t partner_inode{};
};
Graph graph() {
    Graph g;
    const auto compatible=uke::read("/sys/firmware/devicetree/base/compatible",4096);
    g.state.profile=compatible.find(std::string("xiaomi,uke\0",11))!=std::string::npos&&
        compatible.find(std::string("qcom,sm7675\0",12))!=std::string::npos&&
        value("/sys/firmware/devicetree/base/chosen/senemos,usb-profile")=="typec-v1";
    auto ports=children("/sys/class/typec");
    std::erase_if(ports,[](const fs::path& p){return !port_name(p.filename().string());});
    const auto roles=children("/sys/class/usb_role");
    g.state.port=ports.size()==1;g.state.role_switch=roles.size()==1;
    if(g.state.port) {
        g.port=fs::canonical(ports.front());g.partner=g.port/(ports.front().filename().string()+"-partner");
        struct stat st{};g.state.partner=stat(g.partner.c_str(),&st)==0;
        g.partner_device=st.st_dev;g.partner_inode=st.st_ino;
        g.state.typec_role=selected(value(g.port/"data_role"));
    }
    if(g.state.role_switch) {
        g.role=fs::canonical(roles.front());
        // The full Uke profile admits only this controller, independently of class numbering.
        g.state.role_switch=full_controller(g.role);
        g.state.controller_role=value(g.role/"role");
    }
    for(const auto& p:children("/sys/class/block"))if(usb_ancestor(p))g.state.storage=true;
    for(const auto& p:children("/sys/class/net"))if(usb_ancestor(p/"device")) {
        const auto raw=value(p/"flags");unsigned flags{};
        std::string_view v=raw;if(v.starts_with("0x"))v.remove_prefix(2);
        const auto parsed=std::from_chars(v.data(),v.data()+v.size(),flags,16);
        if(parsed.ec!=std::errc{}||parsed.ptr!=v.data()+v.size()||(flags&1))g.state.network=true;
    }
    for(const auto& p:children("/sys/class/drm")) {
        const auto name=p.filename().string();
        if((name.find("-DP-")!=std::string::npos||name.find("-HDMI-A-")!=std::string::npos)&&value(p/"enabled")=="enabled")g.state.display=true;
    }
    for(const auto& p:children("/sys/class/udc")) {
        const auto function=value(p/"function");
        if(!function.empty()&&function!="unavailable")g.state.gadget=true;
    }
    return g;
}
bool same_partner(const Graph& a,const Graph& b) {
    return a.state.partner&&b.state.partner&&a.port==b.port&&a.role==b.role&&
        a.partner_device==b.partner_device&&a.partner_inode==b.partner_inode;
}
std::vector<fs::path> dp_controls(const fs::path& root) {
    std::vector<fs::path> out;
    for(const auto& path:sysfs::entries(root,root/"sys/bus/typec/devices")) {
        if(path.filename().string().find("-partner.")==std::string::npos)continue;
        const auto svid=sysfs::attribute(root,path/"svid");
        if(svid!="\"ff01\""&&svid!="\"0xff01\"")continue;
        if(sysfs::attribute(root,path/"active")!="\"yes\"")continue;
        const auto node=sysfs::resolved(root,path/"displayport/pin_assignment");
        if(node&&fs::is_regular_file(*node))out.push_back(*node);
    }
    return out;
}
std::string dp_link(const fs::path& root) {
    std::string result="null";unsigned count=0;
    for(const auto& path:sysfs::entries(root,root/"sys/bus/platform/drivers/msm-dp-display")) {
        if(sysfs::driver(root,path)!="\"msm-dp-display\"")continue;
        const auto file=sysfs::resolved(root,path/"uke_dp_stream_state");
        if(!file||!fs::is_regular_file(*file))continue;
        const auto raw=value(*file);
        std::istringstream fields(raw);std::string field;
        unsigned version=0,active=2,lanes=0,rate=0,bpp=0,seen=0;
        while(fields>>field) {
            const auto equals=field.find('=');if(equals==field.npos)continue;
            const auto key=field.substr(0,equals);unsigned* target=nullptr;unsigned bit=0;
            if(key=="version"){target=&version;bit=1;}
            else if(key=="active"){target=&active;bit=2;}
            else if(key=="lanes"){target=&lanes;bit=4;}
            else if(key=="rate_khz"){target=&rate;bit=8;}
            else if(key=="bpp"){target=&bpp;bit=16;}
            if(!target)continue;
            if(seen&bit)return "null";
            seen|=bit;
            const std::string_view input(field.data()+equals+1,field.size()-equals-1);
            const auto parsed=std::from_chars(input.data(),input.data()+input.size(),*target);
            if(parsed.ec!=std::errc{}||parsed.ptr!=input.data()+input.size())return "null";
        }
        if(seen!=31||version!=1||active!=1||(lanes!=1&&lanes!=2&&lanes!=4)||
           (rate!=162000&&rate!=270000&&rate!=540000&&rate!=810000)||
           (bpp!=18&&bpp!=24&&bpp!=30))continue;
        if(++count>1)return "null";
        result="{\"lanes\":"+std::to_string(lanes)+",\"rate_khz\":"+std::to_string(rate)+
            ",\"bpp\":"+std::to_string(bpp)+",\"coherence\":\"sequential_driver_cache\"}";
    }
    return result;
}
struct GadgetLock {
    Fd fd{-1};
    GadgetLock() {
        constexpr auto directory="/run/senemos-uke";
        if(mkdir(directory,0700)<0)need(errno==EEXIST,"GADGET_STATE_UNAVAILABLE");
        struct stat st{};need(lstat(directory,&st)==0&&S_ISDIR(st.st_mode)&&st.st_uid==0&&!(st.st_mode&0022),"UNTRUSTED_GADGET_STATE");
        fd.n=open("/run/senemos-uke/usb-gadget.lock",O_CREAT|O_RDWR|O_NOFOLLOW|O_CLOEXEC,0600);
        need(fd.n>=0&&fstat(fd.n,&st)==0&&S_ISREG(st.st_mode)&&st.st_uid==0&&!(st.st_mode&0022),"UNTRUSTED_GADGET_LOCK");
        acquire();
    }
    void acquire(){need(flock(fd.n,LOCK_EX|LOCK_NB)==0,"USB_OWNER_BUSY");}
    void release(){need(flock(fd.n,LOCK_UN)==0,"USB_LOCK_FAILED");}
};
struct Inhibit {
    static constexpr auto path="/run/senemos-uke/usb-gadget-inhibit";
    Fd fd{open(path,O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW|O_CLOEXEC,0600)};
    Inhibit(){need(fd.n>=0,"USB_TRANSITION_ALREADY_INHIBITED");}
    ~Inhibit() {
        if(fd.n<0)return;
        struct stat own{},live{};
        if(fstat(fd.n,&own)==0&&lstat(path,&live)==0&&own.st_dev==live.st_dev&&own.st_ino==live.st_ino)unlink(path);
    }
};
std::string escaped(const std::string& text) {
    std::string out;
    for(unsigned char c:text)if(c>=32&&c<127&&c!='"'&&c!='\\')out+=char(c);else out+='?';
    return out;
}
}
std::string display_profiles(const fs::path& requested_root) {
    const auto root=fs::canonical(requested_root);
    const auto nodes=dp_controls(root);
    std::string out="{\"schema\":\"org.uke.control.display.profiles.v1\",\"live\":";
    out+=root=="/"?"true":"false";
    out+=",\"selection_interface\":"+sysfs::quote(nodes.size()==1?"standard_typec_pin_assignment":
        nodes.empty()?"unavailable":"ambiguous");
    out+=",\"pin_assignments\":"+(nodes.size()==1?sysfs::attribute(root,nodes.front()):"null");
    out+=",\"observed_link\":"+dp_link(root);
    const auto limits=links::lane_controls(root);
    out+=",\"training_ceiling\":{\"available\":"+std::string(limits.size()==1?"true":"false")+
        ",\"values\":[1,2,4],\"current\":"+(limits.size()==1?sysfs::attribute(root,limits.front()):"null")+
        ",\"requires_disconnected\":true,\"changes_typec_pin_assignment\":false}";
    out+=",\"physical_validation\":\"NOT_ESTABLISHED\",\"persisted\":false";
    out+=",\"profiles\":[{\"id\":\"usb3-display\",\"requested_dp_lanes\":2,\"usb3_lanes_reserved\":true},";
    out+="{\"id\":\"display4\",\"requested_dp_lanes\":4,\"usb3_lanes_reserved\":false,";
    out+="\"usb2_fallback_verified\":false,\"ethernet_fallback_verified\":false},";
    out+="{\"id\":\"dual-display\",\"requested_dp_lanes\":4,\"requires_mst\":true,";
    out+="\"available\":false,\"reason\":\"MST_DRIVER_SUPPORT_UNAVAILABLE\"}]}\n";
    return out;
}
Result request_display_profile(const std::string& profile) {
    bool accepted=false,observed=false,noop=false;
    int status=69;std::string error,before,after;
    try {
        // Validate the requested profile before inspecting or changing devices.
        (void)profile_pin("[C] D E F",profile);
        need(geteuid()==0,"ROOT_REQUIRED");
        auto first=graph();
        need(first.state.profile,"FULL_TYPEC_PROFILE_REQUIRED");
        need(first.state.port&&first.state.role_switch,"PORT_ASSOCIATION_AMBIGUOUS");
        need(first.state.partner,"NO_ATTACHED_PARTNER");
        const auto nodes=dp_controls("/");
        need(nodes.size()==1,nodes.empty()?"DP_ALTMODE_CONTROL_UNAVAILABLE":"DP_ALTMODE_CONTROL_AMBIGUOUS");
        need(sysfs::beneath(first.partner,nodes.front()),"DP_PARTNER_ASSOCIATION_INVALID");
        before=value(nodes.front());
        const auto pin=profile_pin(before,profile);
        noop=before.find(std::string("[")+pin+"]")!=before.npos;
        if(noop){observed=true;status=0;after=before;}
        else {
            need(!first.state.storage,"USB_STORAGE_ATTACHED");
            need(!first.state.network,"USB_NETWORK_IN_USE");
            need(!first.state.gadget,"GADGET_REQUIRES_OWNED_TEARDOWN");
            const auto api=execute({"/usr/bin/rpm","-q","--whatprovides","--qf","[%{PROVIDENAME}\\t%{PROVIDEVERSION}\\n]","uke-usb-gadget-ownership-api"},5,4096);
            need(api.status==0&&rpm_api_matches(api.output,"uke-usb-gadget-ownership-api","2"),"GADGET_OWNERSHIP_API2_REQUIRED");
            GadgetLock lock;Inhibit inhibit;
            auto current=graph();
            need(same_partner(first,current),"ATTACHMENT_CHANGED");
            need(!current.state.storage&&!current.state.network&&!current.state.gadget,"USB_RESOURCE_IN_USE");
            need(value(nodes.front())==before,"PIN_ASSIGNMENT_CHANGED");
            Fd fd{open(nodes.front().c_str(),O_WRONLY|O_CLOEXEC|O_NOFOLLOW)};
            struct statfs filesystem{};
            need(fd.n>=0&&fstatfs(fd.n,&filesystem)==0&&filesystem.f_type==SYSFS_MAGIC,"TYPEC_SYSFS_NOT_WRITABLE");
            const std::string data=std::string(1,pin)+'\n';
            const auto written=write(fd.n,data.data(),data.size());
            need(written==ssize_t(data.size()),written<0?"PIN_REQUEST_FAILED: "+std::string(strerror(errno)):"PIN_REQUEST_SHORT_WRITE");
            accepted=true;
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
            unsigned matches=0;
            while(std::chrono::steady_clock::now()<deadline) {
                current=graph();need(same_partner(first,current),"ATTACHMENT_CHANGED");
                after=value(nodes.front());
                if(after.find(std::string("[")+pin+"]")!=after.npos)++matches;else matches=0;
                if(matches==3){observed=true;status=0;break;}
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if(!observed){error="PIN_CONVERGENCE_TIMEOUT";status=124;}
        }
    }catch(const std::exception& e){error=e.what();}
    const auto boolean=[](bool b){return b?"true":"false";};
    return {status,"{\"schema\":\"org.uke.control.display.request.v1\",\"requested_profile\":"+sysfs::quote(profile)+
        ",\"write_accepted\":"+boolean(accepted)+",\"observed_pin_match\":"+boolean(observed)+
        ",\"already_matching\":"+boolean(noop)+",\"before_pin_assignments\":"+sysfs::quote(before)+
        ",\"after_pin_assignments\":"+sysfs::quote(after)+",\"negotiated_dp_lanes\":null,"+
        "\"physical_validation\":\"NOT_ESTABLISHED\",\"persisted\":false,\"error\":"+
        (error.empty()?"null":sysfs::quote(error))+"}\n"};
}
Result request_data(const std::string& desired) {
    bool accepted=false,observed=false,noop=false;
    std::string reason,before_typec,before_controller,after_typec,after_controller;
    int status=69;
    try {
        need(geteuid()==0,"ROOT_REQUIRED");
        auto first=graph();
        before_typec=first.state.typec_role;before_controller=first.state.controller_role;
        auto plan=first.state;plan.gadget=false;
        need(request_refusal(plan,desired).empty(),request_refusal(plan,desired));
        noop=first.state.typec_role==desired&&first.state.controller_role==desired;
        if(noop) {observed=true;status=0;after_typec=before_typec;after_controller=before_controller;}
        else {
            const auto api=execute({"/usr/bin/rpm","-q","--whatprovides","--qf","[%{PROVIDENAME}\\t%{PROVIDEVERSION}\\n]","uke-usb-gadget-ownership-api"},5,4096);
            need(api.status==0&&rpm_api_matches(api.output,"uke-usb-gadget-ownership-api","2"),"GADGET_OWNERSHIP_API2_REQUIRED");
            GadgetLock lock;Inhibit inhibit;
            auto current=graph();need(same_partner(first,current),"ATTACHMENT_CHANGED");
            if(current.state.gadget) {
                need(value("/run/senemos-uke/usb-gadget-profile")=="auto-acm","FOREIGN_OR_LEGACY_GADGET_BOUND");
                lock.release();
                const auto cleanup=execute({"/usr/libexec/senemos-uke/uke-usb-network","stop-auto"},10,4096);
                lock.acquire();need(cleanup.status==0,"OWNED_GADGET_TEARDOWN_FAILED");
            }
            current=graph();need(same_partner(first,current),"ATTACHMENT_CHANGED");
            need(request_refusal(current.state,desired).empty(),request_refusal(current.state,desired));
            Fd fd{open((current.port/"data_role").c_str(),O_WRONLY|O_CLOEXEC|O_NOFOLLOW)};
            struct statfs filesystem{};need(fd.n>=0&&fstatfs(fd.n,&filesystem)==0&&filesystem.f_type==SYSFS_MAGIC,"TYPEC_SYSFS_NOT_WRITABLE");
            const auto data=desired+'\n';
            const auto written=write(fd.n,data.data(),data.size());
            if(written!=ssize_t(data.size()))throw std::runtime_error("TYPEC_REQUEST_FAILED: "+(written<0?std::string(strerror(errno)):"short write"));
            accepted=true;
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
            unsigned matches=0;
            while(std::chrono::steady_clock::now()<deadline) {
                current=graph();after_typec=current.state.typec_role;after_controller=current.state.controller_role;
                need(same_partner(first,current),"ATTACHMENT_CHANGED");
                if(after_typec==desired&&after_controller==desired)++matches;else matches=0;
                if(matches==3){observed=true;status=0;break;}
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if(!observed){reason="ROLE_CONVERGENCE_TIMEOUT";status=124;}
        }
    }catch(const std::exception& e){reason=e.what();}
    const auto boolean=[](bool b){return b?"true":"false";};
    std::string out="{\"schema\":\"org.uke.control.usb.request.v1\",\"requested_data_role\":\""+escaped(desired)+'"';
    out+=",\"write_accepted\":"+std::string(boolean(accepted))+",\"observed_match\":"+boolean(observed)+",\"already_matching\":"+boolean(noop);
    out+=",\"before_typec\":\""+escaped(before_typec)+"\",\"before_controller\":\""+escaped(before_controller)+'"';
    out+=",\"after_typec\":\""+escaped(after_typec)+"\",\"after_controller\":\""+escaped(after_controller)+'"';
    out+=",\"error\":"+(reason.empty()?std::string("null"):"\""+escaped(reason)+"\"");
    out+=",\"physical_validation\":\"NOT_ESTABLISHED\",\"persisted\":false}\n";
    return {status,out};
}
}
