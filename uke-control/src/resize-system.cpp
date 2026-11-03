// SPDX-License-Identifier: MIT
// Uses the existing kernel recipe's F2FS geometry contract and Fedora's offline tools.
#include "../shared/userdata-grow.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/magic.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <unistd.h>
namespace fs=std::filesystem;
namespace uke::resize {
static void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Fd {int value;~Fd(){if(value>=0)close(value);}Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;explicit Fd(int n):value(n){need(n>=0,"Cannot open resize descriptor");}};
static bool present(const fs::path& path){struct stat st{};if(lstat(path.c_str(),&st)==0)return true;need(errno==ENOENT,"Cannot inspect resize path");return false;}
static std::string read(const fs::path& path,std::size_t limit=8192) {
    std::ifstream input(path);need(bool(input),"Required resize observation is unavailable");
    std::string value;char c;while(value.size()<limit&&input.get(c))value+=c;
    need(!input.get(c)&&!input.bad(),"Resize input is oversized or unreadable");return value;
}
static void save(const fs::path& path,const std::string& text,mode_t mode=0600) {
    Fd fd(open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,mode));
    need(fchmod(fd.value,mode)==0,"Cannot protect resize file");
    std::size_t done=0;while(done<text.size()){const auto n=write(fd.value,text.data()+done,text.size()-done);if(n<0&&errno==EINTR)continue;need(n>0,"Resize file write failed");done+=std::size_t(n);}
    need(fsync(fd.value)==0,"Cannot synchronize resize file");
}
static std::string private_request(const fs::path& path) {
    Fd fd(open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK));struct stat st{};
    need(fstat(fd.value,&st)==0&&S_ISREG(st.st_mode)&&st.st_uid==0&&st.st_nlink==1&&(st.st_mode&0777)==0600&&st.st_size>0&&st.st_size<=512,"Untrusted resize request");
    std::array<char,513> bytes{};const auto count=::read(fd.value,bytes.data(),bytes.size());
    need(count==st.st_size,"Incomplete resize request");return std::string(bytes.data(),std::size_t(count));
}
static void trusted_directory(const fs::path& path) {
    need(path.is_absolute()&&path==path.lexically_normal(),"Absolute normalized directory required");
    fs::path current="/";
    for(const auto& part:path.relative_path()) {
        current/=part;struct stat st{};
        need(lstat(current.c_str(),&st)==0&&S_ISDIR(st.st_mode)&&st.st_uid==0&&!(st.st_mode&0022),"Untrusted resize directory");
    }
}
static void run(const std::vector<std::string>& args,const fs::path& cwd={}) {
    need(!args.empty()&&args[0].starts_with("/usr/"),"Absolute distribution executable required");
    const auto pid=fork();need(pid>=0,"Cannot start resize tool");
    if(pid==0) {
        if(!cwd.empty()&&chdir(cwd.c_str())!=0)_exit(126);
        const int input=open("/dev/null",O_RDONLY);if(input<0||dup2(input,STDIN_FILENO)<0)_exit(126);
        setenv("LC_ALL","C",1);
        std::vector<char*> argv;for(const auto& arg:args)argv.push_back(const_cast<char*>(arg.c_str()));argv.push_back(nullptr);
        execv(args[0].c_str(),argv.data());_exit(127);
    }
    int status{};pid_t waited;do{waited=waitpid(pid,&status,0);}while(waited<0&&errno==EINTR);
    need(waited==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"Resize tool failed; no automatic retry is performed");
}
static bool compatible(const std::string& text) {
    bool uke=false,soc=false;
    for(std::size_t at=0;at<text.size();) {
        const auto end=text.find('\0',at);if(end==std::string::npos)return false;
        const auto item=std::string_view(text).substr(at,end-at);uke|=item=="xiaomi,uke";soc|=item=="qcom,sm7675";at=end+1;
    }
    return uke&&soc;
}
static std::uint64_t number(const std::string& value) {
    need(!value.empty()&&value.size()<=20&&value.find_first_not_of("0123456789")==std::string::npos,"Malformed resize number");
    std::size_t used=0;const auto result=std::stoull(value,&used);need(used==value.size(),"Invalid resize number");return result;
}
static std::uint64_t uptime(){const auto text=read("/proc/uptime");return number(text.substr(0,text.find('.')));}
struct Request {std::string device,uuid,boot;std::uint64_t bytes,before,prepared;};
static std::string encode(const Request& request) {
    return "format=1\ndevice="+request.device+"\nuuid="+request.uuid+"\nboot="+request.boot+"\nbytes="+std::to_string(request.bytes)+"\nbefore="+std::to_string(request.before)+"\nprepared="+std::to_string(request.prepared)+"\n";
}
static Request decode(const std::string& text) {
    need(text.size()<=512,"Resize request exceeds its size bound");std::map<std::string,std::string> fields;
    std::istringstream lines(text);std::string line;
    while(std::getline(lines,line)){const auto eq=line.find('=');need(eq!=std::string::npos&&fields.emplace(line.substr(0,eq),line.substr(eq+1)).second,"Malformed or duplicate resize request field");}
    need(fields.size()==7&&fields.at("format")=="1","Unknown resize request format");
    Request value{fields.at("device"),fields.at("uuid"),fields.at("boot"),number(fields.at("bytes")),number(fields.at("before")),number(fields.at("prepared"))};
    const auto colon=value.device.find(':');need(colon!=std::string::npos&&value.device.find(':',colon+1)==std::string::npos,"Invalid device number");
    need(number(value.device.substr(0,colon))>0&&number(value.device.substr(colon+1))>0,"Physical partition required");
    need(value.uuid.size()==32&&value.uuid.find_first_not_of("0123456789abcdef")==std::string::npos,"Invalid filesystem UUID");
    need(value.boot.size()==36&&value.boot.find_first_not_of("0123456789abcdef-")==std::string::npos,"Invalid boot identity");
    need(value.before>=128*1024*1024&&value.before<=value.bytes&&value.bytes%4096==0,"Invalid requested geometry");
    return value;
}
static void unmounted(const std::string& table,const std::string& number) {
    need(!table.empty(),"Mount table unavailable");std::istringstream lines(table);std::string line;
    while(std::getline(lines,line)){std::istringstream fields(line);std::string id,parent,device;need(bool(fields>>id>>parent>>device),"Malformed mount table");need(device!=number,"Userdata is still mounted; offline resize refused");}
}
static void root_device(const std::string& table,const std::string& number) {
    std::istringstream lines(table);std::string line;unsigned found=0;
    while(std::getline(lines,line)) {
        std::istringstream fields(line);std::string id,parent,device,root,mount,flags,field,filesystem;
        need(bool(fields>>id>>parent>>device>>root>>mount>>flags),"Malformed mount table");if(mount!="/")continue;
        while(fields>>field&&field!="-"){}
        need(root=="/"&&device==number&&field=="-"&&bool(fields>>filesystem)&&filesystem=="f2fs","Only the full physical F2FS userdata root is supported");
        need((","+flags+",").find(",rw,")!=std::string::npos,"Writable root required");++found;
    }
    need(found==1,"Ambiguous root mount");
}
static std::string identity(const userdata::Header& header) {
    constexpr char hex[]="0123456789abcdef";std::string out;
    for(unsigned i=108;i<124;++i){out+=hex[header[i]>>4];out+=hex[header[i]&15];}return out;
}
static userdata::Header header(int fd,off_t offset) {
    userdata::Header out{};need(pread(fd,out.data(),out.size(),offset)==ssize_t(out.size()),"Cannot read F2FS superblock");return out;
}
static std::string boot_id(){auto text=read("/proc/sys/kernel/random/boot_id");if(!text.empty()&&text.back()=='\n')text.pop_back();return text;}
static void power(const fs::path& supplies="/sys/class/power_supply") {
    bool external=false;int capacity=-1;
    for(const auto& entry:fs::directory_iterator(supplies)) {
        const auto node=entry.path();
        if(fs::exists(node/"online")&&read(node/"online")=="1\n")external=true;
        if(fs::exists(node/"type")&&read(node/"type")=="Battery\n"&&fs::exists(node/"capacity")) {
            auto text=read(node/"capacity");if(!text.empty()&&text.back()=='\n')text.pop_back();const auto value=number(text);
            need(value<=100&&capacity<0,"Invalid or ambiguous battery capacity");capacity=int(value);
        }
    }
    need(capacity>=50||(capacity>=30&&external),"Battery must be at least 50%, or 30% with external power");
}
static void request_matches(const Request& saved,const Request& actual,std::uint64_t now) {
    need(saved.device==actual.device&&saved.uuid==actual.uuid&&saved.boot==actual.boot&&saved.bytes==actual.bytes&&saved.before==actual.before,"Userdata identity or geometry changed after preparation");
    need(now>=saved.prepared&&now-saved.prepared<=600,"Resize preparation expired; run uke-control resize system immediately before reboot");
}
static bool dracut_device(const fs::path& relative,const struct stat& st) {
    if(!S_ISCHR(st.st_mode)||st.st_uid!=0||st.st_nlink!=1)return false;
    const struct {const char* path;unsigned major,minor;} devices[]={
        {"dev/null",1,3},{"dev/random",1,8},{"dev/urandom",1,9},{"dev/kmsg",1,11},{"dev/console",5,1}
    };
    for(const auto& device:devices)if(relative==device.path&&st.st_rdev==makedev(device.major,device.minor))return true;
    return false;
}
static void audit(const fs::path& root) {
    std::uintmax_t total=0;
    for(const auto& entry:fs::recursive_directory_iterator(root)) {
        const auto name=entry.path().filename().string();
        need(name.find("python")==std::string::npos&&name.find("pypy")==std::string::npos&&!entry.path().string().ends_with(".py")&&!entry.path().string().ends_with(".pyc"),"Python is not admitted in resize RAM environment");
        const auto status=entry.symlink_status();
        if(!fs::is_regular_file(status)&&!fs::is_directory(status)&&!fs::is_symlink(status)) {
            struct stat st{};
            need(lstat(entry.path().c_str(),&st)==0&&dracut_device(entry.path().lexically_relative(root),st),"Unexpected special file in resize RAM environment");
        }
        if(fs::is_regular_file(status))total+=entry.file_size();
        need(total<=128*1024*1024,"Resize RAM environment exceeds 128 MiB");
    }
    for(const char* file:{"shutdown","bin/sh","usr/libexec/uke-control/resize-system","usr/sbin/fsck.f2fs","usr/sbin/resize.f2fs"})need(access((root/file).c_str(),X_OK)==0,"Incomplete resize RAM environment");
    need(fs::is_regular_file(root/"usr/lib/initrd-release"),"Missing shutdown initrd identity");
    need(!fs::exists(root/"etc/shadow"),"Private account data in resize RAM environment");
}
static void build_environment(const fs::path& stage,const Request& request) {
    fs::create_directory(stage/"empty");fs::create_directory(stage/"tree");
    save(stage/"request",encode(request));
    save(stage/"hook","#!/bin/sh\n# One attempt only, after original dracut has unmounted oldroot.\n/usr/libexec/uke-control/resize-system --offline || warn 'Uke filesystem resize failed; no automatic retry'\ntrue\n",0755);
    run({"/usr/bin/dracut","--conf","/dev/null","--confdir",(stage/"empty").string(),"--tmpdir",stage.string(),"--no-kernel","--no-hostonly","--no-hostonly-cmdline","--no-hostonly-default-device","--no-early-microcode","--no-compress","--modules","base shutdown","--install","/usr/libexec/uke-control/resize-system /usr/sbin/fsck.f2fs /usr/sbin/resize.f2fs","--include",(stage/"request").string(),"/etc/uke-resize-system.request","--include",(stage/"hook").string(),"/var/lib/dracut/hooks/shutdown/20-uke-resize-system.sh",(stage/"resize.cpio").string()});
    run({"/usr/bin/lsinitrd","--unpack",(stage/"resize.cpio").string()},stage/"tree");
    audit(stage/"tree");
}
static void label_environment(const fs::path& root) {
    if(!fs::exists("/sys/fs/selinux/enforce"))return;
    std::istringstream config(read("/etc/selinux/config"));std::string line,type;
    while(std::getline(config,line))if(line.starts_with("SELINUXTYPE=")) {
        need(type.empty(),"Duplicate SELinux policy type");type=line.substr(12);
        if(type.size()>=2&&type.front()=='"'&&type.back()=='"')type=type.substr(1,type.size()-2);
    }
    need(!type.empty()&&type.size()<64&&type.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==std::string::npos,"Unknown SELinux policy type");
    run({"/usr/sbin/setfiles","-F","-r",root.string(),"/etc/selinux/"+type+"/contexts/files/file_contexts",root.string()});
}
static void activate(const fs::path& stage,const fs::path& target) {
    need(!present(target/"shutdown")&&!present(target/"bin/sh")&&!present(target/".need_shutdown"),"An existing shutdown RAM environment is preserved; manual review is required");
    fs::rename(target,stage/"previous");
    try {fs::rename(stage/"tree",target);}
    catch(...){fs::rename(stage/"previous",target);throw;}
}
static void expand(int fd,const std::string& device,std::uint64_t bytes) {
    const auto first=header(fd,1024),second=header(fd,5120);
    const auto before=userdata::filesystem_bytes(first,second,bytes);
    need(userdata::needs_growth(first,second,bytes),"Filesystem already fills the partition");
    std::cout<<"UKE_RESIZE_SYSTEM: offline F2FS expansion starting; keep power connected\n"<<std::flush;
    run({"/usr/sbin/fsck.f2fs","-f",device});
    need(header(fd,1024)==first&&header(fd,5120)==first,"Superblock changed during preflight; resize stopped");
    run({"/usr/sbin/resize.f2fs",device});
    run({"/usr/sbin/fsck.f2fs","-f",device});
    const auto after=header(fd,1024),copy=header(fd,5120);
    need(!userdata::needs_growth(after,copy,bytes)&&userdata::filesystem_bytes(after,copy,bytes)>before,"Filesystem did not expand to partition capacity");
    need(std::equal(first.begin()+108,first.end(),after.begin()+108),"Filesystem identity changed");
    need(fsync(fd)==0,"Cannot synchronize expansion");
    std::cout<<"UKE_RESIZE_SYSTEM: verified expansion from "<<before<<" to "<<userdata::filesystem_bytes(after,copy,bytes)<<" bytes; partition table unchanged\n";
}
static bool confirm_reboot(std::istream& input,bool terminal) {
    need(terminal,"Interactive terminal required; run sudo uke-control resize system in a terminal. Nothing was prepared");
    std::cout<<"Close applications and keep a backup and power connected.\n"
               "Prepare offline expansion and reboot now? Type yes to continue: "<<std::flush;
    std::string answer;
    return bool(std::getline(input,answer))&&answer=="yes";
}
int command(const std::string& action) {
    need(geteuid()==0,"Use sudo uke-control resize system");
    need(compatible(read("/sys/firmware/devicetree/base/compatible")),"Exact Uke/SM7675 identity required");
    struct stat self_ns{},init_ns{};
    need(stat("/proc/self/ns/mnt",&self_ns)==0&&stat("/proc/1/ns/mnt",&init_ns)==0&&self_ns.st_ino==init_ns.st_ino,"System mount namespace required");
    const auto device=fs::canonical("/dev/disk/by-partlabel/userdata");
    Fd fd(open(device.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));
    struct stat st{};std::uint64_t bytes=0;int readonly=1;
    need(fstat(fd.value,&st)==0&&S_ISBLK(st.st_mode)&&ioctl(fd.value,BLKGETSIZE64,&bytes)==0&&ioctl(fd.value,BLKROGET,&readonly)==0&&!readonly,"Writable physical userdata partition required");
    const auto number=std::to_string(major(st.st_rdev))+":"+std::to_string(minor(st.st_rdev));
    const auto sys=fs::canonical(fs::path("/sys/dev/block")/number);
    need(sys.string().find("/1d84000.ufshc/")!=std::string::npos&&sys.string().find("/virtual/")==std::string::npos&&fs::is_regular_file(sys/"partition")&&fs::is_empty(sys/"holders"),"Unheld Uke UFS partition required");
    need(("\n"+read(sys/"uevent")).find("\nPARTNAME=userdata\n")!=std::string::npos,"Partition label mismatch");
    const auto first=header(fd.value,1024),second=header(fd.value,5120);
    const auto before=userdata::filesystem_bytes(first,second,bytes);
    Request actual{number,identity(first),boot_id(),bytes,before,uptime()};
    if(action=="--offline") {
        struct statfs ram{};need(statfs("/",&ram)==0&&ram.f_type==TMPFS_MAGIC&&fs::exists("/usr/lib/initrd-release"),"Offline resize requires the shutdown RAM environment");
        const auto request=decode(private_request("/etc/uke-resize-system.request"));request_matches(request,actual,uptime());
        unmounted(read("/proc/self/mountinfo",1024*1024),number);
        // f2fs-tools also probes O_EXCL; release our probe before invoking it.
        {Fd exclusive(open(device.c_str(),O_RDWR|O_EXCL|O_NOFOLLOW|O_CLOEXEC));}
        expand(fd.value,device.string(),bytes);return 0;
    }
    root_device(read("/proc/self/mountinfo",1024*1024),number);
    std::cout<<"Filesystem bytes: "<<before<<"\nUserdata partition bytes: "<<bytes<<"\n";
    if(!userdata::needs_growth(first,second,bytes)){std::cout<<"Filesystem already fills userdata within F2FS section alignment.\n";return 0;}
    std::cout<<"Growth is required: F2FS must be unmounted. Expansion runs during normal reboot, before the next boot.\n";
    if(action=="--check") {
        std::cout<<"Check only; nothing was prepared. Run sudo uke-control resize system to confirm preparation and reboot.\n";return 0;
    }
    if(action=="--interactive") {
        power();
        if(!confirm_reboot(std::cin,isatty(STDIN_FILENO))) {
            std::cout<<"Cancelled; no new maintenance or reboot was requested.\n";return 0;
        }
        run({"/usr/bin/systemd-inhibit","--what=shutdown:sleep","--mode=block","--who=uke-control","--why=Prepare offline userdata filesystem expansion","/usr/libexec/uke-control/resize-system","--prepare"});
        request_matches(decode(private_request("/run/initramfs/etc/uke-resize-system.request")),actual,uptime());
        power();
        // Wait for preparation's inhibitor to exit before asking for normal reboot.
        std::cout<<"Preparation verified. Requesting normal reboot; keep power connected until checking and expansion finish.\n"<<std::flush;
        try {run({"/usr/bin/systemctl","--check-inhibitors=yes","reboot"});}
        catch(...) {
            std::cerr<<"Reboot was not accepted; it was not forced. RAM maintenance remains pending for ten minutes after preparation.\n"
                         "Resolve the reboot blocker and rerun sudo uke-control resize system.\n";throw;
        }
        return 0;
    }
    power();trusted_directory("/run");struct statfs runtime{};
    need(statfs("/run",&runtime)==0&&runtime.f_type==TMPFS_MAGIC&&!(runtime.f_flags&ST_NOEXEC),"Executable tmpfs at /run is required for offline maintenance");
    Fd lock(open("/run/uke-control-resize.lock",O_CREAT|O_RDWR|O_NOFOLLOW|O_CLOEXEC,0600));
    need(flock(lock.value,LOCK_EX|LOCK_NB)==0,"Another resize preparation is running");
    if(!present("/run/initramfs"))need(mkdir("/run/initramfs",0700)==0,"Cannot prepare shutdown directory");
    trusted_directory("/run/initramfs");
    if(present("/run/initramfs/etc/uke-resize-system.request")) {
        const fs::path path="/run/initramfs/etc/uke-resize-system.request";
        const auto pending=decode(private_request(path));request_matches(pending,actual,pending.prepared);
        actual.prepared=uptime();save(path.string()+".new",encode(actual));fs::rename(path.string()+".new",path);
        std::cout<<"Resize preparation renewed after the power check. Reboot normally within ten minutes.\n";return 0;
    }
    need(!present("/run/initramfs/shutdown")&&!present("/run/initramfs/bin/sh")&&!present("/run/initramfs/.need_shutdown"),"An existing shutdown RAM environment is preserved");
    char pattern[]="/run/uke-control-resize-XXXXXX";const auto made=mkdtemp(pattern);need(made,"Cannot create private RAM staging directory");const fs::path stage=made;
    build_environment(stage,actual);
    power();actual.prepared=uptime();
    // Building may take time; the power check and ten-minute lease start at publication.
    fs::remove(stage/"tree/etc/uke-resize-system.request");save(stage/"tree/etc/uke-resize-system.request",encode(actual));
    label_environment(stage/"tree");
    activate(stage,"/run/initramfs");
    std::cout<<"Offline resize prepared in RAM. No filesystem, GPT or boot partition was changed.\nA normal reboot within ten minutes will run checking and expansion before the next boot.\nDo not interrupt power during fsck/resize. After boot, verify with: sudo uke-control resize system --check\n";
    return 0;
}
}
int main(int argc,char** argv) {
    try {if(argc!=2||(std::string(argv[1])!="--check"&&std::string(argv[1])!="--interactive"&&std::string(argv[1])!="--prepare"&&std::string(argv[1])!="--offline"))throw std::runtime_error("Internal resize helper: expected --check, --interactive, --prepare or --offline");return uke::resize::command(argv[1]);}
    catch(const std::exception& error){std::cerr<<"UKE_RESIZE_SYSTEM_REFUSED: "<<error.what()<<'\n';return 1;}
}
