#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
here=$(cd -- "$(dirname -- "$0")"; pwd)
cd "$here"
make srpm
top=build/srpm/rpmbuild
old="$top/SOURCES/uke-control-published.spec"
new="$top/SPECS/uke-control.spec"
# The imported nested archives contain the existing DNF/boot/device behavior.
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
rpm2cpio referances/uke-control-2.8.1-5.fc46.src.rpm > "$stage/source.cpio"
(cd "$stage"; cpio -idm --quiet --no-absolute-filenames < source.cpio)
cmp "$stage/uke-control-2.8.1.tar.xz" "$top/SOURCES/uke-control-2.8.1.tar.xz"
cmp "$stage/uke-control.spec" "$old"
# Existing maintainer scripts remain byte-for-byte unchanged.
scripts() {
    awk '/^%(pre|post|preun|postun|pretrans|posttrans|trigger.*|filetrigger.*|transfiletrigger.*)([[:space:]]|$)/ {copy=1}
         /^%(files|changelog)([[:space:]]|$)/ {copy=0}
         copy {print}' "$1"
}
scripts "$old" > "$stage/old-scripts"
scripts "$new" > "$stage/new-scripts"
cmp "$stage/old-scripts" "$stage/new-scripts"
selection() {
    awk -v package="$1" '$0=="%package -n " package {copy=1;next}
        /^%description/ {copy=0} copy {print}' "$new"
}
selection uke-kde-plasma-meta > "$stage/kde"
selection uke-gnome-meta > "$stage/gnome"
grep -Fx 'Requires: uke-control-qt >= 1.0.0-2' "$stage/kde"
grep -Fx 'Requires: uke-control-gtk >= 1.0.0-2' "$stage/gnome"
! grep -E '^Requires:.*(uke-control-gtk|gnome-shell|uke-gnome-meta)' "$stage/kde"
! grep -E '^Requires:.*(uke-control-qt|plasma-workspace|uke-kde-plasma-meta)' "$stage/gnome"
grep -Fx 'Provides: uke-control(frontend-v1) = 1' "$new"
grep -Fx 'Provides: uke-control(frontend-sdk-v1) = 1' "$new"
grep -Fx 'Provides: /usr/include/uke-control/frontend.hpp' "$new"
grep -Fx 'Provides: /usr/libexec/uke-control/frontend-request' "$new"
grep -Fx 'Requires: uke-control(aarch-64) = 2.8.1-6.fc46' "$new"
grep -Fx 'Recommends: uke-control-tui(aarch-64) = 2.8.1-6.fc46' "$new"
rpmspec --target aarch64 --parse --define "_sourcedir $(realpath "$top/SOURCES")" "$new" > "$stage/expanded.spec"
grep -F 'frontend-request.cpp' "$stage/expanded.spec"
printf '%s\n' 'GUI source contract, monotonic identities, mandatory DE dependencies and preserved payload/scriptlets passed.'
