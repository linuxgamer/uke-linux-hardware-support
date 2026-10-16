#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
spec=packaging/uke-desktop-metas.spec
for desktop in gnome plasma-mobile phosh; do
  case $desktop in
    gnome) package=gnome-uke-meta; marker=GNOME; experimental=false;;
    plasma-mobile) package=plasma-mobile-uke-meta; marker='PLASMA MOBILE'; experimental=true;;
    phosh) package=phosh-uke-meta; marker=PHOSH; experimental=true;;
  esac
  profile="src/$desktop.json"
  jq -e --arg package "$package" --argjson experimental "$experimental" '
    .schema_version == 1 and .device == "uke" and .soc == "SM7675" and
    .distribution == "fedora" and .architecture == "aarch64" and
    .package == $package and (.experimental // false) == $experimental and
    .boot_tested == false and .hardware_tested == false and
    .automatic_session_or_display_configuration == false and
    (.packages | length > 0 and length == (unique | length)) and
    all(.packages[]; test("^[A-Za-z0-9][A-Za-z0-9+_.-]*$") and
        (test("nabu|uke"; "i") | not))
  ' "$profile" >/dev/null
  diff -u <(jq -r '.packages[]' "$profile" | LC_ALL=C sort) \
    <(awk -v begin="# BEGIN ORIGINAL FEDORA $marker PACKAGES" \
           -v end="# END ORIGINAL FEDORA $marker PACKAGES" '
        $0 == begin {selection=1; next}
        $0 == end {selection=0}
        selection && /^Requires:/ {print $2}
      ' "$spec" | LC_ALL=C sort)
  grep -Fx "%package -n $package" "$spec" >/dev/null
  grep -Fx "%files -n $package" "$spec" >/dev/null
  grep -F "src/$desktop.json" "$spec" >/dev/null
done
printf '%s\n' 'GNOME, Plasma Mobile and Phosh package selections match their RPM requirements'
