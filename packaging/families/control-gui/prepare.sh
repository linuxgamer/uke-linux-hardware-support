#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Host/source-chroot only. No RPM payload or maintainer script is executed.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
here=$(cd -- "$(dirname -- "$0")"; pwd)
root=$(cd -- "$here/../../.."; pwd)
out=$(realpath -m "${1:-$here/build/srpm}")
arch=${2:-aarch64}
phase=${3:-backend}
[[ $arch == aarch64 ]]
[[ $phase == backend || $phase == metas ]]
manifest="$here/input.json"
url=$(jq -er .url "$manifest")
digest=$(jq -er .sha256 "$manifest")
expected=$(jq -er .source_evr "$manifest")
[[ $url == https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/Packages/u/uke-control-2.8.1-5.fc46.src.rpm ]]
[[ $digest =~ ^[a-f0-9]{64}$ ]]
input="$here/referances/uke-control-2.8.1-5.fc46.src.rpm"
mkdir -p "$(dirname "$input")"
if [[ ! -s $input ]]; then
    curl --fail --location --silent --show-error --proto '=https' --proto-redir '=https' --max-time 180 "$url" -o "$input.part"
    mv "$input.part" "$input"
fi
printf '%s  %s\n' "$digest" "$input" | sha256sum -c -
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
mkdir -m700 "$stage/gpg" "$stage/keys"
fingerprint=$(gpg --homedir "$stage/gpg" --batch --show-keys --with-colons "$here/copr-public-key.asc" 2>/dev/null | awk -F: '$1=="fpr"{print $10}')
[[ $fingerprint == "$(jq -er .signing_key "$manifest")" ]]
rpmkeys --dbpath "$stage/keys" --import "$here/copr-public-key.asc"
rpmkeys --dbpath "$stage/keys" --checksig "$input"
[[ $(rpm --dbpath "$stage/keys" -qp --qf '%{VERSION}-%{RELEASE}' "$input") == "$expected" ]]
rpm2cpio "$input" > "$stage/input.cpio"
cpio -t --quiet < "$stage/input.cpio" > "$stage/members"
awk '/^\/|(^|\/)\.\.($|\/)/{bad=1} END{exit bad}' "$stage/members"
[[ $(wc -l < "$stage/members") == 2 ]]
mkdir "$stage/input"
(cd "$stage/input"; cpio -idm --quiet --no-absolute-filenames < "$stage/input.cpio")
test -s "$stage/input/uke-control.spec" -a -s "$stage/input/uke-control-2.8.1.tar.xz"
! grep -q '^Source[1-9]:' "$stage/input/uke-control.spec"
rpmspec --target "$arch" -q --qf '%{NAME}\t%{VERSION}\t%{RELEASE}\n' "$stage/input/uke-control.spec" > "$stage/old-identities"
[[ $(wc -l < "$stage/old-identities") == 12 ]]
increment=$(jq -er .release_increment "$manifest")
[[ $increment =~ ^[1-9]$ ]]
awk -F '\t' -v increment="$increment" 'BEGIN{OFS="\t"} $3 !~ /^[0-9]+[.]fc46$/ {exit 1} {new=$3; sub(/^[0-9]+/,int($3)+increment,new); print $1,$2,$3,new}' "$stage/old-identities" > "$stage/identities"
top="$out/rpmbuild"
mkdir -p "$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS}
cp "$stage/input/uke-control-2.8.1.tar.xz" "$top/SOURCES/"
cp "$stage/input/uke-control.spec" "$top/SOURCES/uke-control-published-spec.txt"
cp "$manifest" "$here/copr-public-key.asc" "$top/SOURCES/"
mkdir "$stage/contract"
cp "$root/uke-control/src/"{frontend.hpp,frontend-request.cpp,client.hpp} "$stage/contract/"
cp "$root/uke-desktop-metas/src/gnome.json" "$stage/contract/"
tar --sort=name --mtime=@1793577600 --owner=0 --group=0 --numeric-owner -cJf "$top/SOURCES/uke-control-frontend-contract.tar.xz" -C "$stage/contract" .
package=uke-control
if [[ $phase == metas ]]; then package=uke-desktop-metas; fi
component=uke-control
if [[ $phase == metas ]]; then component=uke-meta-integration; fi
tar -xOf "$stage/input/uke-control-2.8.1.tar.xz" "uke-control-2.8.1/inputs/$component/$component.spec" > "$stage/selected-input.spec"
grep '^BuildRequires:' "$stage/selected-input.spec" > "$stage/buildrequires"
if [[ $phase == backend ]]; then printf '%s\n' 'BuildRequires: findutils' >> "$stage/buildrequires"; fi
awk -v build_phase="$phase" -f "$here/rewrite.awk" "$stage/identities" "$stage/input/uke-control.spec" > "$stage/complete.spec"
awk -v build_phase="$phase" -v requirements="$stage/buildrequires" -f "$here/select-phase.awk" "$stage/complete.spec" > "$top/SPECS/$package.spec"
rpmspec --target "$arch" -q --qf '%{NAME}\t%{VERSION}-%{RELEASE}\n' "$top/SPECS/$package.spec" > "$top/package-identities.tsv"
if [[ $phase == backend ]]; then
grep -Fx $'uke-control\t'"$(jq -er .output_evr "$manifest")" "$top/package-identities.tsv"
else
grep -Fx $'uke-kde-plasma-meta\t'"$(jq -er .desktop_meta_evr "$manifest")" "$top/package-identities.tsv"
grep -Fx $'uke-gnome-meta\t'"$(jq -er .desktop_meta_evr "$manifest")" "$top/package-identities.tsv"
fi
[[ $(wc -l < "$top/package-identities.tsv") == 5 ]]
rpmbuild -bs --nodeps --target "$arch" --define '_buildhost uke-builder' --define "_topdir $top" "$top/SPECS/$package.spec"
output_evr=$(jq -er .output_evr "$manifest")
if [[ $phase == metas ]]; then output_evr=$(jq -er .desktop_meta_evr "$manifest"); fi
install -m644 "$top/SRPMS/$package-$output_evr.src.rpm" "$out/"
