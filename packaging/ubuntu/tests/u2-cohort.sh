#!/bin/bash
# Metadata and refusal tests only. A native cohort readback is separate.
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")/.."; pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cp -a "$here/recipes/uke-linux-kernel/debian" "$work/debian"
bash "$here/export-boot-trial-tools.sh" "$work/debian/boot-trial-tools"
for name in stage-kernel-cohort verify-u2-receipt; do cmp "$here/$name.sh" "$work/debian/boot-trial-tools/$name.sh"; done
cmp "$here/verified-artifacts/1006-apparmor.json" "$work/debian/boot-trial-tools/1006-apparmor.json"
for profile in '' pkg.uke-linux-kernel.verified-cohort; do
 (cd "$work"; DEB_HOST_ARCH=arm64 DEB_BUILD_PROFILES=$profile dh_listpackages | sort) > "$work/packages"
 printf '%s\n' uke-kernel-core uke-kernel-dtbs uke-kernel-modules uke-kernel-boot | sort > "$work/expected"
 cmp "$work/expected" "$work/packages"
done
if (cd "$work"; env -u UKE_VERIFIED_COHORT -u UKE_VERIFIED_EVIDENCE -u UKE_VERIFIED_EARLY_MODULES DEB_BUILD_PROFILES=pkg.uke-linux-kernel.verified-cohort bash debian/import-verified-cohort prepare) > "$work/missing" 2>&1; then exit 1; else [[ $? == 78 ]]; fi
for action in prepare build boot install; do
 if (cd "$work"; DEB_BUILD_PROFILES=pkg.uke-linux-kernel.verified-cohort bash debian/build-kernel "$action") > "$work/refused" 2>&1; then exit 1; else [[ $? == 78 ]]; fi
done
if (cd "$work"; DEB_BUILD_PROFILES='pkg.uke-linux-kernel.verified-cohort pkg.uke-linux-kernel.artifact-trial' make -f debian/rules -n override_dh_auto_configure) > "$work/mixed" 2>&1; then exit 1; fi
grep -Fq 'Select exactly one kernel artifact profile' "$work/mixed"
[[ ! -d $work/build ]]
# Default build must run the complete producer; retained diagnostic candidate
# compilation cannot silently replace the boot cohort.
awk '/ build\|boot\)/,/^ prepare\)/' "$work/debian/build-kernel" | grep -Fq 'src/boot/build-payload.sh'
if grep -q '^Provides:' "$work/debian/control"; then exit 1; else [[ $? == 1 ]]; fi
if find "$work/debian" -maxdepth 1 -type f | grep -Eq '\.(postinst|preinst|postrm|prerm)$'; then exit 1; else [[ $? == 1 ]]; fi
for script in "$here/"{stage-kernel-cohort,verify-u2-receipt}.sh "$work/debian/import-verified-cohort"; do bash -n "$script"; done
echo 'Complete source/artifact profile sets and producer refusal contracts pass; no device code executed.'
