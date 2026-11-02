#!/bin/bash
# Compare the explicit gap plan with actual solver output; no target executes.
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")/.."; pwd)
results=${1:-$here/runtime-dependency-results.tsv}
plan=$here/kernel-abi-gap-plan.json
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
jq -e '
 .schema=="uke-kernel-abi-gap-plan-v1" and
 .source_commit=="fcbdae4b757732f0f0e3162c610f19abae290832" and .producer_run==41331215146 and
 .receipt=="verified-artifacts/1006-native.json" and
 .complete_runtime_closures_from_inert_artifacts==0 and
 .new_kernel_build_required_for_artifact_packaging==false and .runtime_providers_admitted==false and
 (.packages|length==11) and ([.packages[].package]|unique|length)==11 and
 all(.packages[]; .runtime_closed==false and (.gates|length>0) and
  (.existing_evidence|type=="string" and length>30) and (.remaining_work|length>0) and
  (.blocker_classes|length>0) and all(.blocker_classes[];
   IN("boot-integration","hardware-qualification","firmware-rights-or-local-input","userspace-port","security-integration")))
' "$plan" >/dev/null
awk -F '\t' '$3=="unresolved"{print $1}' "$results" | sort > "$work/actual"
jq -r '.packages[].package' "$plan" | sort > "$work/planned"
cmp "$work/actual" "$work/planned"
echo 'All eleven unresolved binary chains retain explicit gates; no kernel artifact was promoted to a runtime provider.'
