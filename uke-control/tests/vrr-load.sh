#!/usr/bin/env bash
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/control" <<'SH'
#!/bin/sh
printf '%s\n' "$UKE_TEST_MINIMUM"
SH
cat > "$work/modinfo" <<'SH'
#!/bin/sh
printf '%s\n' o82_vrr_test_min_hz:uint
[ "${UKE_TEST_API:-1}" != 1 ] || printf '%s\n' o82_vrr_floor_api:uint
exit 0
SH
cat > "$work/modprobe" <<'SH'
#!/bin/sh
printf '<%s>\n' "$@"
SH
chmod 700 "$work/control" "$work/modinfo" "$work/modprobe"
sed -e "s|/usr/bin/uke-control|$work/control|" -e "s|/usr/sbin/modinfo|$work/modinfo|" \
    -e "s|/usr/sbin/modprobe|$work/modprobe|g" "$root/packaging/vrr-min-load" > "$work/load"
sh -n "$work/load"
for minimum in 30 90 143; do
    output=$(UKE_TEST_MINIMUM=$minimum sh "$work/load" preserved=1)
    [[ $output == *'<preserved=1>'* && $output == *"<o82_vrr_test_min_hz=$minimum>"* ]]
done
for minimum in unset 1 29 144 '90;false'; do
    output=$(UKE_TEST_MINIMUM=$minimum sh "$work/load" preserved=1)
    [[ $output == $'<--ignore-install>\n<msm>\n<preserved=1>' ]]
done
output=$(UKE_TEST_MINIMUM=90 UKE_TEST_API=0 sh "$work/load" preserved=1)
[[ $output == $'<--ignore-install>\n<msm>\n<preserved=1>' ]]
printf 'Production module loader: 30/90/143 Hz, preserved options, invalid/unset policy and old-kernel fallback: PASS\n'
