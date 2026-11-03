#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# COPR only. Signed small RPMs and metadata-only isolated installed databases.
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
packages=(uke-nfc-ir-1.0.0-1.fc46.noarch uke-nfc-ir-1.1.0-1.fc46.aarch64)
for package in "${packages[@]}"; do
    file=$fixture/packages/$package.rpm
    curl -fLsS --max-time 120 --max-filesize 1048576 --proto '=https' --proto-redir '=https' "$repository/Packages/u/$package.rpm" -o "$file"
    rpmkeys --dbpath "$fixture/keys" --checksig "$file"
    [[ $(rpm --dbpath "$fixture/keys" -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}' "$file") == "$package" ]]
done
new=$fixture/packages/${packages[1]}.rpm
rpm -qp --requires "$new" | grep -Fx 'uke-kernel(nfc-ir-v2) >= 1'
rpm -qpl "$new" | grep -Fx /usr/bin/uke-nfc-tag
if rpm -qpl "$new" | grep -E '\.py(c|o)?$|/tests?/'; then
    echo 'Unexpected client payload' >&2; exit 1
fi
for mode in upgrade-noarch fresh; do
    target=$fixture/roots/$mode
    mkdir -p "$target/usr/lib/sysimage/rpm"
    rpm --dbpath "$target/usr/lib/sysimage/rpm" --initdb
    rpmkeys --dbpath "$target/usr/lib/sysimage/rpm" --import copr-public-key.asc
    if [[ $mode == upgrade-noarch ]]; then
        rpm --define '__systemd_sysusers /usr/bin/true' --dbpath "$target/usr/lib/sysimage/rpm" \
            -i --justdb --nodeps --noscripts --notriggers "$fixture/packages/${packages[0]}.rpm"
    fi
    "$solver" "$fixture" "$target" "$mode"
done
