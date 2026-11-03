#!/usr/bin/env bash
# Host-only source export; never executes the archived recipe or includes private data.
set -Eeuo pipefail
[[ $# == 3 && $3 =~ ^[a-f0-9]{40}$ ]] || {
    echo 'Usage: prepare-touch-runtime.sh pinned-source-rpm new-output-directory owner-commit' >&2; exit 2
}
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
bash "$root/tests/touch-capture-readiness.sh"
[[ ! -e $2 && ! -L $2 ]]
source_rpm=$(realpath "$1") output=$(realpath -m "$2")
[[ -f $source_rpm && ! -e $output ]]
manifest=$root/manifests/touch-runtime-source.json
jq -e '.schema==1 and .private_inputs==false' "$manifest" >/dev/null
printf '%s  %s\n' "$(jq -er .sha256 "$manifest")" "$source_rpm" | sha256sum -c - >&2
archive=$(jq -er .archive "$manifest") prefix=$(jq -er .root "$manifest")
[[ $archive == uke-touchscreen-0.1.0.tar.xz && $prefix == uke-touchscreen-0.1.0 ]]
mkdir -p "$(dirname -- "$output")"
stage=$(mktemp -d "$(dirname -- "$output")/.uke-touch-export.XXXXXX")
trap 'rm -rf -- "$stage"' EXIT
rpm2cpio "$source_rpm" > "$stage/source.cpio"
cpio --extract --quiet --to-stdout "./$archive" < "$stage/source.cpio" > "$stage/source.tar.xz"
tar -tJf "$stage/source.tar.xz" > "$stage/paths"
awk -v prefix="$prefix/" 'index($0,prefix)!=1 || /^\/|(^|\/)\.\.($|\/)/ {bad=1} END{exit bad}' "$stage/paths"
tar -tvJf "$stage/source.tar.xz" | awk 'substr($0,1,1)!="-" && substr($0,1,1)!="d"{bad=1} END{exit bad}'
mkdir "$stage/legacy"
tar -xJf "$stage/source.tar.xz" --strip-components=1 -C "$stage/legacy"
printf '%s  %s\n' \
    "$(jq -er .core_cpp_sha256 "$manifest")" "$stage/legacy/src/upstream/nt36532-thp/nvt_touch_core.cpp" \
    "$(jq -er .core_hpp_sha256 "$manifest")" "$stage/legacy/src/upstream/nt36532-thp/nvt_touch_core.hpp" | sha256sum -c - >&2
# Preserve the exact packaged standby correction in the legacy rollback path.
patch --batch --fuzz=0 -p1 -d "$stage/legacy" < "$root/../packaging/families/patches/touch-panel-start.patch"
mkdir -p "$stage/capture/upstream/nt36532-thp"
for name in touch-capture.cpp touch-capture.hpp touch-common.hpp touch-panel.hpp touch-input.hpp \
            touch-thp-packet.hpp touch-thp-geometry.hpp; do
    install -m644 "$root/src/$name" "$stage/capture/$name"
done
install -m644 "$stage/legacy/src/upstream/nt36532-thp/"{nvt_touch_core.cpp,nvt_touch_core.hpp,LICENSE} \
    "$stage/capture/upstream/nt36532-thp/"
install -m644 "$stage/legacy/LICENSE.GPL-3.0" "$stage/capture/"
install -m644 "$root/docs/KERNEL-TOUCH-CAPTURE.md" "$stage/capture/README.md"
printf '%s\n' "$3" > "$stage/owner-commit"
cp "$manifest" "$stage/source-lock.json"
(cd "$stage"; find legacy capture -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv -T -n -- "$stage" "$output"
[[ ! -e $stage ]]
trap - EXIT
printf 'Verified legacy and kernel-capture sources exported; no device action\n'
