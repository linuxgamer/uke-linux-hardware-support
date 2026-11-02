#!/bin/bash
# Source-builder architecture reduction only; no upstream build executes.
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")/.."; pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
control=$here/recipes/iio-sensor-proxy/debian/control
cmp "$control" "$here/main-recipes/iio-sensor-proxy/debian/control"
grep -Fxq '	test "$$(dpkg-architecture -qDEB_HOST_ARCH)" = arm64' "$here/main-recipes/iio-sensor-proxy/debian/rules"
mkdir "$work/dpkg"
: > "$work/dpkg/status"
expected='libssc-dev (>= 0.4.4-2ubuntu1~uke1~tenacious1) [arm64], uke-libssc-dual-light-abi-1 [arm64]'
grep -Fq "$expected" "$control"
dpkg-checkbuilddeps --admindir="$work/dpkg" -I -a amd64 -d "$expected" "$control"
if dpkg-checkbuilddeps --admindir="$work/dpkg" -I -a arm64 -d "$expected" "$control" > "$work/arm64.log" 2>&1; then exit 1; fi
grep -Fq 'libssc-dev (>= 0.4.4-2ubuntu1~uke1~tenacious1)' "$work/arm64.log"
grep -Fq uke-libssc-dual-light-abi-1 "$work/arm64.log"
grep -Fxq 'Architecture: arm64' "$control"
grep '^Depends:' "$control" | grep -Eq 'uke-libssc-dual-light-abi-1$'
if (cd "$work"; DEB_HOST_ARCH=amd64 make -f "$here/recipes/iio-sensor-proxy/debian/rules" override_dh_auto_configure) > "$work/refused-compile.log" 2>&1; then exit 1; fi
grep -Fq 'test "$(dpkg-architecture -qDEB_HOST_ARCH)" = arm64' "$work/refused-compile.log"
[[ ! -e $work/src && ! -e $work/build ]]
echo 'amd64 source construction omits only ARM64 libssc build gates; ARM64 build/runtime gates and compile refusal remain.'
