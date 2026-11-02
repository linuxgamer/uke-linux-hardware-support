#!/bin/bash
# Host packaging only. No boot selection, module loading or hardware ABI claim.
set -euo pipefail
export LC_ALL=C
[[ $# == 5 ]] || { echo 'Usage: stage-kernel-cohort.sh COHORT EVIDENCE EARLY_MODULE_LIST NEW_PACKAGE_ROOT BUILD_ORIGIN' >&2; exit 2; }
cohort=$(realpath "$1"); evidence=$(realpath "$2"); early=$(realpath "$3")
out=$(realpath -m "$4"); origin=$5
[[ ! -e $out && ( $origin == source-build || $origin == verified-artifact ) ]]
here=$(cd -- "$(dirname -- "$0")"; pwd)
release=7.3.1-1006u2
bash "$here/inspect-boot-trial.sh" "$cohort" "$evidence" "$early" "$release"
copyright=$here/recipes/uke-linux-kernel/debian/copyright
[[ -f $copyright ]] || copyright=$here/copyright
[[ -d $cohort/licenses && ! -L $cohort/licenses ]]
# The retained common-license aliases are the only nonregular license entries.
while IFS= read -r -d '' link; do
 case ${link#"$cohort/licenses/"}:$(readlink "$link") in
  common-licenses/GFDL:GFDL-1.3|common-licenses/GPL:GPL-3|common-licenses/LGPL:LGPL-3) :;;
  *) echo 'Unsupported license alias' >&2; exit 1;;
 esac
 [[ -f $(dirname "$link")/$(readlink "$link") && ! -L $(dirname "$link")/$(readlink "$link") ]]
done < <(find "$cohort/licenses" -type l -print0)
[[ -z $(find "$cohort/licenses" ! -type f ! -type d ! -type l -print -quit) ]]
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
tar -xJf "$cohort/modules.tar.xz" -C "$work" --no-same-owner --no-same-permissions
base=usr/lib/uke-kernel-cohorts/$release
mkdir -p "$out"
for role in core dtbs modules boot; do
 pkg=uke-kernel-$role
 root=$out/$pkg
 install -d "$root/usr/share/doc/$pkg" "$root/$base"
 case $role in
  core)
   install -m644 "$cohort/Image" "$cohort/module-signing.x509" "$root/$base/"
   install -m644 "$cohort/config" "$root/$base/config"
   printf '%s\n' "$release" > "$root/$base/kernel-release";;
  dtbs) install -m644 "$cohort/sm7675-xiaomi-uke.dtb" "$root/$base/";;
  modules)
   install -d "$root/usr/lib/modules"
   cp -a "$work/lib/modules/$release" "$root/usr/lib/modules/"
   [[ -z $(find "$root/usr/lib/modules" -type l -print -quit) ]];;
  boot)
   install -d "$root/$base/boot" "$root/$base/provenance"
   install -m644 "$cohort/"{linux.img,userdata.img,boot-manifest,boot-header.json,modules.sha256} "$root/$base/boot/"
   install -m644 "$evidence/"{payload-provenance.tsv,build-debs.tsv,base-links.tsv} "$root/$base/provenance/"
   printf '%s\n' 'singleboot_selector=userdata' 'singleboot_boot_payload=userdata.img' 'filesystem_label=UKE_LINUX' 'physical_partition_label=userdata' > "$root/$base/singleboot-pair"
   ;;
 esac
 install -m644 "$copyright" "$root/usr/share/doc/$pkg/copyright"
 cp -a "$cohort/licenses" "$root/usr/share/doc/$pkg/licenses"
 printf '%s\n' "build_origin=$origin" source_commit=fcbdae4b757732f0f0e3162c610f19abae290832 security_profile=ubuntu-apparmor automatic_boot_selection=false runtime_abi_admitted=false runtime_security_tested=false redistribution_admitted=false > "$root/usr/share/doc/$pkg/build-origin"
 (cd "$root"; find usr/lib -type f -print0 | sort -z | xargs -0 sha256sum > "usr/share/doc/$pkg/payload.sha256")
done
printf '%s\n' 'Complete matching core/DT/modules/boot package staging passed; activation and hardware admission are separate.'
