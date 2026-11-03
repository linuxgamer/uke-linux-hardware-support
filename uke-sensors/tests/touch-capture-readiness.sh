#!/usr/bin/env bash
# Exercise the production notification after valid frames, without a device.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
awk '/^static void notify_first_frame\(/ {copy=1} copy {print} copy && /^}/ {copy=0;found++} END{if(found!=1)exit 1}' \
    "$root/src/touch-capture.cpp" > "$stage/production.h"
cat > "$stage/check.cpp" <<'CPP'
#include <cassert>
#include <stdexcept>
#include <string>
static unsigned calls;
static int result=1;
static int sd_notify(int,const char *state) {assert(std::string(state).starts_with("READY=1\n"));++calls;return result;}
static void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
#include "production.h"
int main() {
    notify_first_frame(0);assert(calls==0);
    notify_first_frame(1);assert(calls==1);
    notify_first_frame(2);assert(calls==1);
    result=-1;bool refused=false;
    try{notify_first_frame(1);}catch(const std::runtime_error&){refused=true;}
    assert(refused && calls==2);
}
CPP
"${CXX:-c++}" -std=c++20 -O2 -Wall -Wextra -Werror -I "$stage" "$stage/check.cpp" -o "$stage/check"
"$stage/check"
printf '%s\n' 'THP readiness waits for the first valid frame, notifies once and rejects notification failure: PASS'
