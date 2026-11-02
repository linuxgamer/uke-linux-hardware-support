#!/bin/bash
# Configuration data only. No upstream program, compiler or target executes.
set -euo pipefail
[[ $# == 6 && $1 == --distro && $3 == --baseline && $5 == --output-root ]] || exit 2
distro=$2; baseline=$(realpath "$4"); root=$(realpath -m "$6")
case $distro in ubuntu|fedora) :;; *) echo 'Explicit --distro ubuntu|fedora is required' >&2; exit 2;; esac
[[ -f $baseline && ! -L $4 ]]
here=$(cd -- "$(dirname -- "$0")"; pwd)
# Refuse another distro's output or already modified baseline as the source.
grep -Fxq '# CONFIG_SECURITY_APPARMOR is not set' "$baseline"
grep -Fxq 'CONFIG_SECURITY_SELINUX=y' "$baseline"
grep -Fxq 'CONFIG_LSM="lockdown,yama,selinux,bpf"' "$baseline"
out=$root/$distro
[[ ! -e $out && ! -L $root ]]
mkdir -p "$out"
if [[ $distro == fedora ]]; then
 cp "$baseline" "$out/.config"
else
 # Replace only exact fragment symbols, retaining every other baseline line.
 awk 'FNR==NR{if($0~/^CONFIG_[A-Z0-9_]+=/){key=$0;sub(/=.*/,"",key);keys[key]=1} else if($0~/^# CONFIG_[A-Z0-9_]+ is not set$/){key=$2;keys[key]=1} next}
 {key=$0;if($0~/^CONFIG_[A-Z0-9_]+=/)sub(/=.*/,"",key);else if($0~/^# CONFIG_[A-Z0-9_]+ is not set$/)key=$2; if(!(key in keys))print}' "$here/ubuntu.config" "$baseline" > "$out/.config"
 cat "$here/ubuntu.config" >> "$out/.config"
fi
bash "$here/verify.sh" --distro "$distro" --baseline "$baseline" --config "$out/.config"
printf 'distro=%s\nbaseline_sha256=%s\nseed_sha256=%s\n' "$distro" "$(sha256sum "$baseline" | cut -d' ' -f1)" "$(sha256sum "$out/.config" | cut -d' ' -f1)" > "$out/profile"
if [[ $distro == ubuntu ]]; then sha256sum "$here/ubuntu.config" | cut -d' ' -f1 > "$out/fragment.sha256"; fi
