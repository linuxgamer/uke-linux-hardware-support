#!/usr/bin/env bash
# Inspect RPM's expanded consumer, not the unexpanded shell source.
set -Eeuo pipefail
[[ $# == 1 && -f $1 ]]
test "$(grep -Fc "rpm -q --qf '%{NAME}'" "$1")" = 4
test "$(grep -Fc ') || exit 1' "$1")" = 4
if grep -Fq "rpm -q --qf 'uke-boot-integration'" "$1"; then exit 1; fi
printf '%s\n' 'Expanded RPM migration retains literal package query tags.'
