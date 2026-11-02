#!/bin/bash
# Bind private artifact input to the independently verified AppArmor cohort.
set -euo pipefail
[[ $# == 4 ]]
receipt=$1; cohort=$(realpath "$2"); evidence=$(realpath "$3"); early=$(realpath "$4")
jq -e '
 .schema=="uke-verified-apparmor-cohort-v1" and .kernel_release=="7.3.1-1006u2" and
 .version=="7.3.1-1006ubuntu2~tenacious1" and .source_commit=="fcbdae4b757732f0f0e3162c610f19abae290832" and
 .adapter_commit=="0d979e3f7c76ac43cb09f7b261932169d113703e" and .producer_run==41390762590 and
 .zip_sha256=="077a07945fd4ca42cc4c781795ae38ca08b89f9889a491440bbf8b90f4998715" and
 .runtime_admitted==false and .redistribution_admitted==false and
 ([.files[]|.scope+"/"+.path]|unique|length)==(.files|length) and
 all(.files[]; (.scope|IN("cohort","evidence","early")) and
  (.path|test("^[A-Za-z0-9_:./+-]+$")) and (.path|startswith("/")|not) and
  (.path|contains("..")|not) and (.sha256|test("^[0-9a-f]{64}$")) and (.size|type=="number" and .>=0)) and
 ([.files[]|select(.scope=="cohort" and (.path|startswith("licenses/")|not))|.path]|sort)==
  (["Image","config","module-signing.x509","modules.tar.xz","sm7675-xiaomi-uke.dtb","linux.img","userdata.img","boot-manifest","boot-header.json","modules.sha256"]|sort) and
 ([.files[]|select(.scope=="evidence")|.path]|sort)==(["payload-provenance.tsv","build-debs.tsv","base-links.tsv"]|sort) and
 ([.files[]|select(.scope=="early")|.path])==["early-modules.list"] and
 ([.files[]|select(.scope=="cohort" and (.path|startswith("licenses/")))]|length)>20
' "$receipt" >/dev/null
while IFS=$'\t' read -r scope path hash size; do
 case $scope in cohort) file=$cohort/$path;; evidence) file=$evidence/$path;; early) file=$early;; esac
 [[ -f $file && ! -L $file && $(stat -c %s "$file") == "$size" ]]
 printf '%s  %s\n' "$hash" "$file" | sha256sum --check --strict --quiet -
done < <(jq -r '.files[]|[.scope,.path,.sha256,.size]|@tsv' "$receipt")
# Pin complete notice inventories, including aliases; extra private data cannot
# enter a snapshot merely by being placed beside the verified notices.
diff -u <(jq -r '.files[]|select(.scope=="cohort" and (.path|startswith("licenses/")))|.path' "$receipt" | sort) <(cd "$cohort"; find licenses -type f | sort)
diff -u <(jq -r '.license_aliases[]|[.path,.target]|@tsv' "$receipt" | sort) <(cd "$cohort"; find licenses -type l -printf '%p\t%l\n' | sort)
echo 'Pinned AppArmor cohort and complete retained notice inventory match.'
