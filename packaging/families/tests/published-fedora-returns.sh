#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Run after the candidate Control and meta RPMs are signed; no tablet access.
set -Eeuo pipefail
[[ ${UKE_COPR_FEDORA_RETURN_FIXTURE:-} == 1 ]]
[[ ${UKE_GNU_META_BUILD:-} =~ ^[0-9]+$ ]]
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
test ! -e /etc/senemos/kernel-update.conf
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
cache=$root/uke-desktop-packaging/source-cache/fedora-returns-20261101
fixture=$(mktemp -d)
trap 'rm -rf -- "$fixture"' EXIT
mkdir -p "$fixture"/{baseline,current,candidate,originals,kernel,roots,cache,keys}
touch "$fixture/empty.conf"
rpmkeys --dbpath "$fixture/keys" --import "$root/packaging/families/keys/fedora-46.asc"
curl -fLsS --max-time 60 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/pubkey.gpg -o "$fixture/copr.asc"
rpmkeys --dbpath "$fixture/keys" --import "$fixture/copr.asc"
rpmkeys --dbpath "$fixture/keys" --list | awk '{print toupper($1)}' | LC_ALL=C sort > "$fixture/keys.actual"
jq -r '.key_fingerprints[]' "$root/packaging/families/consolidation/fedora-return-inputs.json" | LC_ALL=C sort > "$fixture/keys.expected"
cmp "$fixture/keys.expected" "$fixture/keys.actual"
while IFS=$'\t' read -r old original; do
    cp "$cache/${old##*/}" "$fixture/baseline/"
    cp "$cache/${old##*/}" "$fixture/candidate/"
    cp "$cache/${original##*/}" "$fixture/originals/"
done < <(jq -r '.pairs[]|[.fork.url,.original.url]|@tsv' "$root/packaging/families/consolidation/fedora-return-inputs.json")
fetch() {
    local id=$1 name=$2 repo=$3 url="https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/$1-$2/"
    curl -fLsS --max-time 60 "${url}results.json" -o "$fixture/results.json"
    while IFS= read -r file; do
        [[ $file =~ ^[A-Za-z0-9+_.~-]+[.]rpm$ ]]
        [[ $file != uke-cutefish-meta-* ]] || continue
        curl -fLsS --max-time 120 "$url$file" -o "$fixture/$repo/$file"
        rpmkeys --dbpath "$fixture/keys" --checksig "$fixture/$repo/$file"
    done < <(jq -r '.packages[]|select(.arch!="src")|.name+"-"+.version+"-"+.release+"."+.arch+".rpm"' "$fixture/results.json")
}
fetch 11239542 uke-meta-integration baseline
fetch 11239731 uke-meta-integration current
fetch "${UKE_GNU_META_BUILD:?qualified candidate meta build}" uke-meta-integration candidate
jq -r '.packages[]|select(.arch!="src")|.name' "$fixture/results.json" > "$fixture/meta-names"
fetch 11239677 uke-control candidate
cp "$fixture/candidate/uke-control-"*.rpm "$fixture/baseline/"
fetch 11239644 uke-boot-integration baseline
fetch 11239485 uke-power-integration baseline
fetch 11239841 uke-boot-integration candidate
fetch 11239842 uke-power-integration candidate
# Model an installed tablet cohort; Control deliberately refuses kernel-free roots.
kernel_url=https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/11238013-senemos-uke-linux-kernel-mainline/
curl -fLsS --max-time 60 "${kernel_url}results.json" -o "$fixture/kernel-results.json"
while IFS= read -r file;do
    [[ $file =~ ^senemos-uke-linux-kernel-mainline(-core|-modules|-dtbs|-boot)?-[A-Za-z0-9_.~-]+[.]aarch64[.]rpm$ ]]
    curl -fLsS --max-time 240 "$kernel_url$file" -o "$fixture/kernel/$file"
    rpmkeys --dbpath "$fixture/keys" --checksig "$fixture/kernel/$file"
done < <(jq -r '.packages[]|select(.arch=="aarch64" and (.name|test("^senemos-uke-linux-kernel-mainline(-core|-modules|-dtbs|-boot)?$")))|.name+"-"+.version+"-"+.release+"."+.arch+".rpm"' "$fixture/kernel-results.json")
[[ $(find "$fixture/kernel" -maxdepth 1 -name '*.rpm' | wc -l) == 5 ]]
cp "$fixture/kernel/"*.rpm "$fixture/baseline/"
cp "$fixture/kernel/"*.rpm "$fixture/candidate/"
for repo in baseline candidate originals; do createrepo_c --quiet "$fixture/$repo"; done
c++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror $(pkg-config --cflags libdnf5 rpm libsystemd) \
    "$root/packaging/families/tests/published-fedora-returns.cpp" "$root/uke-control/src/cli.cpp" -I"$root/uke-control/src" -o "$fixture/solver" $(pkg-config --libs libdnf5 rpm libsystemd)
for selection in core kde all; do
    for mode in fresh upgrade mixed reverse weak-off desktop-upgrade; do
        target=$fixture/roots/$selection-$mode
        mkdir -p "$target/usr/lib/sysimage/rpm"
        rpm --dbpath "$target/usr/lib/sysimage/rpm" --initdb
        rpmkeys --dbpath "$target/usr/lib/sysimage/rpm" --import "$fixture/copr.asc" "$root/packaging/families/keys/fedora-46.asc"
        seed=("$fixture/kernel/"*.rpm);count=0
        if [[ $mode != fresh ]]; then
            for file in "$fixture/baseline/"*.rpm; do
                name=$(rpm -qp --qf '%{NAME}' "$file" 2>/dev/null)
                case $name in senemos-uke-linux-kernel-*|uke-control-tests|uke-control-tui) continue;; uke-plasma-controls) [[ $selection == all ]] || continue;; esac
                original=
                original=$(jq -r --arg name "$name" '.pairs[]|select(.fork_name==$name)|.original.url|split("/")|last' "$root/packaging/families/consolidation/fedora-return-inputs.json")
                caller=false
                case $name in uke-boot-integration|uke-boot-recovery|uke-cdc-debug|uke-esp32-cdc|uke-kernel-update|uke-installation-profile|uke-battery-observer|uke-charge-control|uke-offline-charge|uke-adaptive-display) caller=true;;esac
                if [[ -z $original && $name != uke-control && $name != uke-core-meta && $caller == false ]];then
                    [[ $selection == all || $selection == kde && $name == uke-kde-plasma-meta ]] || continue
                fi
                repo=baseline;[[ $mode != reverse ]] || repo=candidate
                if [[ $mode == mixed && -n $original ]]; then ((count+=1));if ((count%2==0));then repo=originals;fi;fi
                if [[ $mode == desktop-upgrade ]];then
                    [[ -z $original || $name == uke-libstdcxx ]] || repo=originals
                    case $name in uke-core-meta|uke-desktop-metas|uke-kde-plasma-meta|uke-plasma-controls) repo=current;;esac
                fi
                if [[ $repo == originals || $mode == reverse && -n $original ]]; then
                    seed+=("$fixture/originals/$original")
                else
                    mapfile -t matches < <(find "$fixture/$repo" -maxdepth 1 -name "$name-*.rpm" | while read -r candidate; do [[ $(rpm -qp --qf '%{NAME}' "$candidate" 2>/dev/null) != "$name" ]] || printf '%s\n' "$candidate"; done)
                    [[ ${#matches[@]} == 1 ]];seed+=("${matches[0]}")
                fi
            done
        fi
        if ((${#seed[@]}));then rpm --define '__systemd_sysusers /usr/bin/true' --dbpath "$target/usr/lib/sysimage/rpm" -i --justdb --nodeps --noscripts --notriggers "${seed[@]}";fi
        "$fixture/solver" "$fixture" "$target" "$mode" "$selection"
    done
done
UKE_COPR_GNU_RETURN=1 bash "$root/packaging/families/tests/gnu-initramfs.sh" "$fixture/candidate/uke-boot-recovery-1.8.18-2.fc46.aarch64.rpm"
