#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
c++ -std=c++17 -Wall -Wextra -Werror -O2 src/boot-status.cpp -o build/tests/uke-boot-status
fixture=$(mktemp -d build/tests/root.XXXXXX)
baseline=$fixture-baseline
trap 'rm -rf "$fixture" "$baseline"' EXIT
mkdir -p "$fixture/sys/firmware/efi" "$fixture/sys/firmware/devicetree/base" "$fixture/proc/sys/kernel"
printf 'xiaomi,uke\0qcom,sm7675\0' > "$fixture/sys/firmware/devicetree/base/compatible"
printf '7.2.9-senemos-uke\n' > "$fixture/proc/sys/kernel/osrelease"
printf 'root=PARTLABEL=uke_linux rw rootwait\n' > "$fixture/proc/cmdline"
build/tests/uke-boot-status --root "$fixture" > build/tests/accepted.json
jq -e '.efi_present and .uke_compatible and .selected_kernel_release and .uke_root_command_line and (.hardware_acceptance_granted|not)' build/tests/accepted.json >/dev/null
cp -a "$fixture" "$baseline"
printf '7.2.10-senemos-uke\n' > "$fixture/proc/sys/kernel/osrelease"
build/tests/uke-boot-status --identity --root "$fixture" > build/tests/identity-7.2.10.json
jq -e '.selected_kernel_release and (.hardware_acceptance_granted|not)' build/tests/identity-7.2.10.json >/dev/null
for release in 7.2.11-senemos-uke 7.2.10 7.2.10-senemos-nabu; do
    printf '%s\n' "$release" > "$fixture/proc/sys/kernel/osrelease"
    if build/tests/uke-boot-status --identity --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 2; fi
done
printf '7.2.9-senemos-uke\n' > "$fixture/proc/sys/kernel/osrelease"
printf 'root=LABEL=UKE_LINUX rw rootwait\n' > "$fixture/proc/cmdline"
build/tests/uke-boot-status --root "$fixture" > build/tests/filesystem-label.json
jq -e '.uke_root_command_line' build/tests/filesystem-label.json >/dev/null
printf 'root=PARTLABEL=linux rw rootwait\n' > "$fixture/proc/cmdline"
build/tests/uke-boot-status --root "$fixture" > build/tests/physical-partition-name.json
jq -e '.uke_root_command_line' build/tests/physical-partition-name.json >/dev/null
printf 'root=PARTLABEL=userdata rw rootwait\n' > "$fixture/proc/cmdline"
build/tests/uke-boot-status --root "$fixture" > build/tests/userdata-partition-name.json
jq -e '.uke_root_command_line and (.storage_verified|not) and (.hardware_acceptance_granted|not)' build/tests/userdata-partition-name.json >/dev/null
printf 'root=PARTLABEL=userdata root=LABEL=foreign rw\n' > "$fixture/proc/cmdline"
if build/tests/uke-boot-status --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 2; fi
printf 'root=LABEL=foreign root=PARTLABEL=userdata rw\n' > "$fixture/proc/cmdline"
build/tests/uke-boot-status --root "$fixture" >/dev/null
printf 'root=PARTLABEL=linux root=LABEL=foreign rw\n' > "$fixture/proc/cmdline"
if build/tests/uke-boot-status --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 2; fi
printf 'root=LABEL=UKE_LINUX root=LABEL=foreign rw\n' > "$fixture/proc/cmdline"
if build/tests/uke-boot-status --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 2; fi
printf 'root=LABEL=foreign root=LABEL=UKE_LINUX rw\n' > "$fixture/proc/cmdline"
build/tests/uke-boot-status --root "$fixture" >/dev/null
printf 'root=PARTLABEL=uke_linux rw rootwait\n' > "$fixture/proc/cmdline"
build/tests/uke-boot-status --identity --root "$fixture" > build/tests/identity.json
printf 'qcom,cliffs-mtp\0qcom,cliffs\0qcom,mtp\0' > "$fixture/sys/firmware/devicetree/base/compatible"
printf 'Qualcomm Technologies, Inc. Uke based on SM8635\0' > "$fixture/sys/firmware/devicetree/base/model"
build/tests/uke-boot-status --identity --root "$fixture" > build/tests/stock-identity.json
jq -e '.uke_compatible and .selected_kernel_release' build/tests/stock-identity.json >/dev/null
printf 'Qualcomm Technologies, Inc. Other based on SM8635\0' > "$fixture/sys/firmware/devicetree/base/model"
if build/tests/uke-boot-status --identity --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 2; fi
printf 'Qualcomm Technologies, Inc. Uke based on SM8635\0' > "$fixture/sys/firmware/devicetree/base/model"
printf 'qcom,cliffs-mtp\0qcom,mtp\0' > "$fixture/sys/firmware/devicetree/base/compatible"
if build/tests/uke-boot-status --identity --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 2; fi
rm "$fixture/sys/firmware/devicetree/base/model"
printf 'xiaomi,nabu\0qcom,sm8150\0' > "$fixture/sys/firmware/devicetree/base/compatible"
if build/tests/uke-boot-status --root "$fixture" > build/tests/foreign.json; then exit 1; else test "$?" -eq 2; fi
if build/tests/uke-boot-status --identity --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 2; fi
printf 'xiaomi,uke\0qcom,sm7675\0' > "$fixture/sys/firmware/devicetree/base/compatible"
printf 'root=PARTLABEL=uke_linux root=PARTLABEL=foreign rw\n' > "$fixture/proc/cmdline"
if build/tests/uke-boot-status --root "$fixture" > build/tests/foreign-root.json; then exit 1; else test "$?" -eq 2; fi
printf 'root=PARTLABEL=uke_linux rw rootwait\n' > "$fixture/proc/cmdline"
rmdir "$fixture/sys/firmware/efi"
if build/tests/uke-boot-status --root "$fixture" > build/tests/missing-efi.json; then exit 1; else test "$?" -eq 2; fi
mkdir "$fixture/sys/firmware/efi"
diff -r "$fixture" "$baseline" >/dev/null
if build/tests/uke-boot-status --cdc --root "$fixture" > build/tests/cdc-missing.json; then exit 1; else test "$?" -eq 2; fi
mkdir -p "$fixture/sys/class/udc/fixture-controller"
build/tests/uke-boot-status --cdc --root "$fixture" > build/tests/cdc-controller.json
jq -e '.single_udc_present and .usb_enumeration_tested==false and .hardware_acceptance_granted==false' build/tests/cdc-controller.json >/dev/null
if build/tests/uke-boot-status --cdc --require-tty --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 2; fi
mkdir "$fixture/sys/class/udc/second-controller"
if build/tests/uke-boot-status --cdc --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 2; fi
if build/tests/uke-boot-status --cdc --wait 31 --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 1; fi
if build/tests/uke-boot-status --cdc --wait 4294967296 --root "$fixture" >/dev/null; then exit 1; else test "$?" -eq 1; fi
build/tests/uke-boot-status --help >/dev/null
printf '%s\n' 'Boot prerequisite fixtures passed; foreign devices/root overrides rejected; inspection made no writes'
