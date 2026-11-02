#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Source-only checks: no native compilation, RPM generation or COPR submission.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.."; pwd)
cd "$root"
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
jq -e '
  .schema_version==1 and .family=="freecad" and .name=="freecad" and
  (.version|test("^[0-9]+([.][0-9]+){2}$")) and
  (.epoch|type=="number") and (.release|type=="number") and
  (.upstream_commit|test("^[a-f0-9]{40}$")) and
  (.source_date_epoch|type=="number") and
  (.fedora_review_spec_sha256|test("^[a-f0-9]{64}$")) and
  .copr_project=="mcc45tr/uke-linux-unstable" and
  .chroots==["fedora-rawhide-aarch64"] and
  .binary_build_networking==false and .automatic_promotion==false and
  .hardware_tested==false and .python_target_payload==true and
  (.inputs|length==5) and
  ([.inputs[].filename]|unique|length)==5 and
  all(.inputs[];
    (.filename|test("^[a-zA-Z0-9_.-]+$")) and
    (.sha256|test("^[a-f0-9]{64}$")) and
    ((.url|startswith("https://github.com/FreeCAD/FreeCAD/")) or
      .url=="https://raw.githubusercontent.com/nlohmann/json/v3.11.3/LICENSE.MIT"))
' source-lock.json >/dev/null
expected=$(jq -r '[.name,(.epoch|tostring),.version,(.release|tostring)]|join("|")' source-lock.json)
actual=$(rpmspec --target aarch64 --undefine dist --query --srpm --queryformat '%{NAME}|%{EPOCHNUM}|%{VERSION}|%{RELEASE}' freecad.spec)
[[ $actual == "$expected" ]]
rpmspec --target aarch64 --parse freecad.spec >"$stage/expanded.spec"
awk '/^(Source[0-9]*|Patch[0-9]*):/ && $2 ~ /^https:/ {sub(/#\/.*/, "", $2); print $2}' "$stage/expanded.spec" | LC_ALL=C sort >"$stage/spec-inputs"
jq -r '.inputs[].url' source-lock.json | LC_ALL=C sort >"$stage/locked-inputs"
cmp "$stage/spec-inputs" "$stage/locked-inputs"
grep -Eq '^ExclusiveArch:[[:space:]]+aarch64$' "$stage/expanded.spec"
grep -F -- '-DFETCHCONTENT_FULLY_DISCONNECTED=ON' freecad.spec >/dev/null
grep -Fx '%ctest' freecad.spec >/dev/null
# Keep the pre-existing hardware-family hooks from submitting or promoting this
# optional application while the native unstable build is under review.
jq -e 'all(.families[]; .name!="freecad")' ../families.json >/dev/null
bash -n tests/source-contracts.sh
printf '%s\n' 'FreeCAD source pins, RPM identity and unstable-only family boundary passed.'
