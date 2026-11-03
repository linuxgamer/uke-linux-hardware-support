#!/usr/bin/env bash
# Host-only trust/no-clobber checks of the actual source producer.
set -Eeuo pipefail
[[ $# == 1 ]] || exit 2
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
source_rpm=$(realpath "$1")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
owner=$(git -C "$root" rev-parse HEAD)
bash "$root/src/prepare-touch-runtime.sh" "$source_rpm" "$work/runtime" "$owner"
(cd "$work/runtime"; sha256sum -c SOURCE-SHA256SUMS >/dev/null)
cmp "$root/src/touch-capture.cpp" "$work/runtime/capture/touch-capture.cpp"
grep -F 'Waiting for active panel; no touch input device' "$work/runtime/legacy/src/touch-thp.cpp" >/dev/null
if find "$work/runtime/legacy" "$work/runtime/capture" -type f | grep -E '/(normal.bin|reference.bin|config.ini)$|\.py$'; then
    echo 'Unexpected private or Python payload' >&2; exit 1
fi
refuses() { if "$@" >"$work/refused.log" 2>&1; then echo 'Invalid source export accepted' >&2; exit 1; fi; }
before=$(sha256sum "$work/runtime/SOURCE-SHA256SUMS")
refuses bash "$root/src/prepare-touch-runtime.sh" "$source_rpm" "$work/runtime" "$owner"
[[ $before == "$(sha256sum "$work/runtime/SOURCE-SHA256SUMS")" ]]
printf 'invalid\n' > "$work/invalid.src.rpm"
refuses bash "$root/src/prepare-touch-runtime.sh" "$work/invalid.src.rpm" "$work/bad" "$owner"
[[ ! -e $work/bad ]]
ln -s "$work/new-target" "$work/dangling"
refuses bash "$root/src/prepare-touch-runtime.sh" "$source_rpm" "$work/dangling" "$owner"
[[ ! -e $work/new-target && -L $work/dangling ]]
printf 'Verified source export, private/Python exclusion, archive tampering, existing output and dangling-link refusal: PASS\n'
