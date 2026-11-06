#!/usr/bin/env bash
# COPR only: extend the entire qualified101 updater source capsule.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
export LC_ALL=C
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?Source result directory}")
[[ ${2:?Unique updater version} == 1.8.102 ]]
(cd "$owner";jq -r '.source_assets[]|.sha256+"  "+.path' manifests/crc-snapshot-identity-20261106.json | sha256sum -c --strict --quiet -)
work=$(mktemp -d);trap 'find "$work" -depth -delete' EXIT
[[ ! -e $output || (-d $output && -z $(find "$output" -mindepth 1 -maxdepth 1 -print -quit)) ]]
mkdir -p "$output" "$work/rpm" "$work/outer" "$work/inner"
curl -q -fLsS --retry 2 --max-time 180 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11269198/uke-boot-integration-1.8.101-1.fc46.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' 0c138d890796f905035adde060806267c707fe202d2a79dcc66865e87a438fbc "$work/parent.src.rpm" | sha256sum -c --strict -
rpm2cpio "$work/parent.src.rpm" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
(cd "$work/rpm";cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
tar -xJf "$work/rpm/uke-boot-integration-1.8.101.tar.xz" -C "$work/outer"
outer=$work/outer/uke-boot-integration-1.8.101
(cd "$outer";sha256sum -c --strict --quiet SOURCE-SHA256SUMS)
tar -xJf "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.101.tar.xz" -C "$work/inner"
native=$work/inner/uke-boot-integration-1.8.101
(cd "$native";find . -type f ! -path ./src/slot-control.hpp ! -path ./src/metadata-maintenance.hpp ! -path ./tests/slot-control.cpp -print0 | sort -z | xargs -0 sha256sum) > "$work/retained"
(cd "$native";patch --dry-run -p1 --fuzz=0 --batch -i "$owner/patches/crc-snapshot-identity.patch";patch -p1 --fuzz=0 --batch -i "$owner/patches/crc-snapshot-identity.patch")
(cd "$native";sha256sum -c --strict --quiet "$work/retained")
[[ $(cat "$native/src/profiles/rollback.conf") == mode=enforced ]]
[[ $(grep -Fx automatic=1 "$native/src/profiles/kernel-update.conf") == automatic=1 ]]
[[ $(grep -Fx automatic=1 "$native/src/profiles/userdata-update.conf") == automatic=1 ]]
install -m644 "$owner/tests/crc-snapshot-identity.sh" "$native/tests/crc-snapshot-identity.sh"
install -m644 "$owner/docs/CRC-SNAPSHOT-IDENTITY.md" "$native/docs/CRC-SNAPSHOT-IDENTITY.md"
spec=$output/uke-boot-integration.spec
awk '
 /^%changelog$/ {history=1;print;print "* Fri Nov 06 2026 Uke contributors - 1.8.102-1";
 print "- Identify immutable CRC maintenance journals by the complete observed snapshot.";
 print "- Preserve legacy journals, exact content refusal, slot state and local preferences.";print "";next}
 !history {gsub(/1[.]8[.]101/,"1.8.102")}
 /^Provides: uke-kernel-update\(alternating-checkpoint-v2\)/ {capability++}
 /^bash tests\/rollback-off-crc[.]sh$/ {print;print "bash tests/crc-snapshot-identity.sh";added++;next}
 {print} END {if(capability!=1 || added!=1)exit 1}
' "$work/rpm/uke-boot-integration.spec" > "$spec"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$spec") == uke-boot-integration-1.8.102-1.fc46 ]]
install -m644 "$spec" "$outer/inputs/uke-boot-integration/uke-boot-integration.spec"
mv "$native" "$work/inner/uke-boot-integration-1.8.102"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
unlink "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.101.tar.xz"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.102.tar.xz" -C "$work/inner" uke-boot-integration-1.8.102
git -C "$owner" rev-parse HEAD > "$outer/crc-snapshot-identity-source-commit"
printf 'parent_build=11269198\nparent_sha256=0c138d890796f905035adde060806267c707fe202d2a79dcc66865e87a438fbc\n' > "$outer/crc-snapshot-identity-parent"
cp "$outer/SOURCE-SHA256SUMS" "$outer/SOURCE-SHA256SUMS.before-crc-snapshot-identity"
(cd "$outer";find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > "$work/source-sha256")
install -m644 "$work/source-sha256" "$outer/SOURCE-SHA256SUMS"
mv "$outer" "$work/outer/uke-boot-integration-1.8.102"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.102.tar.xz" -C "$work/outer" uke-boot-integration-1.8.102
printf 'Complete101 updater retained; immutable snapshot102 native/package and device qualification pending.\n'
