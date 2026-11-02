#!/bin/bash
# Static source-contract checks only; never build or run device code.
set -euo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
jq -e 'length == 43 and (map(.name)|unique|length)==43 and all(.[]; .status != "ready")' "$root/packaging/ubuntu/coverage.json" >/dev/null
count=0
while IFS=$'\t' read -r package component version; do
 bash "$root/packaging/ubuntu/prepare-source.sh" "$package" "$work/$package"
 dsc=("$work/$package/"*.dsc)
 dpkg-source --no-check -x "${dsc[0]}" "$work/$package-extracted"
 stage=$work/$package-extracted
 debian=$stage/debian
 [[ $(dpkg-parsechangelog -l"$debian/changelog" -S Distribution) == tenacious ]]
 grep -qx 'Architecture: arm64' "$debian/control"
 ! grep -Eq 'dnf|rpm' "$debian/control"
 ! grep -Eq '^Depends:.*python' "$debian/control"
 find "$debian" -type f -name '*.postinst' -exec sh -n {} \;
 ! grep -ER --include=rules --include='*.postinst' --include='*.preinst' --include='*.postrm' --include='*.prerm' --include=postinst --include=preinst --include=postrm --include=prerm 'efibootmgr|flashrom|fastboot|update-initramfs|systemctl (start|enable)' "$debian"
 case "$package" in
  uke-speaker-firmware)
   bash "$root/packaging/ubuntu/verify-speaker-provenance.sh" "$stage"
   grep -qx '/usr/lib/firmware/qcom/uke/audio/fs19xx.fsm' "$debian/conffiles" ;;
  uke-image-tools)
   grep -qx 'Revision: 9e4d2153e116419c474d0c73eb2280447bf45243' "$stage/SOURCE-ORIGIN"
   [[ $(grep -c '\$(CXX).*build-tools/' "$debian/rules") == 6 ]]
   [[ ! -e $stage/guest-fedora-inputs.tar.xz ]] ;;
  uke-linux-kernel)
   for tool in package-boot-trial inspect-boot-trial inspect-embedded-initramfs validate-cohort verify-native-artifact-receipt; do
    cmp "$root/packaging/ubuntu/$tool.sh" "$debian/boot-trial-tools/$tool.sh"
   done
   cmp "$root/packaging/ubuntu/verified-artifacts/1006-native.json" "$debian/boot-trial-tools/1006-native.json"
   for tool in seed.sh verify.sh ubuntu.config; do cmp "$root/packaging/kernel-security/$tool" "$debian/kernel-security/$tool"; done
   bash "$root/packaging/kernel-security/test.sh" "$stage/inputs/boot-recipe/packaging/native/native.config"
   grep -Fxq 'export UKE_KERNEL_DISTRO := ubuntu' "$debian/rules"
   if UKE_KERNEL_DISTRO=fedora make -f "$debian/rules" -n override_dh_auto_configure > "$work/refused-distro" 2>&1; then exit 1; fi
   grep -Fq 'This Debian recipe requires UKE_KERNEL_DISTRO=ubuntu' "$work/refused-distro"
   mkdir -m700 "$work/kernel-keyring"
   gpg --batch --dearmor < "$stage/inputs/linux-stable.asc" > "$work/kernel-keyring/linux.gpg"
   [[ ! -e $stage/inputs/linux-7.3.1.tar.xz ]]
   (cd "$stage/inputs"; sha256sum --check --strict --quiet linux-source-parts.sha256)
   cat "$stage/inputs/"linux-7.3.1.tar.xz.part-* | xz -dc | gpgv --homedir "$work/kernel-keyring" --keyring "$work/kernel-keyring/linux.gpg" --status-fd=1 "$stage/inputs/linux-7.3.1.tar.sign" - > "$work/kernel-signature"
   grep -F "[GNUPG:] VALIDSIG $(jq -er .signer "$stage/manifests/linux-7.3.1.json") " "$work/kernel-signature"
   grep -qx 'Revision: fcbdae4b757732f0f0e3162c610f19abae290832' "$stage/SOURCE-ORIGIN"
   (cd "$stage/inputs/native-profile"; sha256sum --check --strict --quiet SHA256SUMS)
   (cd "$stage/inputs/boot-recipe"; jq -r '.source_assets[]|.sha256+"  "+.path' lock.json | sha256sum --check --strict --quiet)
   jq -e '.source_commit=="fcbdae4b757732f0f0e3162c610f19abae290832" and .parent_copr_build==11240068 and .physical_acceptance==false and (.overlays|length)==4' "$stage/inputs/native-profile/source-composition.json" >/dev/null
   release=$(sed -n 's/^release=//p' "$debian/build-kernel")
   [[ $release == 7.3.1-1006u2 && $((${#release}+3+1)) -le 16 ]]
   bash "$debian/boot-adapter/stage.sh" "$stage/inputs/boot-recipe" "$work/ubuntu-boot-recipe" > "$work/adapter-stage.log"
   jq -e '.native_executed==false and .security_configuration_changed==false and .boot_qualified==false' "$work/ubuntu-boot-recipe/ubuntu-adapter.json" >/dev/null
   UKE_BOOT_PROVENANCE_ADAPTER="$work/ubuntu-boot-recipe/src/boot/ubuntu-dpkg-provenance.sh" bash "$root/packaging/ubuntu/tests/boot-provenance.sh"
   bash "$root/packaging/ubuntu/tests/embedded-initramfs.sh" "$stage/inputs/boot-recipe/src/boot/dracut/root-guard.sh"
   cp -a "$stage/inputs/boot-recipe" "$work/corrupt-boot-recipe"
   printf '\nchanged source fixture\n' >> "$work/corrupt-boot-recipe/src/boot/dracut/root-guard.sh"
   if bash "$debian/boot-adapter/stage.sh" "$work/corrupt-boot-recipe" "$work/refused-adapter" > "$work/adapter-rejected.log" 2>&1; then echo 'Changed shared boot source admitted' >&2; exit 1; fi
   [[ ! -e $work/refused-adapter ]]
   ! grep -q '^Provides:' "$debian/control"
   status=0
   (cd "$stage"; UKE_KERNEL_DISTRO=ubuntu bash debian/build-kernel boot) 2> "$work/unsupported-boot" || status=$?
   [[ $status == 78 ]]
   grep -q 'Unsupported: root-prepared signed-host hwdb receipt' "$work/unsupported-boot"
   [[ ! -d $stage/build ]]
   bash -n "$debian/build-kernel" ;;
  uke-orangefox-recovery)
   (cd "$stage/release"; sha256sum -c PACKAGE-SHA256SUMS)
   [[ ! -e $debian/postinst && ! -e $debian/preinst && ! -e $debian/postrm && ! -e $debian/prerm ]]
   [[ -s $stage/release/SOURCE-OFFER.md && -d $stage/release/licenses ]]
   ! grep -q '^Provides:' "$debian/control" ;;
  uke-front-color) grep -q 'dh_installsystemd --no-enable --no-start' "$debian/rules" ;;
  iio-sensor-proxy)
   [[ $(wc -l < "$debian/patches/series") == 2 ]]
   grep -q "dual-light.h" "$stage/src/drv-ssc-light.c"
   grep -q "uke-sensorproxy-dual-light-abi-1" "$debian/control" ;;
  uke-vaapi-v4l2)
   [[ $(wc -l < "$debian/patches/series") == 6 ]]
   grep -q -- "-Dfastcv=disabled" "$debian/rules" ;;
  uke-battery-observer)
   grep -q 'integrated_protection_inventory' "$stage/src/battery/control-profile.cpp"
   grep -q 'Source-SHA256: 59ddb88df4139a1ba9e893ac23975ef6885582afcbfa1ed2d4d8b68c42573839' "$stage/RELEASE-ORIGIN"
   dpkg --compare-versions "$(dpkg-parsechangelog -l"$debian/changelog" -S Version)" ge 1.4.1
   grep -q "dh_installsystemd -puke-battery-observer --no-start" "$debian/rules"
   grep -q "dh_installsystemd -puke-charge-control --no-enable --no-start" "$debian/rules"
   jq -e ".enable_control==false and .qualification==\"unqualified\"" "$stage/manifests/control-profile.json" >/dev/null ;;
  uke-touchscreen)
   [[ $(sha256sum "$stage/src/upstream/nt36532-thp/nvt_touch_core.cpp" | cut -d " " -f1) == 7619b12a440be1f64cacb47b4b617fcfcf4486853822323f8d143c6e73aba9b1 ]]
   grep -q "uke-kernel-touch-thp-abi-1" "$debian/control"
   [[ ! -d $stage/var/lib/uke-touch ]] ;;
  upower)
   [[ $(wc -l < "$debian/patches/series") == 5 ]]
   grep -q "uke-upower-battery-abi-1" "$debian/control"
   grep -q -- "-Dinstalled_tests=false" "$debian/rules"
   dpkg --compare-versions "$(dpkg-parsechangelog -l"$debian/changelog" -S Version)" gt 1.91.5-1 ;;
  libssc)
   [[ $(wc -l < "$debian/patches/series") == 7 ]]
   grep -q "get_option('tests')" "$stage/meson.build"
   dpkg --compare-versions "$(dpkg-parsechangelog -l"$debian/changelog" -S Version)" gt 0.4.4-2 ;;
  uke-ssc-fileservice)
   [[ $(sha256sum "$stage/uke/src/ssc-cache.cpp" | cut -d ' ' -f1) == b135389fb30f54ea632db23f6bb1a0a5bd99cb399e19fd74725a791c68bf0afc ]]
   [[ $(sha256sum "$stage/uke/tests/ssc-cache.cpp" | cut -d ' ' -f1) == 1f0a06bfa6d8b95dcf8c801d6922de1e74792d10e5001ace7402237ee0f7feb3 ]]
   [[ $(wc -l < "$debian/patches/series") == 6 ]]
   cmp "$debian/uke-ssc-fileservice.udev" "$stage/uke/packaging/systemd/70-uke-ssc.rules"
   grep -q 'uke-sensorproxy-dual-light-abi-1' "$debian/control"
   grep -q 'uke-ssc-firmware-inputs-abi-1' "$debian/control" ;;
  *) grep -q 'dh_installsystemd --no-start' "$debian/rules" ;;
 esac
 # Verify declared static install sources; generated binaries are checked later.
 for list in "$debian/"*.install; do
  [[ -f $list ]] || continue
  while read -r pattern destination; do
   [[ $pattern == build/* || $pattern == usr/* || $pattern == etc/* ]] && continue
   compgen -G "$stage/$pattern" >/dev/null
  done < "$list"
 done
 (cd "$work/$package"; sha256sum -c SHA256SUMS)
 count=$((count+1))
done < "$root/packaging/ubuntu/components.tsv"
if bash "$root/packaging/ubuntu/prepare-source.sh" unknown "$work/unknown" 2>/dev/null; then exit 1; fi
if bash "$root/packaging/ubuntu/prepare-source.sh" uke-front-color "$work/uke-front-color" 2>/dev/null; then exit 1; fi
# No private per-device calibration package can enter public source preparation.
if bash "$root/packaging/ubuntu/prepare-source.sh" uke-local-touch-data "$work/private" 2>/dev/null; then exit 1; fi
printf 'PASS: %s source packages, extraction, patch application, metadata, lifecycle, checksums and rejection cases\n' "$count"
