#!/bin/bash
# Metadata/receipt refusal fixtures; no kernel source or target code executes.
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")/.."; pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
profile=pkg.uke-linux-kernel.artifact-trial
mkdir "$work/tree" "$work/cohort" "$work/evidence"
cp -a "$here/recipes/uke-linux-kernel/debian" "$work/tree/"
bash "$here/export-boot-trial-tools.sh" "$work/tree/debian/boot-trial-tools"
for script in package-boot-trial inspect-boot-trial inspect-embedded-initramfs validate-cohort verify-native-artifact-receipt; do
 cmp "$here/$script.sh" "$work/tree/debian/boot-trial-tools/$script.sh"
done
cmp "$here/verified-artifacts/1006-native.json" "$work/tree/debian/boot-trial-tools/1006-native.json"
(cd "$work/tree"; DEB_HOST_ARCH=arm64 DEB_BUILD_PROFILES= dh_listpackages | sort) > "$work/normal"
printf '%s\n' uke-kernel-core uke-kernel-dtbs uke-kernel-modules uke-kernel-boot | sort > "$work/normal-expected"
cmp "$work/normal" "$work/normal-expected"
(cd "$work/tree"; DEB_HOST_ARCH=arm64 DEB_BUILD_PROFILES=$profile dh_listpackages | sort) > "$work/artifact"
printf '%s\n' uke-kernel-trial-core uke-kernel-trial-dtbs uke-kernel-trial-modules uke-kernel-trial-boot | sort > "$work/artifact-expected"
cmp "$work/artifact" "$work/artifact-expected"
if (cd "$work/tree"; env -u UKE_TRIAL_COHORT -u UKE_TRIAL_EVIDENCE -u UKE_TRIAL_EARLY_MODULES -u UKE_TRIAL_LAYOUT DEB_BUILD_PROFILES=$profile bash debian/import-boot-trial prepare) > "$work/missing.log" 2>&1; then exit 1; else [[ $? == 78 ]]; fi
for action in prepare build install boot; do
 if (cd "$work/tree"; DEB_BUILD_PROFILES=$profile bash debian/build-kernel "$action") > "$work/refused.log" 2>&1; then exit 1; else [[ $? == 78 ]]; fi
done
[[ ! -e $work/tree/build ]]
: > "$work/files.jsonl"
while IFS=$'\t' read -r scope path; do
 case $scope in cohort|evidence) file=$work/$scope/$path;; early) file=$work/early-modules.list;; esac
 printf '%s\n' "$scope/$path fixture only" > "$file"
 jq -nc --arg scope "$scope" --arg path "$path" --arg hash "$(sha256sum "$file" | cut -d' ' -f1)" --argjson size "$(stat -c %s "$file")" '{scope:$scope,path:$path,sha256:$hash,size:$size}' >> "$work/files.jsonl"
done < <(jq -r '.files[]|[.scope,.path]|@tsv' "$here/verified-artifacts/1006-native.json")
jq -s '.' "$work/files.jsonl" > "$work/files.json"
jq --slurpfile files "$work/files.json" '.files=$files[0] | .test_fixture=true' "$here/verified-artifacts/1006-native.json" > "$work/fixture.json"
bash "$here/verify-native-artifact-receipt.sh" "$work/fixture.json" "$work/cohort" "$work/evidence" "$work/early-modules.list" > "$work/valid.log"
for mutation in '.architecture="amd64"' '.source_commit="0000000000000000000000000000000000000000"' '.original_zip_sha256="0000000000000000000000000000000000000000000000000000000000000000"' '.root_image_admitted=true' '.redistribution_admitted=true' '.files[0].path="../Image"' '.files|=.[1:]'; do
 jq "$mutation" "$work/fixture.json" > "$work/rejected.json"
 if bash "$here/verify-native-artifact-receipt.sh" "$work/rejected.json" "$work/cohort" "$work/evidence" "$work/early-modules.list" >/dev/null 2>&1; then exit 1; fi
done
printf changed >> "$work/cohort/.config"
if bash "$here/verify-native-artifact-receipt.sh" "$work/fixture.json" "$work/cohort" "$work/evidence" "$work/early-modules.list" >/dev/null 2>&1; then exit 1; fi
printf '%s\n' 'cohort/.config fixture only' > "$work/cohort/.config"
mv "$work/cohort/.config" "$work/real-config"
ln -s ../real-config "$work/cohort/.config"
if bash "$here/verify-native-artifact-receipt.sh" "$work/fixture.json" "$work/cohort" "$work/evidence" "$work/early-modules.list" >/dev/null 2>&1; then exit 1; fi
rm "$work/cohort/.config"
mv "$work/real-config" "$work/cohort/.config"
mv "$work/early-modules.list" "$work/real-early"
ln -s real-early "$work/early-modules.list"
if bash "$here/verify-native-artifact-receipt.sh" "$work/fixture.json" "$work/cohort" "$work/evidence" "$work/early-modules.list" >/dev/null 2>&1; then exit 1; fi
# Separate tiny stage fixtures exercise the post-Debhelper boundary. Their
# text is explicitly not a kernel, module, native output or hardware result.
version=$(dpkg-parsechangelog -l"$work/tree/debian/changelog" -SVersion)
for role in core dtbs modules boot; do
 package=uke-kernel-trial-$role
 expected=$work/tree/build/artifact-trial/expected/$package
 fixture=$work/package-$role
 mkdir -p "$expected/usr/share/uke-trial-fixture" "$fixture/DEBIAN"
 printf 'Fixture only; no kernel or hardware payload: %s\n' "$role" > "$expected/usr/share/uke-trial-fixture/$role"
 cp -a "$expected/." "$fixture/"
 {
  printf 'Package: %s\nVersion: %s\nArchitecture: arm64\nMaintainer: Test <test@example.invalid>\nDescription: inert byte-check fixture only\n' "$package" "$version"
  if [[ $role == boot ]]; then printf 'Depends: uke-kernel-trial-core (= %s), uke-kernel-trial-dtbs (= %s), uke-kernel-trial-modules (= %s)\n' "$version" "$version" "$version"; fi
 } > "$fixture/DEBIAN/control"
 dpkg-deb --build --root-owner-group "$fixture" "$work/${package}_${version}_arm64.deb" >/dev/null
done
(cd "$work/tree"; DEB_BUILD_PROFILES=$profile bash debian/import-boot-trial verify) > "$work/stage-check.log"
for scenario in provider hook linked-hook extra-file changed-file changed-mode loose-version; do
 fixture=$work/rejected-tree
 rm -rf "$fixture"
 cp -a "$work/package-boot" "$fixture"
 case $scenario in
  provider) echo 'Provides: uke-kernel-touch-thp-abi-1' >> "$fixture/DEBIAN/control";;
  hook) printf '#!/bin/sh\nexit 0\n' > "$fixture/DEBIAN/postinst"; chmod 755 "$fixture/DEBIAN/postinst";;
  linked-hook) ln -s control "$fixture/DEBIAN/postinst";;
  extra-file) mkdir -p "$fixture/usr/lib/systemd/system"; echo rejected > "$fixture/usr/lib/systemd/system/unexpected.service";;
  changed-file) printf corruption >> "$fixture/usr/share/uke-trial-fixture/boot";;
  changed-mode) chmod 755 "$fixture/usr/share/uke-trial-fixture/boot";;
  loose-version) sed -i 's/(= /(>= /g' "$fixture/DEBIAN/control";;
 esac
 dpkg-deb --build --root-owner-group "$fixture" "$work/uke-kernel-trial-boot_${version}_arm64.deb" >/dev/null
 if (cd "$work/tree"; DEB_BUILD_PROFILES=$profile bash debian/import-boot-trial verify) >/dev/null 2>&1; then exit 1; fi
done
echo 'Default/profile package sets, no-producer guard and receipt rejection fixtures passed; no payload qualification was inferred from fixtures.'
