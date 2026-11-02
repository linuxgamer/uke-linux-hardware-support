#!/bin/bash
# Read actual extracted candidate payloads. Never load a module, apply a DT,
# execute a candidate binary, create an initramfs, or select/flash a boot image.
set -euo pipefail
export LC_ALL=C
payload=$(realpath "${1:?Extracted merged candidate payload directory required}")
release=${2:?Exact expected kernel release required}
[[ $release == 7.3.1-1006u1 || $release == 7.3.1-1006u2 ]]
tree=$payload/usr/lib/uke-kernel-candidates/$release
if [[ $release == 7.3.1-1006u2 ]]; then
 for key in SECURITY_APPARMOR SECURITY SECURITYFS SECURITY_NETWORK SECURITY_PATH AUDIT NET; do grep -Fxq "CONFIG_$key=y" "$tree/config"; done
 grep -Fxq 'CONFIG_LSM="lockdown,yama,apparmor,bpf"' "$tree/config"
fi
[[ -d $tree && ! -L $tree ]]
[[ ! -e $payload/boot && ! -e $payload/usr/lib/modules && ! -e $payload/etc ]]
[[ ! -e $payload/usr/bin && ! -e $payload/usr/sbin && ! -e $payload/usr/libexec ]]
[[ -z $(find "$tree" ! -type d ! -type f -print -quit) ]]
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
for manifest in core.sha256 dtbs.sha256 modules.sha256; do
 [[ -s $tree/$manifest ]]
 # Reject escaped sha256sum names, absolute paths and traversal before reading.
 awk 'NF!=2 || $1 !~ /^[0-9a-f]+$/ || length($1)!=64 || $2 !~ /^[A-Za-z0-9_+./-]+$/ || $2 ~ /^\// || $2 ~ /(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$tree/$manifest"
 (cd "$tree"; sha256sum --check --strict "$manifest")
done
printf '%s\n' Image KERNEL-SOURCE-ORIGIN admission config kernel-release module-signing.x509 | sort > "$work/core-expected"
awk '{print $2}' "$tree/core.sha256" | sort > "$work/core-declared"
cmp "$work/core-expected" "$work/core-declared"
[[ $(awk '{print $2}' "$tree/dtbs.sha256") == sm7675-xiaomi-uke.dtb ]]
(cd "$tree"; find . -type f ! -name core.sha256 ! -name dtbs.sha256 ! -name modules.sha256 -printf '%P\n' | sort) > "$work/all-actual"
awk '{print $2}' "$tree/core.sha256" "$tree/dtbs.sha256" "$tree/modules.sha256" | sort > "$work/all-declared"
cmp "$work/all-actual" "$work/all-declared"
[[ $(cat "$tree/kernel-release") == "$release" ]]
cmp <(printf '%s\n' candidate_only=true bootable=false stable_admitted=false) "$tree/admission"
grep -qx 'Fedora source composition: 7.3.1-1006' "$tree/KERNEL-SOURCE-ORIGIN"
[[ $(od -An -tx1 -j56 -N4 "$tree/Image" | tr -d ' \n') == 41524d64 ]] || { echo 'Invalid ARM64 Image magic' >&2; exit 1; }
size=$(stat -c %s "$tree/Image")
((size>=64 && size<=1073741824))
reservation=$(od --endian=little -An -tu8 -j16 -N8 "$tree/Image" | tr -d ' ')
((reservation>=64 && reservation<=1073741824))
strings "$tree/Image" | grep -F "Linux version $release " >/dev/null
[[ $(fdtget -t s "$tree/sm7675-xiaomi-uke.dtb" / compatible) == 'xiaomi,uke qcom,sm7675' ]] || { echo 'Foreign DT compatible' >&2; exit 1; }
for required in CONFIG_ARM64=y CONFIG_QCOM_UKE_ABL_DT=y CONFIG_INITRAMFS_FORCE=y CONFIG_MODULE_SIG=y CONFIG_MODULE_SIG_FORCE=y; do
 grep -Fx "$required" "$tree/config"
done
grep -Fx 'CONFIG_INITRAMFS_SOURCE=""' "$tree/config"
openssl x509 -inform DER -in "$tree/module-signing.x509" -noout >/dev/null
[[ -s $tree/modules/modules.dep && -s $tree/modules/modules.builtin && -s $tree/modules/modules.order ]]
count=0
while IFS= read -r -d '' module; do
 [[ $module == "$tree/modules/kernel/"*.ko.zst ]]
 (ulimit -f 262144; zstd -dc -- "$module" > "$work/module.ko")
 [[ $(od -An -tx1 -N6 "$work/module.ko" | tr -d ' \n') == 7f454c460201 ]]
 [[ $(od --endian=little -An -tu2 -j18 -N2 "$work/module.ko" | tr -d ' ') == 183 ]]
 vermagic=$(modinfo -F vermagic "$module")
 [[ ${vermagic%% *} == "$release" ]]
 [[ $(modinfo -F sig_id "$module") == PKCS#7 ]]
 [[ -n $(modinfo -F signer "$module") && -n $(modinfo -F sig_key "$module") ]]
 count=$((count+1))
done < <(find "$tree/modules/kernel" -type f -print0)
((count>0)) || { echo 'Empty module candidate refused' >&2; exit 1; }
while IFS= read -r dependency_line; do
 read -ra dependencies <<< "${dependency_line//:/ }"
 for dependency in "${dependencies[@]}"; do
  [[ $dependency =~ ^kernel/[A-Za-z0-9_+./-]+\.ko\.zst$ && $dependency != *..* && -f $tree/modules/$dependency ]]
 done
done < "$tree/modules/modules.dep"
# Require a complete digest inventory, including module dependency metadata.
(cd "$tree"; find modules -type f -print | sort) > "$work/actual"
awk '{print $2}' "$tree/modules.sha256" | sort > "$work/declared"
cmp "$work/actual" "$work/declared"
printf '%s\n' 'PASS: candidate Image, DT, configuration, module digests/vermagic/signature metadata. Cryptographic module trust, initramfs, boot admission and physical acceptance remain pending.'
