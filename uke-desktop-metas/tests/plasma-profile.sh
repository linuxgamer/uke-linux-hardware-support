#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
profile=src/plasma.json
spec=packaging/uke-desktop-metas.spec
[[ $(grep -c '^Recommends: uke-plasma-controls >= 1.0.0$' "$spec") == 1 ]]
! grep -q '^Requires:.*uke-plasma-controls' "$spec"
[[ $(grep -c '^Recommends: chromium$' "$spec") == 1 ]]
! grep -q '^Requires: chromium' "$spec"
jq -e '.KPlugin.Id == "org.senemos.uke.controls" and ."X-Plasma-NotificationArea" == true' src/plasma-applet/metadata.json >/dev/null
! grep -rE 'nabu-|/sys/|/dev/|systemctl|kscreen-doctor|busctl|kwriteconfig' src/plasma-applet/contents
jq -e '
  .schema_version == 1 and .device == "uke" and .soc == "SM7675" and
  .package == "uke-kde-plasma-meta" and .architecture == "aarch64" and
  .boot_tested == false and .hardware_tested == false and
  .automatic_session_or_display_configuration == false and
  .original_fedora_dependency_exception_date == "2026-10-14" and
  (.reference.commit | test("^[a-f0-9]{40}$")) and
  (.packages | length == (unique | length)) and
  all(.packages[]; test("^[A-Za-z0-9][A-Za-z0-9+_.-]*$") and
      (test("nabu|uke|python|pypy"; "i") | not)) and
  (["plasma-discover-packagekit", "PackageKit", "kio-admin", "dolphin-plugins", "chromium", "plasma-login-manager", "kcm-plasmalogin"] - .packages | length == 0)
' "$profile" >/dev/null
diff -u <(jq -r '.packages[]' "$profile" | LC_ALL=C sort) \
  <(awk '/^# BEGIN ORIGINAL FEDORA PLASMA PACKAGES$/ {selection=1; next}
         /^# END ORIGINAL FEDORA PLASMA PACKAGES$/ {selection=0}
         selection && /^(Requires|Recommends):/ {print $2}' "$spec" | LC_ALL=C sort)
# The console meta conflicts with Python; Plasma must not pull it indirectly.
if awk '/^%package -n uke-kde-plasma-meta$/ {selection=1; next}
       /^%description -n uke-kde-plasma-meta$/ {selection=0}
       selection && /^(Requires|Recommends):.*uke-(core-meta|desktop-metas)/ {bad=1}
       END {exit !bad}' "$spec"; then
  echo 'Plasma must not require the strict console meta' >&2
  exit 1
fi
# Only the vendor firewall policy is reloaded; Fedora owns session autostart.
diff -u <(printf '%s\n' '%posttrans -n uke-kde-plasma-meta' '%postun -n uke-kde-plasma-meta') \
  <(grep -E '^%(pre|post|preun|postun|pretrans|posttrans|trigger[a-z]*|transfiletrigger[a-z]*|filetrigger[a-z]*)([[:space:]]|$)' "$spec")
policy=src/uke-kdeconnect.xml
xmllint --nonet --noout "$policy"
[[ $(xmllint --xpath 'boolean(
  /policy[@target="CONTINUE" and @priority < 0] and
  count(/policy/ingress-zone)=1 and /policy/ingress-zone/@name="ANY" and
  count(/policy/egress-zone)=1 and /policy/egress-zone/@name="HOST" and
  not(/policy/service | /policy/protocol | /policy/disable) and
  count(/policy/rule)=5 and
  count(/policy/rule[count(*)=3 and count(source)=1 and not(source/@invert) and count(service)=1 and service/@name="kdeconnect" and accept])=5 and
  count(/policy/rule[@family="ipv4"]/source[@address="10.0.0.0/8"])=1 and
  count(/policy/rule[@family="ipv4"]/source[@address="172.16.0.0/12"])=1 and
  count(/policy/rule[@family="ipv4"]/source[@address="192.168.0.0/16"])=1 and
  count(/policy/rule[@family="ipv6"]/source[@address="fe80::/10"])=1 and
  count(/policy/rule[@family="ipv6"]/source[@address="fc00::/7"])=1
)' "$policy") == true ]]
printf '%s\n' 'Original Fedora Plasma selection and local-only KDE Connect policy checks passed'
