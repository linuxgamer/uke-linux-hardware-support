#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# COPR only. Signed public RPM metadata in isolated databases; no payload install.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $(uname -m) == aarch64 ]]
solver=$(realpath "${1:?Pass the compiled remote solver}")
fixture=$(mktemp -d)
trap 'find "$fixture" -depth -delete' EXIT
mkdir -p "$fixture"/{keys,gpg,packages,roots,cache}
chmod 700 "$fixture/gpg"
touch "$fixture/empty.conf"
fingerprint=$(gpg --homedir "$fixture/gpg" --batch --show-keys --with-colons copr-public-key.asc 2>/dev/null | awk -F: '$1=="fpr"{print $10}')
[[ $fingerprint == 2204A661EA630A5303500085538797E72A3E4ECA ]]
rpmkeys --dbpath "$fixture/keys" --import copr-public-key.asc
repository=https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64
packages=(
 uke-kde-plasma-meta-1.0.0-35.1.fc46.aarch64
 uke-core-meta-1.0.7-19.1.fc46.aarch64
 uke-control-2.8.1-5.fc46.aarch64
 uke-kde-plasma-meta-1.0.0-37.fc46.aarch64
 uke-gnome-meta-1.0.0-37.fc46.noarch
 uke-control-gui-qt-1.0.0-3.fc46.aarch64
 uke-control-gui-gtk-1.0.0-3.fc46.aarch64
 uke-control-2.8.1-7.fc46.aarch64
 uke-control-client-2.8.1-7.fc46.aarch64
 uke-control-client-devel-2.8.1-7.fc46.aarch64
)
for package in "${packages[@]}"; do
    file=$fixture/packages/$package.rpm
    curl -fLsS --max-time 120 --proto '=https' --proto-redir '=https' "$repository/Packages/u/$package.rpm" -o "$file"
    rpmkeys --dbpath "$fixture/keys" --checksig "$file"
    [[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}' "$file") == "$package" ]]
done
for mode in upgrade-kde fresh-kde fresh-gnome; do
    target=$fixture/roots/$mode
    mkdir -p "$target/usr/lib/sysimage/rpm"
    rpm --dbpath "$target/usr/lib/sysimage/rpm" --initdb
    if [[ $mode == upgrade-kde ]]; then
        seed=("${packages[@]:0:3}")
        files=();for package in "${seed[@]}"; do files+=("$fixture/packages/$package.rpm"); done
        rpm --define '__systemd_sysusers /usr/bin/true' --dbpath "$target/usr/lib/sysimage/rpm" \
            -i --justdb --nodeps --noscripts --notriggers "${files[@]}"
    fi
    "$solver" "$fixture" "$target" "$mode"
done
