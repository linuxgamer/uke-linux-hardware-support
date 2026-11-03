#!/usr/bin/env bash
# Exercise the expanded RPM scriptlets with Bluetooth absent and touch present.
set -Eeuo pipefail
[[ $# == 1 && -f $1 ]]
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
mkdir -p "$work/bin" "$work/systemd"
for hook in pre posttrans; do
    awk -v hook="$hook" '
      $0 ~ "^%" hook "([[:space:]]+-n[[:space:]]+uke-boot-integration)?$" {copy=1;next}
      copy && /^%/ {copy=0}
      copy {print}
    ' "$1" | sed -e "s#/run/systemd/system#$work/systemd#g" -e "s#/run/uke-rpm-migration#$work/state#g" > "$work/$hook"
    test -s "$work/$hook"
done
cat > "$work/bin/rpm" <<'SH'
#!/bin/sh
test "$1" = -q && test "$2" = --qf && test "$3" = '%{NAME}' || exit 1
test "$4" = uke-touchscreen || exit 1
printf 'uke-touchscreen'
SH
cat > "$work/bin/systemctl" <<'SH'
#!/bin/sh
printf '%s\n' "$*" >> "$FIXTURE_LOG"
case $1 in
is-enabled) printf '%s\n' "$FIXTURE_ENABLED";;
is-active) printf '%s\n' "$FIXTURE_ACTIVE";;
start) test "${FIXTURE_START_FAIL:-0}" = 0;;
esac
SH
chmod 755 "$work/bin/"*
export PATH="$work/bin:$PATH" FIXTURE_LOG="$work/calls"
key=$work/state/uke-boot-integration-touch
export FIXTURE_ENABLED=enabled FIXTURE_ACTIVE=active
sh "$work/pre"
test -f "$key" && test ! -e "$work/state/uke-boot-integration"
export FIXTURE_ENABLED=disabled FIXTURE_ACTIVE=inactive
sh "$work/posttrans"
grep -Fx 'enable uke-touchscreen.service' "$work/calls"
grep -Fx 'start uke-touchscreen.service' "$work/calls"
test ! -e "$key"
: > "$work/calls"
sh "$work/pre"
sh "$work/posttrans"
! grep -Eq '^(enable|start) ' "$work/calls"
export FIXTURE_ENABLED=enabled FIXTURE_ACTIVE=active
sh "$work/pre"
export FIXTURE_ENABLED=masked FIXTURE_ACTIVE=inactive
: > "$work/calls"
sh "$work/posttrans"
! grep -Eq '^(enable|start) ' "$work/calls"
test ! -e "$key"
export FIXTURE_ENABLED=enabled FIXTURE_ACTIVE=active
sh "$work/pre"
export FIXTURE_ENABLED=disabled FIXTURE_ACTIVE=inactive FIXTURE_START_FAIL=1
if sh "$work/posttrans"; then echo 'Failed service restoration was hidden' >&2; exit 1; fi
test -f "$key"
printf 'Expanded migrations: absent Bluetooth does not skip touch; active/disabled/masked and failed restoration: PASS\n'
