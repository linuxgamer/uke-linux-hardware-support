#!/usr/bin/env bash
# Prepare the reviewed video component source for the selected COPR channel.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.."; pwd)
output=$(realpath -m "${1:?source RPM output directory}")
cd "$root"
commit=$(jq -er '.upstream.commit|select(test("^[a-f0-9]{40}$"))' manifests/upstream-pins.json)
digest=$(jq -er '.upstream.archive_sha256|select(test("^[a-f0-9]{64}$"))' manifests/upstream-pins.json)
archive="source-cache/libva-v4l2-$commit.tar.gz"
mkdir -p source-cache "$output"/{SOURCES,SPECS,SRPMS,BUILD,BUILDROOT,RPMS}
if [[ ! -s $archive ]]; then
    curl --disable --fail --location --silent --show-error --proto '=https' --proto-redir '=https' \
        --connect-timeout 15 --max-time 180 \
        "https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-migration-archive-20261020/libva-v4l2-$commit.tar.gz" -o "$archive.part"
    printf '%s  %s\n' "$digest" "$archive.part" | sha256sum --check
    mv "$archive.part" "$archive"
fi
printf '%s  %s\n' "$digest" "$archive" | sha256sum --check
while IFS=$'\t' read -r patchfile hash; do
    printf '%s  %s\n' "$hash" "$patchfile" | sha256sum --check --status
    cp "$patchfile" "$output/SOURCES/"
done < <(jq -r '.patches[]|[.path,.sha256]|@tsv' manifests/upstream-pins.json)
cp "$archive" README.md tests/{runtime-caps.cpp,main10-contract.sh,rate-negotiation.sh,dimension-contract.sh,decode-error-contract.sh,shared-surface-cache-contract.sh} "$output/SOURCES/"
cp packaging/uke-vaapi-v4l2.spec "$output/SPECS/"
rpmbuild -bs --nodeps --define "_topdir $output" "$output/SPECS/uke-vaapi-v4l2.spec"
