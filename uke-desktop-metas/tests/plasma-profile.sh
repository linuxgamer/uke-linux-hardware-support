#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
profile=src/plasma.json
spec=packaging/uke-desktop-metas.spec
jq -e '
  .schema_version == 1 and .device == "uke" and .soc == "SM7675" and
  .package == "kde-plasma-uke-meta" and .architecture == "aarch64" and
  .boot_tested == false and .hardware_tested == false and
  .automatic_session_or_display_configuration == false and
  (.reference.commit | test("^[a-f0-9]{40}$")) and
  (.packages | length == (unique | length)) and
  all(.packages[]; test("^[A-Za-z0-9][A-Za-z0-9+_.-]*$") and
      (test("nabu|uke"; "i") | not)) and
  (["plasma-discover-packagekit", "PackageKit", "kio-admin", "dolphin-plugins", "chromium"] - .packages | length == 0)
' "$profile" >/dev/null
diff -u <(jq -r '.packages[]' "$profile" | LC_ALL=C sort) \
  <(awk '/^# BEGIN ORIGINAL FEDORA PLASMA PACKAGES$/ {selection=1; next}
         /^# END ORIGINAL FEDORA PLASMA PACKAGES$/ {selection=0}
         selection && /^Requires:/ {print $2}' "$spec" | LC_ALL=C sort)
if grep -Eq '^%(pre|post|preun|postun|pretrans|posttrans|trigger[a-z]*|transfiletrigger[a-z]*|filetrigger[a-z]*)([[:space:]]|$)' "$spec"; then
  echo 'Desktop selection must not contain activation scriptlets' >&2
  exit 1
fi
printf '%s\n' 'Plasma selection, dependency policy and login-manager preservation checks passed'
