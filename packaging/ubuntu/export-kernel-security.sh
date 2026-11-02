#!/bin/bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")"; pwd)
out=${1:?new destination required}
[[ ! -e $out ]]
mkdir -p "$out"
for file in seed.sh verify.sh ubuntu.config; do cp "$here/../kernel-security/$file" "$out/$file"; done
