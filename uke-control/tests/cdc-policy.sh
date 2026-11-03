#!/usr/bin/env bash
# Actual backend policy functions; synthetic state and no service/device writes.
set -Eeuo pipefail
ulimit -c 0
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
awk '/^static void cdc_validate\(/{copy=1} /^std::string status\(/{copy=0} copy' "$root/src/backend.cpp" > "$work/policy.inc"
cat > "$work/check.cpp" <<'CPP'
#include "backend.hpp"
#include "rpm-api.hpp"
#include "sysfs.hpp"
#include <cassert>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
namespace fs=std::filesystem;
namespace uke {
static fs::path state;
static bool locked=false, board_ok=true, api_ok=true, disk_failure=false, helper_trusted=true;
static int helper_failure=0, service_failure=0, helper_calls=0, service_calls=0;
static uid_t policy_uid=0;
static std::map<std::string,std::string> runtime;
static void need(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
static void board(){need(board_ok,"foreign board");}
struct Lock {
 explicit Lock(bool wait=false){(void)wait;assert(!locked);locked=true;}
 ~Lock(){assert(locked);locked=false;}
};
struct Fd{int n;~Fd(){if(n>=0)close(n);}};
std::string read(const std::string& path,size_t limit) {
 if(path.starts_with(state.string())) {std::ifstream in(path);return {std::istreambuf_iterator<char>(in),{}};}
 if(path=="/sys/firmware/devicetree/base/chosen/senemos,usb-profile")return "typec-v1\n";
 (void)limit;return runtime[path];
}
static int test_lstat(const char* path,struct stat* st) {
 if(std::string(path)=="/usr/libexec/senemos-uke/uke-usb-network") {
  *st={};st->st_mode=helper_trusted?S_IFREG|0755:S_IFLNK|0777;return 0;
 }
 int result=::lstat(path,st);if(!result)st->st_uid=policy_uid;return result;
}
static void atomic_file(const fs::path& path,const std::string& value) {
 if(disk_failure)throw std::runtime_error("storage failure");
 std::ofstream(path)<<value;assert(chmod(path.c_str(),0600)==0);
}
#define lstat test_lstat
#include "policy.inc"
#undef lstat
// Preserve the process boundary; GCC 16 AArch64 misdiagnoses IPA of the stub.
#if defined(__GNUC__) && !defined(__clang__)
[[gnu::noipa]]
#endif
Result execute(const std::vector<std::string>& args,unsigned seconds,size_t limit) {
 (void)seconds;(void)limit;
 if(args[0]=="/usr/bin/rpm")return {api_ok?0:1,api_ok?"uke-usb-manual-cdc-api\t2\n":""};
 if(args[0]=="/usr/bin/systemctl") {
  assert(!locked);service_calls++;
  assert(args.back()=="uke-control-cdc.service");
  if(args[1]=="disable")assert(args.size()==4 && args[2]=="--now");
  else assert(args.size()==3 && (args[1]=="enable" || args[1]=="reload-or-restart"));
  if(service_failure)return {service_failure,"fixture service failure\n"};
  if(args[1]=="reload-or-restart")return cdc_apply();
  return {0,"Created symlink \"fixture\"\n"};
 }
 assert(args[0]=="/usr/libexec/senemos-uke/uke-usb-network" && locked);
 helper_calls++;
 if(helper_failure)return {helper_failure,"fixture not ready\n"};
 bool enabled=args[1]=="manual-on";
 assert(enabled||args[1]=="manual-off");
 runtime["/run/senemos-uke/usb-gadget-profile"]=enabled?"manual-acm\n":"";
 runtime["/sys/kernel/config/usb_gadget/uke_debug/UDC"]=enabled?"fixture\n":"";
 return {0,""};
}
template<class Call>static void refuses(Call call){bool rejected=false;try{call();}catch(const std::runtime_error&){rejected=true;}assert(rejected);}
}
int main(int argc,char** argv) {
 using namespace uke;
 assert(argc==2);state=argv[1];fs::create_directories(state);
 assert(cdc_apply().status==0 && helper_calls==0);
 assert(cdc_set(true).status==0 && !fs::exists(state/"cdc-on"));
 assert(cdc_set(false).status==0 && !fs::exists(state/"cdc-on"));
 auto on=cdc_persistent(true);
 assert(on.status==0 && cdc_saved() && service_calls==2);
 assert(on.output.find("\"policy_saved\":true")!=std::string::npos);
 assert(on.output.find("\"automatic_startup_requested\":true")!=std::string::npos);
 assert(on.output.find("\\\"fixture\\\"\\u000a")!=std::string::npos);
 assert(cdc_apply().status==0 && cdc_saved()); // A fresh apply retains saved intent.
 int reapplies=helper_calls;
 assert(cdc_persistent(true).status==0 && helper_calls==reapplies+1);
 auto off=cdc_persistent(false);
 assert(off.status==0 && !cdc_saved() && runtime["/run/senemos-uke/usb-gadget-profile"].empty());
 int calls=helper_calls;
 assert(cdc_apply().status==0 && helper_calls==calls);
 assert(cdc_persistent(false).status==0);
 board_ok=false;refuses([]{cdc_persistent(true);});assert(!cdc_saved());board_ok=true;
 api_ok=false;refuses([]{cdc_persistent(true);});assert(!cdc_saved());api_ok=true;
 helper_trusted=false;refuses([]{cdc_persistent(true);});assert(!cdc_saved());helper_trusted=true;
 disk_failure=true;refuses([]{cdc_persistent(true);});assert(!cdc_saved());disk_failure=false;
 service_failure=42;auto failed=cdc_persistent(true);
 assert(failed.status==42 && cdc_saved() && failed.output.find("\"operation_completed\":false")!=std::string::npos);
 service_failure=0;helper_failure=74;
 auto failed_apply=cdc_persistent(true);
 assert(failed_apply.status==74 && cdc_saved() && failed_apply.output.find("\"operation_completed\":false")!=std::string::npos);
 service_failure=0;helper_failure=69;
 runtime["/run/senemos-uke/usb-gadget-profile"]="";
 assert(cdc_apply().status==0 && cdc_saved());
 runtime["/run/senemos-uke/usb-gadget-profile"]="foreign\n";
 calls=helper_calls;assert(cdc_apply().status==0 && helper_calls==calls+1);
 helper_failure=74;assert(cdc_apply().status==74);
 helper_failure=0;assert(cdc_persistent(false).status==0);
 atomic_file(state/"cdc-on","invalid\n");refuses([]{cdc_apply();});
 fs::remove(state/"cdc-on");fs::create_symlink("/etc/passwd",state/"cdc-on");refuses([]{cdc_apply();});
 fs::remove(state/"cdc-on");atomic_file(state/"cdc-on","on\n");chmod((state/"cdc-on").c_str(),0644);refuses([]{cdc_apply();});
 chmod((state/"cdc-on").c_str(),0600);fs::create_hard_link(state/"cdc-on",state/"alias");refuses([]{cdc_apply();});
 fs::remove(state/"alias");assert(cdc_apply().status==0);
 policy_uid=1000;refuses([]{cdc_apply();});policy_uid=0;
}
CPP
sanitize=()
[[ -z ${UKE_CDC_SANITIZE:-} ]] || sanitize=(-fsanitize=address,undefined -fno-omit-frame-pointer)
"${CXX:-c++}" -std=c++20 -O2 -g -Wall -Wextra -Werror "${sanitize[@]}" -I "$root/src" -I "$work" "$work/check.cpp" -o "$work/check"
"$work/check" "$work/state"
grep -Fx 'ConditionPathExists=/var/lib/uke-control/cdc-on' "$root/packaging/uke-control-cdc.service"
grep -Fx 'After=sys-kernel-config.mount systemd-udev-trigger.service uke-control.service' "$root/packaging/uke-control-cdc.service"
grep -Fx 'RuntimeDirectoryPreserve=yes' "$root/packaging/uke-control-cdc.service"
grep -Fx 'RemainAfterExit=yes' "$root/packaging/uke-control-cdc.service"
grep -Fx 'ExecReload=/usr/bin/uke-control cdc apply' "$root/packaging/uke-control-cdc.service"
grep -Fx 'disable uke-control-cdc.service' "$root/packaging/80-uke-control.preset"
[[ $(grep -c 'TEST=="/var/lib/uke-control/cdc-on".*--no-block reload-or-restart uke-control-cdc.service' "$root/packaging/90-uke-control-cdc.rules") == 3 ]]
printf 'Production CDC policy: persistence, fresh apply, off, identity/API, ownership, storage and service failure checks passed\n'
