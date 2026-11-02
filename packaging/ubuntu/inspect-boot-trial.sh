#!/bin/bash
# Read-only host admission checks. Never installs, loads or flashes a payload.
set -euo pipefail
export LC_ALL=C
[[ $# == 3 || $# == 4 ]] || { echo 'Usage: inspect-boot-trial.sh COHORT_DIRECTORY BUILD_EVIDENCE_DIRECTORY PINNED_EARLY_MODULE_LIST [EXACT_RELEASE]' >&2; exit 2; }
cohort=$(realpath "$1")
evidence=$(realpath "$2")
early_list=$(realpath "$3")
[[ $(sha256sum "$early_list" | cut -d ' ' -f1) == 17ae751bfb30c3b2ec9d10e7ab37deeb53b5231793cf5733ceb438438c23a6a6 ]]
for tool in openssl zstd modinfo fdtget sha256sum od dd cmp readelf perl bsdtar gzip; do command -v "$tool" >/dev/null; done
here=$(cd -- "$(dirname -- "$0")" && pwd)
release=${4:-7.3.1-1006u1}
[[ $release == 7.3.1-1006u1 || $release == 7.3.1-1006u2 ]]
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cd "$cohort"
[[ -f Image && -f module-signing.x509 && -f modules.tar.xz ]]
[[ $(od -An -tx1 -j56 -N4 Image | tr -d ' \n') == 41524d64 ]]
grep -aFq "Linux version $release " Image
config=config
[[ -f $config ]] || config=.config
if [[ -f config && -f .config ]]; then cmp config .config; fi
if [[ $release == 7.3.1-1006u2 ]]; then
 for key in SECURITY_APPARMOR SECURITY SECURITYFS SECURITY_NETWORK SECURITY_PATH AUDIT NET; do grep -Fxq "CONFIG_$key=y" "$config"; done
 grep -Fxq 'CONFIG_LSM="lockdown,yama,apparmor,bpf"' "$config"
fi
for key in MODULE_SIG MODULE_SIG_FORCE MODULE_SIG_ALL INITRAMFS_FORCE QCOM_UKE_ABL_DT; do
    grep -Fxq "CONFIG_$key=y" "$config"
done
grep -Fxq "kernel_release=$release" boot-manifest
grep -Fxq 'root_selector_capability=uke-elfnote-v2' boot-manifest
for pair in 'linux.img linux_image' 'userdata.img userdata_image' 'Image linux_kernel' 'Image userdata_kernel' 'modules.sha256 modules'; do
    read -r path key <<< "$pair"
    grep -Fxq "${key}_sha256=$(sha256sum "$path" | cut -d ' ' -f1)" boot-manifest
done
image_size=$(stat -c %s Image)
for image in linux.img userdata.img; do
    [[ $(stat -c %s "$image") == 100663296 ]]
    [[ $(head -c8 "$image") == 'ANDROID!' ]]
    read -ra fields <<< "$(od -An -tu4 -j8 -N16 --endian=little "$image")"
    [[ ${fields[0]} == "$image_size" && ${fields[1]} == 0 && ${fields[3]} == 1584 ]]
    [[ $(od -An -tu4 -j40 -N4 --endian=little "$image" | xargs) == 4 ]]
    [[ $(od -An -tu4 -j1580 -N4 --endian=little "$image" | xargs) == 0 ]]
    dd if="$image" iflag=skip_bytes,count_bytes skip=4096 count="$image_size" status=none | cmp Image -
done
[[ $(dd if=linux.img bs=1 skip=44 count=32 status=none | tr -d '\000') == '' ]]
[[ $(dd if=userdata.img bs=1 skip=44 count=32 status=none | tr -d '\000') == 'senemos.root=userdata' ]]
compat=$(fdtget -t s sm7675-xiaomi-uke.dtb / compatible)
[[ $compat == 'xiaomi,uke qcom,sm7675' ]]
# Extract only the expected module tree, never a path supplied by a manifest.
[[ -z $(tar -tvJf modules.tar.xz | sed -n '/^[d-]/!p') ]]
while IFS= read -r path; do
    [[ $path == lib/modules/"$release"/* && $path != *..* && $path != /* ]]
done < <(tar -tJf modules.tar.xz)
tar -xJf modules.tar.xz -C "$work" --no-same-owner --no-same-permissions
modules=$work/lib/modules/$release
[[ -d $modules/kernel ]]
while read -r hash path; do
    [[ $hash =~ ^[0-9a-f]{64}$ && $path == kernel/*.ko.zst && $path != *..* ]]
done < modules.sha256
(cd "$modules"; sha256sum -c "$cohort/modules.sha256" >/dev/null)
openssl x509 -inform DER -in module-signing.x509 -out "$work/certificate.pem"
# Binary substring matching cannot use shell variables (DER contains NUL).
# Perl is an essential host utility here; it is never copied into the payload.
perl -e 'local $/; open my $i, "<:raw", $ARGV[0] or die $!; my $image=<$i>; open my $c, "<:raw", $ARGV[1] or die $!; my $cert=<$c>; length($cert)>0 && index($image,$cert)>=0 or die "Cohort certificate is absent from Image\n";' Image module-signing.x509
checked=0
while IFS= read -r -d '' path; do
    [[ ! -L $path ]]
    zstd -q -d -c -- "$path" > "$work/module.ko"
    [[ $(modinfo -F vermagic "$work/module.ko") == "$release "* ]]
    readelf -h "$work/module.ko" | grep -Eq 'Machine:.*AArch64'
    size=$(stat -c %s "$work/module.ko")
    [[ $size -gt 40 ]]
    tail -c28 "$work/module.ko" | cmp - <(printf '~Module signature appended~\n')
    read -ra header <<< "$(od -An -tu1 -j$((size-40)) -N12 "$work/module.ko")"
    [[ ${header[*]:0:8} == '0 0 2 0 0 0 0 0' ]]
    sig_size=$(( (header[8]<<24) + (header[9]<<16) + (header[10]<<8) + header[11] ))
    content_size=$((size-40-sig_size))
    [[ $sig_size -gt 0 && $content_size -gt 64 ]]
    dd if="$work/module.ko" of="$work/unsigned.ko" iflag=count_bytes count="$content_size" status=none
    dd if="$work/module.ko" of="$work/signature.der" iflag=skip_bytes,count_bytes skip="$content_size" count="$sig_size" status=none
    # nointern ensures the supplied cohort certificate, not a certificate from
    # the signature itself, authenticates every module. No new key is created.
    openssl cms -verify -binary -inform DER -in "$work/signature.der" -content "$work/unsigned.ko" -nointern -certfile "$work/certificate.pem" -noverify -out /dev/null 2> "$work/cms.log"
    ((checked+=1))
done < <(find "$modules/kernel" -type f -name '*.ko.zst' -print0)
[[ $checked -gt 0 ]]
printf 'All %s module signatures match the certificate embedded in Image.\n' "$checked"
bash "$here/inspect-embedded-initramfs.sh" "$cohort/Image" "$work/initramfs"
early=$work/initramfs/usr/lib/modules/$release
[[ -d $early/kernel && ! -e $early/kernel/drivers/remoteproc/qcom_q6v5_pas.ko.zst ]]
[[ -z $(find "$work/initramfs/usr/lib/modules" -mindepth 1 -maxdepth 1 -type d ! -name "$release" -print -quit) ]]
early_modules=0
while IFS= read -r -d '' path; do
 cmp "$path" "$modules/${path#"$early/"}"
 grep -Fxq "${path#"$early/kernel/"}" "$early_list"
 ((early_modules+=1))
done < <(find "$early/kernel" -type f -name '*.ko.zst' -print0)
builtin_early=0
while IFS= read -r path; do
 [[ $path =~ ^[A-Za-z0-9_./-]+\.ko\.zst$ && $path != *..* ]]
 if [[ ! -f $early/kernel/$path ]]; then
  grep -Fxq "kernel/${path%.zst}" "$modules/modules.builtin"
  ((builtin_early+=1))
 fi
done < "$early_list"
[[ $((early_modules+builtin_early)) == 90 ]]
printf 'All 90 pinned early drivers accounted for: %s signed modules, %s built in; no extra early module.\n' "$early_modules" "$builtin_early"
[[ -f $evidence/payload-provenance.tsv && -f $evidence/build-debs.tsv && -f $evidence/base-links.tsv ]]
base_files=0
while IFS=$'\t' read -r path digest kind origin; do
 [[ $path != path ]] || continue
 [[ $path == /* && $path != *..* && $digest =~ ^[0-9a-f]{64}$ ]]
 file=$work/initramfs$path
 [[ -f $file && ! -L $file && $(sha256sum "$file" | cut -d ' ' -f1) == "$digest" ]]
 case $kind in
  dpkg-owner|dpkg-content-match)
   awk -F '\t' -v package="$origin" '$1==package && ($3=="arm64" || $3=="all"){found++} END{exit found!=1}' "$evidence/build-debs.tsv" ;;
  shared-project-source|generated-metadata) :;;
  *) echo 'Unknown base payload provenance kind' >&2; exit 1;;
 esac
 ((base_files+=1))
done < "$evidence/payload-provenance.tsv"
base_links=0
omitted_links=0
while IFS=$'\t' read -r path target; do
 [[ $path != /* && $path != *..* && -n $path && -n $target ]]
 file=$work/initramfs/$path
 if [[ ! -L $file ]]; then
  # Exact1006 source is typec-v1: the full producer removes this one legacy
  # early-shell link. Its unit/source bytes remain checked above.
  [[ $path == usr/lib/systemd/system/initrd.target.wants/uke-cdc-shell.service && ! -e $file ]]
  ((omitted_links+=1))
 else
  [[ $(readlink "$file") == "$target" ]]
  ((base_links+=1))
 fi
done < "$evidence/base-links.tsv"
[[ $base_files -gt 0 && $base_links -gt 0 && $omitted_links == 1 ]]
printf 'Embedded base matches %s recorded file hashes and %s links; one typec-v1 legacy link is absent.\n' "$base_files" "$base_links"
printf 'Verified %s ARM64 modules against the cohort certificate; boot headers, compile-stage DT identity and manifest agree.\n' "$checked"
printf 'This host result does not admit a root image or establish tablet boot success.\n'
