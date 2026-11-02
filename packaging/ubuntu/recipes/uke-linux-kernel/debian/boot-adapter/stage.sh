#!/bin/bash
# Stage a bounded host-integration overlay. This never executes any shared
# source script, compiler, native helper, boot producer or device command.
set -euo pipefail
umask 022
input=$(realpath "${1:?verified source boot recipe required}")
out=${2:?new output directory required}
[[ -d $input && ! -e $out ]]
here=$(cd "$(dirname "$0")"; pwd)
printf '%s  %s\n' dc2f9a7f0ded526e191147402403b9f66998004831cdec85d7756070cb1d4df4 "$input/lock.json" | sha256sum -c -
jq -e 'all(.source_assets[]; (.sha256|test("^[0-9a-f]{64}$")) and (.path|test("^[A-Za-z0-9_+,./-]+$") and (test("(^/|(^|/)\\.\\.(/|$))")|not)))' "$input/lock.json" >/dev/null
(cd "$input"; jq -r '.source_assets[]|.sha256+"  "+.path' lock.json | sha256sum --check --strict --quiet)
cp -a "$input" "$out"
out=$(realpath "$out")
patch --fuzz=0 --dry-run --batch --forward --directory="$out" -p1 < "$here/ubuntu-host-integration.patch"
patch --fuzz=0 --batch --forward --directory="$out" -p1 < "$here/ubuntu-host-integration.patch"
install -m644 "$here/ubuntu-dpkg-provenance.sh" "$out/src/boot/ubuntu-dpkg-provenance.sh"
patch --fuzz=0 --dry-run --batch --forward --directory="$out" -p1 < "$here/tenacious-dracut.patch"
patch --fuzz=0 --batch --forward --directory="$out" -p1 < "$here/tenacious-dracut.patch"
patch --fuzz=0 --dry-run --batch --forward --directory="$out" -p1 < "$here/ubuntu-apparmor.patch"
patch --fuzz=0 --batch --forward --directory="$out" -p1 < "$here/ubuntu-apparmor.patch"
mkdir -p "$out/packaging/kernel-security"
cp "$here/../kernel-security/"* "$out/packaging/kernel-security/"
for helper in normalize-initramfs-accounts verify-initrd-release verify-hwdb; do
 install -m644 "$here/$helper.sh" "$out/src/boot/$helper.sh"
done
# Root admission, actual switch-root, configuration and runtime helper source
# remain byte-identical. The Ubuntu profile is an explicit build-time overlay;
# staging does not select it or execute Kconfig. Fedora sources are unchanged.
for source in src/boot/dracut/root-guard.sh src/boot/dracut/uke-userdata-grow.service src/boot/offline-switch-root.cpp packaging/native/native.config; do
 cmp "$input/$source" "$out/$source"
done
scratch=$(mktemp)
trap 'rm -f "$scratch"' EXIT
: > "$scratch"
for source in src/boot/make-initramfs-base.sh src/boot/build-payload.sh src/boot/dracut/uke-root-guard.service src/boot/dracut/module-setup.sh src/boot/ubuntu-dpkg-provenance.sh src/boot/normalize-initramfs-accounts.sh src/boot/verify-initrd-release.sh src/boot/verify-hwdb.sh tests/kernel-hardening-config-contract.sh packaging/kernel-security/seed.sh packaging/kernel-security/verify.sh packaging/kernel-security/ubuntu.config; do
 before=null
 if [[ -f $input/$source ]]; then before=$(sha256sum "$input/$source" | cut -d ' ' -f1); fi
 after=$(sha256sum "$out/$source" | cut -d ' ' -f1)
 jq -nc --arg path "$source" --arg before "$before" --arg after "$after" '{path:$path,original_sha256:(if $before=="null" then null else $before end),ported_sha256:$after}' >> "$scratch"
done
jq -s --arg patch_sha256 "$(sha256sum "$here/ubuntu-host-integration.patch" | cut -d ' ' -f1)" --arg compatibility_patch_sha256 "$(sha256sum "$here/tenacious-dracut.patch" | cut -d ' ' -f1)" --arg security_patch_sha256 "$(sha256sum "$here/ubuntu-apparmor.patch" | cut -d ' ' -f1)" '{schema:1,original_lock_sha256:"dc2f9a7f0ded526e191147402403b9f66998004831cdec85d7756070cb1d4df4",source_commit:"fcbdae4b757732f0f0e3162c610f19abae290832",patch_sha256:$patch_sha256,compatibility_patch_sha256:$compatibility_patch_sha256,security_patch_sha256:$security_patch_sha256,changes:.,native_executed:false,security_configuration_changed:false,explicit_security_profile_available:true,boot_qualified:false}' "$scratch" > "$out/ubuntu-adapter.json"
bash -n "$out/src/boot/make-initramfs-base.sh" "$out/src/boot/build-payload.sh" "$out/src/boot/ubuntu-dpkg-provenance.sh"
printf 'Staged Ubuntu host adapter only; no native source execution or boot qualification.\n'
