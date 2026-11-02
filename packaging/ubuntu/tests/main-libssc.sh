#!/bin/bash
# Source assembly/refusal fixtures only; no compiler or upstream program runs.
set -euo pipefail
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
bash "$root/packaging/ubuntu/tests/sensor-build-architecture.sh"
for scenario in valid corrupt missing-patch broken-patch; do
 stage=$work/$scenario
 mkdir -p "$stage/source/referances/libssc" "$stage/source/patches"
 cp "$root/uke-sensors/referances/libssc/libssc-v0.4.4.tar.gz" "$stage/source/referances/libssc/"
 cp -a "$root/uke-sensors/patches/libssc" "$stage/source/patches/"
 case $scenario in
  corrupt) printf corruption >> "$stage/source/referances/libssc/libssc-v0.4.4.tar.gz";;
  missing-patch) rm "$stage/source/patches/libssc/0007-reject-invalid-proximity-values.patch";;
  broken-patch) printf 'invalid patch fixture\n' > "$stage/source/patches/libssc/0007-reject-invalid-proximity-values.patch";;
 esac
 if (cd "$stage"; bash "$root/packaging/ubuntu/prepare-main-libssc.sh") > "$stage/log" 2>&1; then
  [[ $scenario == valid ]]
  [[ -f $stage/upstream/src/libssc-sensor-proximity.c ]]
  [[ -z $(find "$stage/upstream" -name '*.orig' -o -name '*.rej') ]]
 else
  [[ $scenario != valid ]]
  [[ ! -e $stage/upstream ]]
 fi
 [[ -z $(find "$stage" -maxdepth 1 -name '.main-libssc.*') ]]
done
echo 'libssc pinned input and complete shared patch admission fixtures passed.'
