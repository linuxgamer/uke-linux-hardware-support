#!/usr/bin/env bash
# Compile the actual worker with host-only clock/IO stubs; no device command.
set -Eeuo pipefail
cd "$(dirname "$0")/.."
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
cat > "$stage/test.cpp" <<'CPP'
#include "lights.hpp"
#include <cassert>
#include <chrono>
#include <csignal>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <unistd.h>
#include <vector>
namespace uke::lights {
static std::filesystem::path location;
static std::vector<std::string> writes;
static unsigned calls, clock_failure, readback_failure, notify_failure, signal_stop, owned;
static void need(bool ok,const std::string& text){if(!ok)throw std::runtime_error(text);}
static void root() {}
struct Lock {explicit Lock(bool) {}};
static Source required(bool){Source s;s.path=location;s.brightness=owned;return s;}
static void write_fd(int fd,const std::string& value){assert(fd>=0);writes.push_back(value);}
static std::string raw(const std::string&,const std::filesystem::path&){return readback_failure?"0":"255";}
static int sd_notify(int,const char*){if(signal_stop)std::raise(SIGTERM);return notify_failure?-1:0;}
static volatile sig_atomic_t stopped;
static void stop(int){stopped=1;}
static int test_clock(clockid_t clock,timespec* value) {
    assert(clock==CLOCK_BOOTTIME);++calls;
    if(calls==clock_failure)return -1;
    value->tv_sec=calls==1?0:100;value->tv_nsec=0;return 0;
}
#define clock_gettime test_clock
CPP
sed -n '/^struct Fd /p' src/lights.cpp >> "$stage/test.cpp"
for function in boot_seconds hold; do
    awk -v name="$function" '$0 ~ "^static .* "name"\\(\\)" {copy=1} copy {print} copy && /^}/ {exit}' src/lights.cpp >> "$stage/test.cpp"
done
cat >> "$stage/test.cpp" <<'CPP'
}
int main(int argc,char** argv) {
    using namespace uke::lights;
    assert(argc==2);location=argv[1];std::ofstream(location/"brightness")<<"0\n";
    for(unsigned mode=0;mode<6;++mode) {
        writes.clear();calls=0;stopped=0;owned=0;clock_failure=0;readback_failure=0;notify_failure=0;signal_stop=0;
        if(mode==1)clock_failure=1;
        if(mode==2)clock_failure=2;
        if(mode==3)readback_failure=1;
        if(mode==4)notify_failure=1;
        if(mode==5)signal_stop=1;
        bool failed=false;try{hold();}catch(...){failed=true;}
        assert(failed==(mode>0 && mode<5));
        if(mode==1)assert(writes.empty());
        else assert((writes==std::vector<std::string>{"255\n","0\n"}));
    }
    writes.clear();owned=255;bool failed=false;try{hold();}catch(...){failed=true;}
    assert(failed && writes.empty());
    std::cout<<"Actual torch worker: suspend-time expiry, SIGTERM, clock/readback/notify failure cleanup and ownership refusal passed; simulated IO only\n";
}
CPP
${CXX:-c++} -std=c++20 -Wall -Wextra -Werror -UNDEBUG -Isrc "$stage/test.cpp" -o "$stage/test"
"$stage/test" "$stage"
