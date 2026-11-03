// SPDX-License-Identifier: MIT
#include "diagnostic.hpp"
#include "backend.hpp"
#include "sysfs.hpp"
#include <chrono>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <glob.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

namespace uke::diagnostic {
namespace {
const std::vector<std::string> topics{
    "system","hardware","kernel","boot","packages","services","wifi","bluetooth",
    "sensors","sleep","power","usb","display","audio","input","camera","storage","firmware","security"};
struct Job {std::string topic,label;std::vector<std::string> args;};
std::vector<Job> jobs(const std::string& topic) {
    std::vector<Job> result;
    const auto add=[&](const char* group,const char* label,std::vector<std::string> args) {
        if(topic=="all"||topic==group)result.push_back({group,label,std::move(args)});
    };
    add("system","CPU topology",{"/usr/bin/lscpu"});
    add("system","Memory",{"/usr/bin/free","-w"});
    add("system","Processes (no command arguments)",{"/usr/bin/ps","-e","-o","pid,ppid,comm,stat,etimes,pcpu,pmem,rss,wchan:32","--sort=-rss"});
    add("system","Resource sample",{"/usr/bin/vmstat","-w","1","3"});
    add("system","Control groups",{"/usr/bin/systemd-cgtop","--batch","--iterations=1","--depth=3"});
    add("hardware","All udev devices, paths and properties",{"/usr/bin/udevadm","info","--export-db"});
    add("hardware","All sysfs device paths",{"/usr/bin/find","/sys/devices","-type","d","-print"});
    add("hardware","Bus device and driver links",{"/usr/bin/find","/sys/bus","-maxdepth","4","-type","l","-printf","%p -> %l\n"});
    add("hardware","Device nodes (metadata only)",{"/usr/bin/ls","-lR","/dev"});
    add("hardware","PCI devices and drivers",{"/usr/bin/lspci","-nnk"});
    add("hardware","Cached Uke hardware paths",{"/usr/bin/uke-control","hardware","status"});
    add("kernel","Kernel build",{"/usr/bin/uname","-a"});
    add("kernel","Loaded modules",{"/usr/bin/lsmod"});
    add("kernel","Kernel ring buffer",{"/usr/bin/dmesg","--color=never"});
    add("kernel","Current kernel journal",{"/usr/bin/journalctl","-k","-b","--no-pager","-o","short-monotonic","-n","60000"});
    add("kernel","Previous kernel journal",{"/usr/bin/journalctl","-k","-b","-1","--no-pager","-n","30000"});
    add("kernel","Deferred driver probes",{"/usr/bin/cat","/sys/kernel/debug/devices_deferred"});
    add("kernel","Installed module trees",{"/usr/bin/find","/usr/lib/modules","-maxdepth","2","-printf","%y %p -> %l\n"});
    add("boot","Boot history",{"/usr/bin/journalctl","--list-boots","--no-pager"});
    add("boot","Current boot journal",{"/usr/bin/journalctl","-b","--no-pager","-o","short-monotonic","-n","100000"});
    add("boot","Previous boot journal",{"/usr/bin/journalctl","-b","-1","--no-pager","-n","50000"});
    add("boot","Boot time",{"/usr/bin/systemd-analyze","time"});
    add("boot","Boot ordering",{"/usr/bin/systemd-analyze","critical-chain","--no-pager"});
    add("boot","Service startup times",{"/usr/bin/systemd-analyze","blame","--no-pager"});
    add("boot","Boot artifacts (metadata only)",{"/usr/bin/find","/boot","-maxdepth","4","-printf","%y %m %s %p -> %l\n"});
    add("boot","EFI boot status",{"/usr/bin/bootctl","status","--no-pager"});
    add("boot","Uke boot identity",{"/usr/bin/uke-boot-status"});
    add("packages","Installed RPM versions, origins and signatures",{"/usr/bin/rpm","-qa","--qf","%{NEVRA}\t%{VENDOR}\t%{SOURCERPM}\t%{INSTALLTIME:date}\t%{RSAHEADER:pgpsig}\n"});
    add("packages","RPM database integrity (read only)",{"/usr/bin/rpmdb","--verifydb"});
    add("packages","Core RPM file verification",{"/usr/bin/rpm","-V","uke-control","uke-core","uke-hw","uke-boot-integration","uke-power","uke-sensors"});
    add("packages","DNF version",{"/usr/bin/dnf5","--version"});
    add("packages","Repositories (cached, no network refresh)",{"/usr/bin/dnf5","--cacheonly","repo","info","--all"});
    add("packages","DNF transaction history",{"/usr/bin/dnf5","--cacheonly","history","list"});
    add("packages","Last DNF transaction",{"/usr/bin/dnf5","--cacheonly","history","info","last"});
    add("packages","Installed dependency problems",{"/usr/bin/dnf5","--cacheonly","check"});
    add("packages","Duplicate RPMs",{"/usr/bin/dnf5","--cacheonly","repoquery","--installed","--duplicates"});
    add("packages","Packages absent from cached repositories",{"/usr/bin/dnf5","--cacheonly","repoquery","--extras"});
    add("packages","Updates in cached repositories",{"/usr/bin/dnf5","--cacheonly","repoquery","--upgrades"});
    add("packages","Retained kernel packages",{"/usr/bin/dnf5","--cacheonly","repoquery","--installed","--installonly"});
    add("services","Failed units",{"/usr/bin/systemctl","--failed","--all","--no-pager"});
    add("services","All units",{"/usr/bin/systemctl","list-units","--all","--no-pager"});
    add("services","Unit enablement",{"/usr/bin/systemctl","list-unit-files","--no-pager"});
    add("services","Unit overrides",{"/usr/bin/systemd-delta","--no-pager"});
    add("services","Warnings and errors this boot",{"/usr/bin/journalctl","-b","-p","warning","--no-pager","-n","30000"});
    add("services","Crash inventory (no core dumps)",{"/usr/bin/coredumpctl","list","--no-pager"});
    add("wifi","Network interfaces",{"/usr/bin/ip","-details","-statistics","link","show"});
    add("wifi","Addresses",{"/usr/bin/ip","address","show"});
    add("wifi","Routes",{"/usr/bin/ip","route","show","table","all"});
    add("wifi","IPv6 routes",{"/usr/bin/ip","-6","route","show","table","all"});
    add("wifi","Wireless PHY capabilities",{"/usr/sbin/iw","phy"});
    add("wifi","Wireless interfaces",{"/usr/sbin/iw","dev"});
    add("wifi","NetworkManager state (no connection secrets or scans)",{"/usr/bin/nmcli","--colors","no","general","status"});
    add("wifi","NetworkManager devices",{"/usr/bin/nmcli","--colors","no","device","status"});
    add("wifi","NetworkManager device properties",{"/usr/bin/nmcli","--colors","no","device","show"});
    add("wifi","DNS state",{"/usr/bin/resolvectl","status","--no-pager"});
    add("wifi","Radio blocks",{"/usr/sbin/rfkill","list"});
    add("bluetooth","Radio blocks",{"/usr/sbin/rfkill","list"});
    add("bluetooth","Existing BlueZ controller paths (no activation)",{"/usr/bin/busctl","--system","--auto-start=no","--no-pager","tree","org.bluez"});
    add("sensors","Exported sensor channels",{"/usr/bin/uke-control","sensors","list"});
    add("sensors","Existing SensorProxy interface (no activation)",{"/usr/bin/busctl","--system","--auto-start=no","--no-pager","introspect","net.hadess.SensorProxy","/net/hadess/SensorProxy","net.hadess.SensorProxy"});
    add("sensors","Front color snapshot (no acquisition)",{"/usr/bin/uke-control","display","sensor"});
    add("sleep","Wake permissions",{"/usr/bin/uke-control","wake","status"});
    add("sleep","Sleep inhibitors",{"/usr/bin/systemd-inhibit","--list","--no-pager"});
    add("sleep","Sleep configuration",{"/usr/bin/systemd-analyze","cat-config","systemd/sleep.conf"});
    add("sleep","Logind configuration",{"/usr/bin/systemd-analyze","cat-config","systemd/logind.conf"});
    add("sleep","Wakeup counters",{"/usr/bin/cat","/sys/kernel/debug/wakeup_sources"});
    add("sleep","Suspend counters",{"/usr/bin/find","/sys/power/suspend_stats","-maxdepth","1","-type","f","-print","-exec","/usr/bin/cat","{}",";"});
    add("power","Battery snapshot",{"/usr/bin/uke-battery"});
    add("power","Cached charging status",{"/usr/libexec/uke-charge-control","status"});
    add("power","Charging admission",{"/usr/libexec/uke-charge-control","admission"});
    add("power","Charging protocols",{"/usr/libexec/uke-charge-control","protocols"});
    add("power","Cached protection state",{"/usr/libexec/uke-charge-control","protection-status"});
    add("power","Power profiles",{"/usr/bin/powerprofilesctl","list"});
    add("power","Existing UPower interface (no activation)",{"/usr/bin/busctl","--system","--auto-start=no","--no-pager","introspect","org.freedesktop.UPower","/org/freedesktop/UPower","org.freedesktop.UPower"});
    add("usb","Cached USB/Type-C topology",{"/usr/bin/uke-control","usb","status"});
    add("usb","USB inventory",{"/usr/bin/lsusb"});
    add("usb","USB topology and drivers",{"/usr/bin/lsusb","-t"});
    add("display","Display and GPU driver modules",{"/usr/bin/modinfo","msm","drm","gpu_sched"});
    add("display","Loaded MSM module build ID",{"/usr/bin/od","-An","-v","-tx1","/sys/module/msm/notes/.note.gnu.build-id"});
    add("display","Loaded O82 panel module build ID",{"/usr/bin/od","-An","-v","-tx1","/sys/module/panel_xiaomi_o82/notes/.note.gnu.build-id"});
    add("display","Loaded THP module build ID",{"/usr/bin/od","-An","-v","-tx1","/sys/module/nt36532_uke_thp/notes/.note.gnu.build-id"});
    add("audio","ALSA cards",{"/usr/bin/cat","/proc/asound/cards","/proc/asound/pcm","/proc/asound/devices"});
    add("audio","PipeWire session (current user)",{"/usr/bin/wpctl","status"});
    add("input","Input devices and paths",{"/usr/bin/cat","/proc/bus/input/devices","/proc/bus/input/handlers"});
    add("input","Input node aliases",{"/usr/bin/ls","-lR","/dev/input"});
    add("camera","Video/media nodes (metadata only)",{"/usr/bin/find","/sys/class/video4linux","/sys/class/media","-maxdepth","2","-printf","%y %p -> %l\n"});
    add("storage","Block topology and labels",{"/usr/bin/lsblk","--bytes","--output","NAME,KNAME,PATH,TYPE,SIZE,RO,FSTYPE,FSVER,LABEL,UUID,PARTLABEL,PARTUUID,MOUNTPOINTS,MODEL,TRAN"});
    add("storage","Mounts",{"/usr/bin/findmnt","--all"});
    add("storage","Fstab validation",{"/usr/bin/findmnt","--verify","--verbose"});
    add("storage","Filesystem capacity",{"/usr/bin/df","-hT"});
    add("storage","Inode capacity",{"/usr/bin/df","-i"});
    add("storage","Active swap",{"/usr/sbin/swapon","--show","--bytes"});
    add("storage","Open device users (metadata only)",{"/usr/bin/lslocks"});
    add("firmware","Cached remote processors and SCMI",{"/usr/bin/uke-control","firmware","status"});
    add("firmware","Firmware files (metadata only)",{"/usr/bin/find","/usr/lib/firmware","-maxdepth","6","-printf","%y %s %p -> %l\n"});
    add("security","SELinux status",{"/usr/sbin/sestatus"});
    add("security","Recent SELinux denials",{"/usr/sbin/ausearch","-m","AVC,USER_AVC,SELINUX_ERR","-ts","boot"});
    add("security","Kernel lockdown",{"/usr/bin/cat","/sys/kernel/security/lockdown","/sys/kernel/security/lsm"});
    // Fixed patterns select status/configuration only; never sweep arbitrary sysfs attributes or private calibration.
    const auto files=[&](const char* group,std::initializer_list<const char*> patterns) {
        for(const char* pattern:patterns)add(group,pattern,{"@files",pattern});
    };
    files("system",{"/etc/os-release","/proc/cpuinfo","/proc/meminfo","/proc/uptime","/proc/pressure/*","/proc/vmstat","/proc/stat"});
    files("hardware",{"/sys/firmware/devicetree/base/model","/sys/firmware/devicetree/base/compatible","/sys/devices/soc0/{family,machine,soc_id,revision}"});
    files("kernel",{"/proc/version","/proc/cmdline","/proc/sys/kernel/tainted","/proc/interrupts","/proc/softirqs","/proc/iomem","/proc/ioports","/proc/modules","/boot/config-*","/etc/modprobe.d/*.conf","/etc/modules-load.d/*.conf"});
    files("boot",{"/proc/cmdline","/etc/fstab","/boot/loader/entries/*.conf","/etc/senemos/*update.conf","/var/lib/senemos/installation/profile","/var/lib/senemos/kernel-update/*.state","/var/lib/senemos/kernel-update/*.json","/var/lib/senemos/kernel-update/*.log"});
    files("packages",{"/etc/dnf/dnf.conf","/etc/dnf/vars/*","/etc/dnf/repos.override.d/*.repo","/etc/dnf/protected.d/*.conf","/etc/uke-control/*channel*","/var/lib/uke-control/*channel*","/var/log/dnf5*.log","/var/log/dnf.rpm.log"});
    files("wifi",{"/proc/net/wireless","/sys/class/net/*/uevent","/sys/class/net/*/operstate","/sys/class/net/*/device/uevent","/sys/class/net/*/device/power/runtime_status","/etc/NetworkManager/conf.d/*.conf"});
    files("bluetooth",{"/sys/class/bluetooth/*/uevent","/sys/class/bluetooth/*/device/uevent","/sys/class/bluetooth/*/device/power/wakeup","/sys/class/rfkill/*/uevent"});
    files("sensors",{"/sys/bus/iio/devices/iio:device*/name","/sys/bus/iio/devices/iio:device*/in_*_raw","/sys/bus/iio/devices/iio:device*/in_*_input","/sys/bus/iio/devices/iio:device*/in_*_scale","/sys/bus/iio/devices/iio:device*/in_*_offset","/sys/class/hwmon/hwmon*/name","/sys/class/hwmon/hwmon*/*_label","/sys/class/hwmon/hwmon*/*_input","/sys/class/thermal/thermal_zone*/type","/sys/class/thermal/thermal_zone*/temp"});
    files("sleep",{"/sys/power/state","/sys/power/mem_sleep","/sys/power/disk","/sys/power/wakeup_count","/sys/class/wakeup/*/name","/sys/class/wakeup/*/event_count","/sys/class/wakeup/*/wakeup_count","/etc/uke-control/wake*"});
    files("power",{"/sys/class/power_supply/*/uevent","/sys/class/thermal/thermal_zone*/type","/sys/class/thermal/thermal_zone*/temp","/sys/class/thermal/thermal_zone*/trip_point_*_temp","/sys/class/thermal/cooling_device*/type","/sys/class/thermal/cooling_device*/cur_state","/sys/devices/system/cpu/cpufreq/policy*/scaling_*_freq","/sys/devices/system/cpu/cpufreq/policy*/scaling_governor"});
    files("usb",{"/sys/class/typec/*/uevent","/sys/class/typec/*/data_role","/sys/class/typec/*/power_role","/sys/class/typec/*/port_type","/sys/class/usb_role/*/role","/sys/class/udc/*/state","/sys/bus/usb/devices/*/uevent","/sys/bus/usb/devices/*/speed","/sys/bus/usb/devices/*/power/runtime_status"});
    files("display",{"/sys/class/drm/*/status","/sys/class/drm/*/enabled","/sys/class/drm/*/modes","/sys/class/backlight/*/type","/sys/class/backlight/*/brightness","/sys/class/backlight/*/actual_brightness","/sys/class/backlight/*/max_brightness","/sys/class/devfreq/*/name","/sys/class/devfreq/*/governor","/sys/class/devfreq/*/available_frequencies"});
    files("display",{"/sys/module/msm/taint","/sys/module/panel_xiaomi_o82/taint","/sys/module/nt36532_uke_thp/taint"});
    files("audio",{"/sys/class/sound/*/id","/sys/bus/soundwire/devices/*/uevent","/sys/bus/aprbus/devices/*/uevent","/proc/asound/card*/codec*"});
    files("input",{"/sys/class/input/input*/name","/sys/class/input/input*/properties","/sys/class/input/input*/capabilities/*"});
    files("camera",{"/sys/class/video4linux/*/name","/sys/class/video4linux/*/uevent","/sys/class/media/*/uevent"});
    files("storage",{"/proc/partitions","/proc/mounts","/proc/swaps","/proc/diskstats","/etc/fstab","/sys/block/*/device/model","/sys/block/*/queue/scheduler","/sys/block/*/stat","/sys/fs/f2fs/*/status","/sys/fs/ext4/*/errors_count"});
    files("firmware",{"/sys/class/remoteproc/*/name","/sys/class/remoteproc/*/state","/sys/class/remoteproc/*/firmware","/sys/module/firmware_class/parameters/path"});
    files("security",{"/proc/sys/kernel/dmesg_restrict","/proc/sys/kernel/kptr_restrict","/proc/sys/kernel/perf_event_paranoid","/proc/sys/kernel/unprivileged_bpf_disabled"});
    const std::vector<std::pair<std::string,std::string>> units{
        {"wifi","NetworkManager.service"},{"wifi","wpa_supplicant.service"},{"wifi","*uke*wlan*"},{"wifi","*uke*wifi*"},
        {"bluetooth","bluetooth.service"},{"bluetooth","*uke*bluetooth*"},
        {"sensors","*ssc*"},{"sensors","*sensor*"},
        {"sleep","systemd-suspend*"},{"sleep","systemd-hibernate*"},{"sleep","systemd-logind.service"},{"sleep","uke-control-wake.service"},
        {"power","*uke*charge*"},{"power","*uke*battery*"},{"power","power-profiles-daemon.service"},{"power","upower.service"},
        {"usb","*uke*usb*"},{"display","*backlight*"},{"display","*display*"},
        {"audio","*audio*"},{"audio","*pipewire*"},{"audio","*wireplumber*"},
        {"input","*touch*"},{"camera","*camera*"},{"boot","uke*boot*"},{"firmware","*remoteproc*"}};
    for(const auto& [group,unit]:units) {
        add(group.c_str(),("Service state: "+unit).c_str(),{"/usr/bin/systemctl","status",unit,"--no-pager","--full","--lines=30"});
        add(group.c_str(),("Service journal: "+unit).c_str(),{"/usr/bin/journalctl","-b","-u",unit,"--no-pager","-n","10000"});
    }
    const std::vector<std::pair<std::string,std::string>> kernel_filters{
        {"wifi","wlan|wifi|ath1[012]k|wcn|cnss|cfg80211|mac80211|firmware"},
        {"bluetooth","bluetooth|hci[0-9]|btqca|btusb|qca_uart|wcn"},
        {"sensors","sensor|ssc|slpi|iio|fastrpc|adsp|remoteproc|qmi|pmic.*adc"},
        {"sleep","suspend|resume|wakeup|wake up|PM:|freez|sleep|hung task"},
        {"power","batt|charg|power_supply|thermal|overheat|pmic|scmi|regulator"},
        {"usb","usb|typec|ucsi|dwc3|xhci|altmode|displayport|ps883|fsa"},
        {"display","drm|dpu|dsi|panel|gpu|adreno|kgsl|gmu|smmu|backlight|nt365"},
        {"audio","sound|snd|audio|alsa|codec|wcd|swr|soundwire|gpr|adsp|q6|amp"},
        {"input","input|touch|nt365|nvt|pen|stylus|hall|hid|i2c"},
        {"camera","camera|camss|v4l2|video|media|cci|csid|sensor|isp"},
        {"storage","ufs|scsi|sd[a-z]|I/O error|f2fs|ext4|btrfs|nvme|mmc|corrupt"},
        {"firmware","firmware|remoteproc|scmi|adsp|cdsp|slpi|wpss|pas|q6v5"},
        {"security","avc:|denied|audit|lockdown|verification|signature|secure boot"}};
    for(const auto& [group,filter]:kernel_filters)
        add(group.c_str(),"Related kernel journal",{"/usr/bin/journalctl","-k","-b","--no-pager","--case-sensitive=no","--grep",filter,"-n","30000"});
    for(const auto& [group,paths]:std::vector<std::pair<std::string,std::vector<std::string>>>{
        {"wifi",{"/sys/class/net"}},{"bluetooth",{"/sys/class/bluetooth","/sys/class/rfkill"}},
        {"sensors",{"/sys/bus/iio/devices","/sys/class/hwmon","/sys/class/thermal"}},
        {"sleep",{"/sys/class/wakeup"}},{"power",{"/sys/class/power_supply"}},
        {"usb",{"/sys/class/typec","/sys/class/usb_role","/sys/class/udc"}},
        {"display",{"/sys/class/drm","/sys/class/backlight","/sys/class/devfreq"}},
        {"audio",{"/sys/class/sound","/sys/bus/soundwire/devices","/sys/bus/aprbus/devices"}},
        {"input",{"/sys/class/input"}},{"camera",{"/sys/class/video4linux","/sys/class/media"}},
        {"storage",{"/sys/class/block","/dev/disk/by-partlabel"}},{"firmware",{"/sys/class/remoteproc"}}}) {
        std::vector<std::string> args{"/usr/bin/ls","-l"};args.insert(args.end(),paths.begin(),paths.end());
        add(group.c_str(),"Subsystem device paths",std::move(args));
    }
    return result;
}
void write_text(int fd,const std::string& text) {
    std::size_t done=0;
    while(done<text.size()) {
        const auto n=::write(fd,text.data()+done,text.size()-done);
        if(n<0&&errno==EINTR)continue;
        if(n<=0)throw std::runtime_error("Diagnostic log write failed: "+std::string(strerror(errno)));
        done+=std::size_t(n);
    }
}
std::string text_output(const std::string& raw) {
    std::string out;out.reserve(raw.size());constexpr char hex[]="0123456789abcdef";
    for(unsigned char c:raw) {
        if((c<32&&c!='\n'&&c!='\t')||c==127){out+="\\x";out+=hex[c>>4];out+=hex[c&15];}
        else out+=char(c);
    }
    return out;
}
}
std::string collect(const std::string& topic) {
    if(topic!="all"&&std::find(topics.begin(),topics.end(),topic)==topics.end())
        throw std::runtime_error("Unknown diagnostic topic; use uke-control diagnostic --help");
    const std::string name="uke-diagnostic-"+topic+".log";
    const int fd=open(name.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
    if(fd<0)throw std::runtime_error("Cannot create "+name+": "+strerror(errno)+". Existing reports are never overwritten; move the old file before retrying.");
    struct Close {int fd;~Close(){close(fd);}} close_file{fd};
    if(fchmod(fd,0600)!=0)throw std::runtime_error("Cannot protect diagnostic log");
    const auto started=std::chrono::steady_clock::now();
    const auto now=std::time(nullptr);std::tm utc{};char date[32]{};
    gmtime_r(&now,&utc);std::strftime(date,sizeof(date),"%Y-%m-%dT%H:%M:%SZ",&utc);
    write_text(fd,"Uke Control diagnostic 1.10.2\nUTC: "+std::string(date)+"\nTopic: "+topic+
        "\nEffective UID: "+std::to_string(geteuid())+
        "\nPRIVATE LOCAL LOG: may contain IP/MAC addresses, host/user names, device identifiers, repository URLs and journal contents. Review before sharing. No upload is performed."
        "\nCollection: IN PROGRESS until the final COLLECTION COMPLETE marker."
        "\nNo active scan, pairing, stream, suspend, firmware control request, device setting or package transaction is requested."
        "\nLimits: 20 seconds and 8 MiB per command; 256 MiB and 15 minutes overall. Files: first 65536 bytes each, 4096 matches per pattern. Sysfs file sizes may not indicate truncation. Truncation and failures are recorded."
        "\nMissing tools, permissions, services and cached repository metadata remain unavailable. Collection success does not establish hardware support."
        "\nUser-session tools use this process's existing session; sudo may have no desktop/audio session.\n");
    unsigned ok=0,failed=0;bool bounded=false;
    const auto run=[&](const std::string& group,const std::string& label,const std::vector<std::string>& args) {
        struct stat info{};
        if(fstat(fd,&info)!=0)throw std::runtime_error("Cannot check diagnostic log size");
        if(info.st_size>=256*1024*1024||std::chrono::steady_clock::now()-started>=std::chrono::minutes(15)){bounded=true;return;}
        write_text(fd,"\n===== "+group+" / "+text_output(label)+" =====\nCommand:");
        for(const auto& arg:args)write_text(fd," "+sysfs::quote(arg));
        write_text(fd,"\n");
        const auto begin=std::chrono::steady_clock::now();
        const auto result=[&] {
            try {return execute(args,20,8*1024*1024);}
            catch(const std::exception& error){return Result{1,std::string("Collector error: ")+error.what()};}
        }();
        write_text(fd,text_output(result.output));
        const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-begin).count();
        write_text(fd,"\n[exit="+std::to_string(result.status)+" elapsed_ms="+std::to_string(ms)+"]\n");
        if(result.status==0)++ok;else ++failed;
    };
    run("context","Operating system",{"/usr/bin/cat","/etc/os-release"});
    run("context","Running kernel",{"/usr/bin/uname","-a"});
    run("context","Collector package",{"/usr/bin/rpm","-q","uke-control"});
    for(const auto& job:jobs(topic)) {
        if(bounded)break;
        if(job.args[0]!="@files"){run(job.topic,job.label,job.args);continue;}
        glob_t matches{};
        const int result=glob(job.args[1].c_str(),GLOB_NOSORT|GLOB_BRACE,nullptr,&matches);
        if(result!=0) {
            write_text(fd,"\n===== "+job.topic+" / "+job.label+" =====\n[unavailable: no matching files or glob error="+std::to_string(result)+"]\n");
            ++failed;
        } else {
            std::vector<std::string> args{"/usr/bin/head","-v","-c","65536","--"};
            const auto count=std::min<std::size_t>(matches.gl_pathc,4096);
            args.insert(args.end(),matches.gl_pathv,matches.gl_pathv+count);
            run(job.topic,job.label,args);
            if(matches.gl_pathc>count){write_text(fd,"[file match limit reached]\n");++failed;}
            // head bounds each file; mark exact-limit output conservatively without hiding later files.
            for(std::size_t i=0;i<count;++i) {
                struct stat st{};
                if(stat(matches.gl_pathv[i],&st)==0&&st.st_size>=65536)
                    write_text(fd,"[file may be truncated at 65536 bytes: "+text_output(matches.gl_pathv[i])+"]\n");
            }
        }
        globfree(&matches);
    }
    write_text(fd,"\nCOLLECTION COMPLETE\nSuccessful collectors: "+std::to_string(ok)+"\nUnavailable/failed collectors: "+std::to_string(failed)+
        "\nOverall limit reached: "+(bounded?"yes; remaining collectors skipped":"no")+"\nReview individual exit codes; missing data and bounded output are not device test results.\n");
    if(fsync(fd)!=0)throw std::runtime_error("Cannot sync diagnostic log");
    return name;
}
int command(int argc,char** argv) {
    if(argc==0||(argc==1&&(std::string_view(argv[0])=="--help"||std::string_view(argv[0])=="help"))) {
        std::cout<<"Usage: uke-control diagnostic TOPIC\nTopics: all";
        for(const auto& topic:topics)std::cout<<' '<<topic;
        std::cout<<"\nSaves uke-diagnostic-TOPIC.log in the current directory with mode 0600.\nUse sudo for system logs; existing output is never overwritten.\nReports are private and are not uploaded. Optional tools are not installed automatically.\n";
        return 0;
    }
    if(argc!=1)throw std::runtime_error("Expected one diagnostic topic; use uke-control diagnostic --help");
    std::cerr<<"Collecting diagnostic "<<argv[0]<<"; the log is private. Optional collectors may be unavailable.\n";
    const auto path=collect(argv[0]);
    std::cout<<path<<"\n";
    return 0;
}
}
