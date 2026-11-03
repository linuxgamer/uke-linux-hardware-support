#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
here=$(cd -- "$(dirname -- "$0")"; pwd)
cd "$here"
make srpm outdir=build/backend
make srpm PHASE=metas outdir=build/metas
top=build/backend/rpmbuild
metatop=build/metas/rpmbuild
old="$top/SOURCES/uke-control-published-spec.txt"
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
    awk -v phase="$2" '/^%(pre|post|preun|postun|pretrans|posttrans|trigger.*|filetrigger.*|transfiletrigger.*)([[:space:]]|$)/ {
             package="uke-control";for(i=2;i<=NF;i++)if($i=="-n")package=$(i+1)
             copy=(phase=="backend" ? package=="uke-control" : package=="uke-kde-plasma-meta")
         }
         /^%(files|changelog)([[:space:]]|$)/ {copy=0}
         copy {print}' "$1"
}
scripts "$old" backend > "$stage/old-scripts"
scripts "$new" backend > "$stage/new-scripts"
cmp "$stage/old-scripts" "$stage/new-scripts"
scripts "$old" metas > "$stage/old-meta-scripts"
scripts "$metatop/SPECS/uke-desktop-metas.spec" metas > "$stage/new-meta-scripts"
cmp "$stage/old-meta-scripts" "$stage/new-meta-scripts"
! grep -E '^uke-.*meta' "$top/package-identities.tsv"
! grep -E '^uke-control' "$metatop/package-identities.tsv"
selection() {
    awk -v package="$1" '$0=="%package -n " package {copy=1;next}
        /^%description/ {copy=0} copy {print}' "$metatop/SPECS/uke-desktop-metas.spec"
}
selection uke-kde-plasma-meta > "$stage/kde"
selection uke-gnome-meta > "$stage/gnome"
grep -Fx 'Requires: uke-control-gui-qt >= 1.0.0-3' "$stage/kde"
grep -Fx 'Requires: uke-control-gui-gtk >= 1.0.0-3' "$stage/gnome"
grep -Fx 'Requires: hunspell-tr' "$stage/kde"
! grep -E '^Requires:.*(uke-control-gui-gtk|gnome-shell|uke-gnome-meta)' "$stage/kde"
! grep -E '^Requires:.*(uke-control-gui-qt|plasma-workspace|uke-kde-plasma-meta)' "$stage/gnome"
grep -Fx 'Provides: uke-control(frontend-v1) = 1' "$new"
grep -Fx 'Provides: uke-control(frontend-sdk-v1) = 1' "$new"
grep -Fx 'Provides: /usr/include/uke-control/frontend.hpp' "$new"
grep -Fx 'Provides: /usr/libexec/uke-control/frontend-request' "$new"
grep -Fx 'Requires: uke-control(aarch-64) = 2.8.1-7.fc46' "$new"
grep -Fx 'Recommends: uke-control-tui(aarch-64) = 2.8.1-7.fc46' "$new"
for rpm in "$top"/SRPMS/*.src.rpm "$metatop"/SRPMS/*.src.rpm; do
    rpm2cpio "$rpm" | cpio -t --quiet > "$stage/generated-members"
    [[ $(grep -c '[.]spec$' "$stage/generated-members") == 1 ]]
    grep -q 'uke-control-published-spec[.]txt$' "$stage/generated-members"
done
rpmspec --target aarch64 --parse --define "_sourcedir $(realpath "$top/SOURCES")" "$new" > "$stage/expanded.spec"
grep -F 'frontend-request.cpp' "$stage/expanded.spec"
printf '%s\n' 'GUI source contract, monotonic identities, mandatory DE dependencies and preserved payload/scriptlets passed.'
