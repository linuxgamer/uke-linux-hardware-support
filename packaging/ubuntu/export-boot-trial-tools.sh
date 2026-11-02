#!/bin/bash
# Export shared host-only validation tools into a source package without forks.
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")"; pwd)
output=${1:?new output directory required}
[[ ! -e $output ]]
mkdir -p "$output"
for script in package-boot-trial inspect-boot-trial inspect-embedded-initramfs validate-cohort verify-native-artifact-receipt stage-kernel-cohort verify-u2-receipt verify-kernel-debs; do
 cp "$here/$script.sh" "$output/$script.sh"
done
cp "$here/verified-artifacts/1006-native.json" "$output/1006-native.json"
cp "$here/verified-artifacts/1006-apparmor.json" "$output/1006-apparmor.json"
cp "$here/recipes/uke-linux-kernel/debian/copyright" "$output/copyright"
