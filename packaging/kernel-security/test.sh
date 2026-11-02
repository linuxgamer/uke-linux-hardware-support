#!/bin/bash
# Data-only profile/refusal tests; no Kconfig or upstream code executes here.
set -euo pipefail
baseline=${1:?unchanged native baseline required}
here=$(cd -- "$(dirname -- "$0")"; pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
before=$(sha256sum "$baseline")
bash "$here/seed.sh" --distro fedora --baseline "$baseline" --output-root "$work/output"
bash "$here/seed.sh" --distro ubuntu --baseline "$baseline" --output-root "$work/output"
cmp "$baseline" "$work/output/fedora/.config"
[[ $(sha256sum "$baseline") == "$before" ]]
for args in '' '--distro' '--distro arch' '--distro ubuntu --distro fedora'; do
 if bash "$here/seed.sh" $args --baseline "$baseline" --output-root "$work/rejected" >/dev/null 2>&1; then exit 1; fi
done
[[ ! -e $work/rejected ]]
if bash "$here/seed.sh" --distro ubuntu --baseline "$baseline" --output-root "$work/output" >/dev/null 2>&1; then exit 1; fi
if bash "$here/seed.sh" --distro fedora --baseline "$work/output/ubuntu/.config" --output-root "$work/rejected" >/dev/null 2>&1; then exit 1; fi
for distro in fedora ubuntu; do
 cp "$work/output/$distro/.config" "$work/corrupt"
 if [[ $distro == fedora ]]; then sed -i 's/# CONFIG_SECURITY_APPARMOR is not set/CONFIG_SECURITY_APPARMOR=y/' "$work/corrupt"; else sed -i 's/CONFIG_SECURITY_PATH=y/# CONFIG_SECURITY_PATH is not set/' "$work/corrupt"; fi
 if bash "$here/verify.sh" --distro "$distro" --baseline "$baseline" --config "$work/corrupt" >/dev/null 2>&1; then exit 1; fi
done
for mutation in 's/apparmor,bpf/selinux,apparmor,bpf/' 's/CONFIG_SECURITY_YAMA=y/# CONFIG_SECURITY_YAMA is not set/' 's/CONFIG_MODULE_SIG_FORCE=y/# CONFIG_MODULE_SIG_FORCE is not set/' 's/^CONFIG_CMDLINE=.*/CONFIG_CMDLINE="apparmor=0"/'; do
 sed "$mutation" "$work/output/ubuntu/.config" > "$work/corrupt"
 if bash "$here/verify.sh" --distro ubuntu --baseline "$baseline" --config "$work/corrupt" >/dev/null 2>&1; then exit 1; fi
done
cp "$work/output/ubuntu/.config" "$work/corrupt"
echo 'CONFIG_SECURITY_APPARMOR=y' >> "$work/corrupt"
if bash "$here/verify.sh" --distro ubuntu --baseline "$baseline" --config "$work/corrupt" >/dev/null 2>&1; then exit 1; fi
echo 'Explicit distro, isolated output, unchanged Fedora and Ubuntu security refusal fixtures passed.'
