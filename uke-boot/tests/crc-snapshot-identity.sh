#!/usr/bin/env bash
# COPR only. The original package matrix runs the actual expanded GPT fixture.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
ulimit -c 0
work=$(mktemp -d);trap 'find "$work" -depth -delete' EXIT
cp -a src tests "$work/"
for mutation in entries-only allow-mismatched-record;do
 cp src/metadata-maintenance.hpp "$work/src/metadata-maintenance.hpp"
 cp src/slot-control.hpp "$work/src/slot-control.hpp"
 if [[ $mutation == entries-only ]];then
  sed 's/disk->backup_identity()/digest(disk->before.entries)/' src/metadata-maintenance.hpp > "$work/src/metadata-maintenance.hpp"
 else
  awk '/need\(file\(path\)==data,"Durable GPT backup differs from current metadata"\);/ {changed++;next} {print} END {if(changed!=1)exit 1}' src/slot-control.hpp > "$work/src/slot-control.hpp"
 fi
 for compiler in g++ clang++;do
  set +e
  (cd "$work";CXX=$compiler SANITIZE=0 bash tests/slot-control.sh) > "$work/$compiler.$mutation.log" 2>&1
  status=$?
  set -e
  [[ $status == 134 ]] || { cat "$work/$compiler.$mutation.log" >&2;exit 1; }
  printf 'Actual GPT snapshot regression refused: %s/%s/status134.\n' "$compiler" "$mutation"
 done
done
printf 'CRC snapshot identity controls pass; actual block-device qualification remains separate.\n'
