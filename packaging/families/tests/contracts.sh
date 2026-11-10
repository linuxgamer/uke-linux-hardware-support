#!/usr/bin/env bash
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
cd "$root"
jq -e '.schema==1 and ([.families[].name]|(length==9) and (unique|length==9))' packaging/families/families.json >/dev/null
jq -e '.schema==1 and .fedora==46 and (.families|length==7) and ([.families[].sources[].name]|length==17 and (unique|length)==17) and all(.families[]; (.name=="uke-control" or (.name|test("^uke-(meta|power|sensor|audio|video|desktop-extras)-integration$"))) and all(.sources[]; (.sha256|test("^[a-f0-9]{64}$")) and (.url|(startswith("https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/") or startswith("https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-consolidation-archive-20261101/")))))' packaging/families/integrations.json >/dev/null
jq -e 'all(.families[]|select(has("debug_owner")); . as $family|any(.sources[];.name==$family.debug_owner))' packaging/families/integrations.json >/dev/null
jq -e '.families[]|select(.name=="uke-additional")|(.suggests|index("uke-vaapi-v4l2")!=null) and (.watch_paths|index("uke-video/packaging")!=null)' packaging/families/families.json >/dev/null
[[ $(rpmspec --target aarch64 -q --srpm --qf '%{NAME}' uke-video/packaging/uke-vaapi-v4l2.spec) == uke-vaapi-v4l2 ]]
jq -e 'all(.[]; (.sha256|test("^[a-f0-9]{64}$")) and (.url|(startswith("https://download.copr.fedorainfracloud.org/results/mcc45tr/") or (startswith("https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-migration-archive-20261020/") and endswith(".src.rpm")))))' packaging/families/source-inputs.json >/dev/null
jq -e '(.packages|length==5) and all(.packages[]; .name!="uke-power-profile-backend" and (.sha256|test("^[a-f0-9]{64}$")) and (.signed_mirror_sha256|test("^[a-f0-9]{64}$")) and (.source_payload_sha256|test("^[a-f0-9]{64}$")) and (.family|test("^uke-(additional|power|sensors)$"))) and any(.withdrawn_packages[]; .name=="uke-power-profile-backend" and .automatic_build==false and .target_admitted==false)' uke-desktop-packaging/manifests/native-runtime.json >/dev/null
jq -e '. as $catalog | ["at-spi2-core","gstreamer1","libwacom","libstdcxx-uke-runtime"] | all(.[];. as $name | ($catalog.packages|map(.name)|index($name))==null and any($catalog.withdrawn_packages[];.name==$name and .automatic_build==false and .target_admitted==false))' uke-desktop-packaging/manifests/native-runtime.json >/dev/null
jq -e 'any(.[]; .name=="material-decoration" and .family=="uke-extras")' packaging/families/source-inputs.json >/dev/null
jq -e 'any(.[]; .name=="uke-mesa-libclc22-spirv" and .family=="uke-additional" and (.url|startswith("https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-migration-archive-20261020/")))' packaging/families/source-inputs.json >/dev/null
jq -e '.rename["material-decoration"]=="uke-material-decoration" and (.exceptions|length)==4' packaging/families/families.json >/dev/null
jq -e '.families[]|select(.name=="uke-hw")|.requires|index("bluez")!=null and index("libinput")!=null and index("libwacom")!=null' packaging/families/families.json >/dev/null
jq -e 'all(.[]|select(.name=="pipewire" or .name=="uke-speaker-orientation" or .name=="uke-speaker-firmware"); .family=="uke-sound") and ([.[]|select(.name=="pipewire" or .name=="uke-speaker-orientation" or .name=="uke-speaker-firmware")]|length==3)' packaging/families/source-inputs.json >/dev/null
for script in packaging/families/host/*.sh; do bash -n "$script"; done
for name in uke-core uke-hw uke-power uke-sensors uke-extras uke-additional uke-metas uke-camera uke-sound; do
    spec="packaging/families/build/$name/SPECS/$name.spec"
    [[ -f $spec ]] || continue
    rpmspec --target aarch64 -q --qf '%{NAME}\n' "$spec" >"$spec.names"
    if grep -Ev '^uke-[A-Za-z0-9+_.-]+$' "$spec.names"; then exit 1; fi
    [[ $(rpmspec --target aarch64 -q --srpm --qf '%{NAME}' "$spec") == "$name" ]]
    ! grep -q '/__UKE_' "$spec"
    ! grep -qE '^(Requires|Recommends):.*(python|pypy)' "$spec"
done
hw_spec=packaging/families/build/uke-hw/SPECS/uke-hw.spec
if [[ -f $hw_spec ]]; then
    for name in adsp-firmware wpss-firmware wlan-board wlan-regdb; do
        grep -Fxq "Obsoletes: uke-owner-$name <= 3.0.304.0-1.owner.fc46" "$hw_spec"
    done
fi
power_spec=packaging/families/build/uke-power/SPECS/uke-power.spec
if [[ -f $power_spec ]]; then
    rpmspec --target aarch64 -q --qf '[%{OBSOLETENAME}\t%{OBSOLETEVERSION}\n]' "$power_spec" | grep -Fxq $'uke-power-profile-backend\t1.3.2'
    rpmspec --target aarch64 -q --qf '[%{CONFLICTNAME}\n]' "$power_spec" | grep -Fxq uke-power-profile-backend
    grep -Fxq 'Requires: uke-kernel(platform-profile-qos-v1) >= 1' "$power_spec"
    grep -Fxq 'Requires: power-profiles-daemon' "$power_spec"
    grep -Fxq 'old=uke-power-profile-backend; new=uke-power; unit=power-profiles-daemon.service; phase=save' "$power_spec"
    grep -Fxq 'old=uke-power-profile-backend; new=uke-power; unit=power-profiles-daemon.service; phase=restore' "$power_spec"
fi
[[ ! -e uke-desktop-packaging/packaging/fastfetch ]]
boot_spec=uke-boot/packaging/uke-boot-integration.spec
if rpmspec --target aarch64 -q --qf '%{NAME}\n' "$boot_spec" | grep -Fxq uke-wifi-factory; then exit 1; fi
rpmspec --target aarch64 -q --qf '[%{PROVIDENAME}\n]' "$boot_spec" | grep -Fx 'uke-boot-integration(factory-wlan-v1)'
if rpmspec --target aarch64 -q --qf '%{NAME}\n' "$boot_spec" | grep -Fxq uke-bluetooth; then exit 1; fi
rpmspec --target aarch64 -q --qf '[%{PROVIDENAME}\n]' "$boot_spec" | grep -Fx 'uke-boot-integration(factory-bluetooth-v1)'
jq -e 'all(.[]; .name!="uke-bluetooth")' packaging/families/source-inputs.json >/dev/null
jq -e 'all(.[]; .name!="uke-touchscreen")' packaging/families/source-inputs.json >/dev/null
rpmspec --target aarch64 -q --qf '[%{PROVIDENAME}\n]' "$boot_spec" | grep -Fx 'uke-boot-integration(touch-runtime-v1)'
if rpmspec --target aarch64 -q --qf '%{NAME}\n' "$boot_spec" | grep -Fxq uke-touchscreen; then exit 1; fi
[[ ! -e uke-desktop-packaging/src/fastfetch/config.jsonc && ! -e uke-boot/src/profiles/fastfetch.jsonc ]]
jq -e '.families[]|select(.name=="uke-hw")|(.requires|index("fastfetch")!=null) and all(.requires[]; (startswith("uke-fastfetch-config")|not))' packaging/families/families.json >/dev/null
if [[ -f $hw_spec ]]; then
    ! rpmspec --target aarch64 -q --qf '%{NAME}\n' "$hw_spec" | grep -Fxq uke-fastfetch-config
    rpmspec --target aarch64 -q --qf '[%{OBSOLETENAME}\t%{OBSOLETEVERSION}\n]' "$hw_spec" | grep -Fxq $'uke-fastfetch-config\t1.0.1-4.fc46'
    ! grep -Fq /etc/fastfetch/ "$hw_spec"
fi
grep -Fx "Version: $(jq -er '.families[]|select(.name=="uke-core")|.version' packaging/families/families.json)" uke-control/packaging/uke-control.spec
grep -Fx 'Version: 1.0.16' uke-desktop-integration/packaging/uke-adaptive-display.spec
grep -Fx 'Name: uke-plymonth' uke-desktop-packaging/packaging/plymouth-uke/plymouth-uke.spec
if [[ -e packaging/families/build/uke-additional/SPECS/uke-additional.spec ]]; then
    rpmspec --target aarch64 -q --qf '%{NAME}\n' packaging/families/build/uke-additional/SPECS/uke-additional.spec | grep -Fxq uke-vaapi-v4l2
    rpmspec --target aarch64 -q --qf '%{NAME}\n' packaging/families/build/uke-additional/SPECS/uke-additional.spec | grep -Fxq uke-mesa-libclc22-spirv
    grep -Fx "BuildRequires: llvm-devel = $(jq -er '.build_tools["llvm-devel"]' uke-desktop-packaging/manifests/native-runtime.json)" packaging/families/build/uke-additional/SPECS/uke-additional.spec
fi
printf '%s\n' 'PASS: family/source identity, prefix, pinned inputs, native versions and no Fastfetch fork.'
