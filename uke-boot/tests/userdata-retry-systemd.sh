#!/usr/bin/env bash
# Host user-manager fixture; never starts a device updater or reboot command.
set -Eeuo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
systemctl --user show-environment >/dev/null
runtime=${XDG_RUNTIME_DIR:?A running host user manager is required}
test -O "$runtime" && [[ $runtime =~ ^/[a-zA-Z0-9/_-]+$ ]]
stage=$(mktemp -d "$runtime/uke-ab-retry.XXXXXX")
unit="uke-ab-retry-test-$$"
units="$runtime/systemd/user"
mkdir -p "$units"
cleanup() {
    systemctl --user stop "$unit.service" "$unit-fallback.service" >/dev/null 2>&1 || :
    systemctl --user reset-failed "$unit.service" "$unit-fallback.service" >/dev/null 2>&1 || :
    rm -f -- "$units/$unit.service" "$units/$unit-fallback.service"
    rm -rf -- "$stage"
    systemctl --user daemon-reload
}
trap cleanup EXIT
cat >"$units/$unit.service" <<UNIT
[Unit]
OnFailure=$unit-fallback.service
$(grep -E '^(StartLimitIntervalSec|StartLimitBurst)=' src/units/uke-userdata-accept.service)
[Service]
Type=oneshot
ExecStart=/bin/sh -c 'printf a >> $stage/attempts; exit 1'
$(grep -E '^(Restart|RestartMode)=' src/units/uke-userdata-accept.service)
RestartSec=50ms
UNIT
cat >"$units/$unit-fallback.service" <<UNIT
[Service]
Type=oneshot
ExecStart=/bin/sh -c 'printf f >> $stage/fallback'
UNIT
systemctl --user daemon-reload
systemctl --user start --no-block "$unit.service"
for ((attempt=0; attempt<200; ++attempt)); do
    [[ ! -f $stage/fallback ]] || break
    sleep 0.05
done
test "$(cat "$stage/attempts")" = aaaaa
test "$(cat "$stage/fallback")" = f
test "$(systemctl --user show "$unit.service" --value --property=Result)" = start-limit-hit
printf '%s\n' 'Host systemd: five bounded starts then one OnFailure action; no device command or reboot executed'
