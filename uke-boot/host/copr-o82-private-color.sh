#!/usr/bin/env bash
# COPR only: extend exact qualified1.8.93; no kernel build or target operation.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
export LC_ALL=C
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
manifest=$owner/manifests/o82-private-color-20261105.json
output=$(realpath -m "${1:?Pass the COPR source result directory}")
(cd "$owner"; jq -r '.inputs[]|.sha256+"  "+.path' "$manifest" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir -p "$output" "$work/rpm" "$work/outer" "$work/inner"
curl -fLsS --max-time 45 --get --data-urlencode ownername=mcc45tr \
 --data-urlencode projectname=uke-linux-unstable --data-urlencode packagename=uke-boot-integration \
 --data-urlencode limit=100 https://copr.fedorainfracloud.org/api_3/build/list > "$work/builds.json"
jq -e '.items|type=="array" and length>0' "$work/builds.json" >/dev/null
while IFS= read -r version; do
    [[ $version =~ ^[0-9]+([.][0-9]+)*-[0-9]+([.][a-zA-Z0-9]+)*$ ]]
    comparison=$(rpm --eval "%{lua:print(rpm.vercmp(\"$version\", \"1.8.94-1.fc46\"))}")
    [[ $comparison =~ ^-?[0-9]+$ && $comparison -lt 0 ]]
done < <(jq -r '.items[]|select(.source_package.name=="uke-boot-integration" and .source_package.url!=null)|
 select(.project_dirname=="uke-linux-unstable" or .source_package.version=="1.8.94-1.fc46")|.source_package.version' "$work/builds.json")
curl -fLsS --retry 2 --max-time 90 --proto '=https' --proto-redir '=https' \
 "$(jq -er .parent.url "$manifest")" -o "$work/parent.src.rpm"
printf '%s  %s\n' "$(jq -er .parent.sha256 "$manifest")" "$work/parent.src.rpm" | sha256sum -c --strict -
rpmkeys --checksig --nosignature --verbose "$work/parent.src.rpm"
rpm2cpio "$work/parent.src.rpm" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" > "$work/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$work/entries"
(cd "$work/rpm"; cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
extract()
{
    tar -tJf "$1" > "$work/entries"
    awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$work/entries"
    tar -xJf "$1" -C "$2"
}
extract "$work/rpm/uke-boot-integration-1.8.93.tar.xz" "$work/outer"
outer=$work/outer/uke-boot-integration-1.8.93
extract "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.93.tar.xz" "$work/inner"
native=$work/inner/uke-boot-integration-1.8.93
(cd "$native"; find src tests -type f ! -path src/kernel-update.cpp ! -path src/oem-video.hpp -print0 | sort -z | xargs -0 sha256sum) > "$work/retained.sha256"
printf '%s  %s\n' be9830d0859d8358cc5ace5dc4d5d4b0b2294c711b7942e49aa27645ac8aba0a "$native/src/kernel-update.cpp" \
 19b1897ee6de470013f7d3ea40fb5db6b97aa26e7ae9b2d77380308a7b552cdb "$native/src/oem-video.hpp" | sha256sum -c --strict -
git -C "$native" apply --check "$owner/patches/o82-private-color-dnf.patch"
git -C "$native" apply "$owner/patches/o82-private-color-dnf.patch"
printf '%s  %s\n' 47805a2cb4a3c2048454ba30afea9675811d452074cf17633c686f194ff1819f "$native/src/kernel-update.cpp" \
 f8af3591efe435bbe8f0749fa62ff139f23c242bf46210c15c6497322fac6a4b "$native/src/oem-video.hpp" | sha256sum -c --strict -
for file in src/o82-private-color.hpp tests/o82-private-color.cpp tests/o82-private-color.sh; do
    test ! -e "$native/$file"
    install -Dm644 "$owner/$file" "$native/$file"
done
(cd "$native"; sha256sum -c --strict --quiet "$work/retained.sha256")
[[ $(cat "$native/src/profiles/rollback.conf") == mode=enforced ]]
grep -Fxq '%config(noreplace) /etc/senemos/rollback.conf' "$work/rpm/uke-boot-integration.spec"
[[ $(grep -c 'uke_color::with_prepared' "$native/src/kernel-update.cpp") == 3 ]]
printf 'All other1.8.93 updater, RTC, policy, services and test sources retained byte-for-byte\n'

# Qualify the three-line kernel metadata layer on the actual complete12101 spec.
kernel_commit=$(jq -er .kernel_layer.commit "$manifest")
[[ $kernel_commit =~ ^[a-f0-9]{40}$ ]]
git -c init.defaultBranch=source init -q "$work/kernel-owner"
git -C "$work/kernel-owner" remote add origin https://github.com/MCC45TR/senemos-uke-kernel-mainline.git
git -C "$work/kernel-owner" fetch -q --depth=1 origin "$kernel_commit"
git -C "$work/kernel-owner" checkout -q --detach FETCH_HEAD
[[ $(git -C "$work/kernel-owner" rev-parse HEAD) == "$kernel_commit" ]]
while IFS=$'\t' read -r relative digest; do
    printf '%s  %s\n' "$digest" "$work/kernel-owner/$relative" | sha256sum -c --strict --quiet -
done < <(jq -r '.kernel_layer.assets[]|[.path,.sha256]|@tsv' "$manifest")
curl -fLsS --retry 2 --max-time 600 --proto '=https' --proto-redir '=https' \
 "$(jq -er .kernel_layer.parent_url "$manifest")" -o "$work/kernel.src.rpm"
printf '%s  %s\n' "$(jq -er .kernel_layer.parent_sha256 "$manifest")" "$work/kernel.src.rpm" | sha256sum -c --strict -
mkdir "$work/kernel-rpm" "$work/kernel-unpacked"
rpm2cpio "$work/kernel.src.rpm" | (cd "$work/kernel-rpm"; cpio -idm --quiet --no-absolute-filenames \
 '*senemos-uke-linux-kernel-mainline-unstable.spec' '*senemos-native-profile-7.3.1.tar.xz' '*senemos-uke-boot-recipe.tar.xz')
extract "$work/kernel-rpm/senemos-native-profile-7.3.1.tar.xz" "$work/kernel-unpacked"
extract "$work/kernel-rpm/senemos-uke-boot-recipe.tar.xz" "$work/kernel-unpacked"
bash "$work/kernel-owner/packaging/native/overlay-o82-memory731.sh" \
 "$work/kernel-unpacked/native-profile" "$work/kernel-unpacked/boot-recipe"
bash "$work/kernel-owner/tests/o82-private-dnf-layer.sh" "$work/kernel-owner" \
 "$work/kernel-unpacked/native-profile" "$work/kernel-rpm/senemos-uke-linux-kernel-mainline-unstable.spec"
printf 'Kernel metadata checked only; no revision, Image, module or boot package was built\n'

spec=$output/uke-boot-integration.spec
awk '
 /^%changelog$/ {history=1;print;print "* Thu Nov 05 2026 Uke contributors - 1.8.94-1";
   print "- Prepare admitted private O82 v5 data before kernel activation.";
   print "- Preserve legacy and dry checks, staging policy, RTC and rollback preferences.";print "";next}
 !history {gsub(/1[.]8[.]93/,"1.8.94")}
 /^%package -n uke-kernel-update$/ {
   print;print "Requires: uke-o82-color-profile >= 0.7.0-1";
   print "Provides: uke-kernel-update(o82-private-color-v5) = 1";packages++;next}
 /^%check$/ {checking=1}
 checking && /^%[a-z]/ && $0!="%check" {
   print "bash %{_builddir}/uke-boot-integration-1.8.94/work/uke-boot-integration/uke-boot-integration-1.8.94/tests/o82-private-color.sh";
   checking=0;checks++}
 /^%transfiletriggerin -n uke-kernel-update -- \/usr\/lib\/senemos$/ {trigger++}
 {print}
 END {if(packages!=1||checks!=1||trigger!=1)exit 1}' "$work/rpm/uke-boot-integration.spec" > "$spec"
mv "$native" "$work/inner/uke-boot-integration-1.8.94"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
find "$outer/inputs/uke-boot-integration" -maxdepth 1 -type f -name uke-boot-integration-1.8.93.tar.xz -delete
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.94.tar.xz" -C "$work/inner" uke-boot-integration-1.8.94
cp "$spec" "$outer/inputs/uke-boot-integration/uke-boot-integration.spec"
install -m644 "$manifest" "$outer/o82-private-color-source.json"
git -C "$owner" rev-parse HEAD > "$outer/o82-private-color-source-commit"
mv "$outer" "$work/outer/uke-boot-integration-1.8.94"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$output/uke-boot-integration-1.8.94.tar.xz" -C "$work/outer" uke-boot-integration-1.8.94
[[ $(rpmspec --define 'dist %{nil}' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$spec") == uke-boot-integration-1.8.94-1.fc46 ]]
printf 'Private O82 DNF userspace source prepared; native compilation/checks remain in COPR\n'
