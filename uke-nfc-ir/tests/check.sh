#!/usr/bin/env bash
# COPR only: actual client logic with fake providers, never a real NFC device.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
for compiler in g++ clang++; do
    "$compiler" -std=c++17 -O1 -g -Wall -Wextra -Werror -UNDEBUG \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -I "$root/include" -I "$root/src" "$root/tests/client.cpp" -o "$stage/check"
    ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 "$stage/check"
done
