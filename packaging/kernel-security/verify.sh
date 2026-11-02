#!/bin/bash
# Verify input or resolved Kconfig; never infer running policy enforcement.
set -euo pipefail
[[ $# == 6 && $1 == --distro && $3 == --baseline && $5 == --config ]] || exit 2
distro=$2; baseline=$4; config=$6
case $distro in ubuntu|fedora) :;; *) exit 2;; esac
[[ -f $baseline && ! -L $baseline && -f $config && ! -L $config ]]
here=$(cd -- "$(dirname -- "$0")"; pwd)
grep -Fxq '# CONFIG_SECURITY_APPARMOR is not set' "$baseline"
grep -Fxq 'CONFIG_LSM="lockdown,yama,selinux,bpf"' "$baseline"
# Duplicate settings can conceal a later override. Refuse ambiguous inputs.
for file in "$baseline" "$config"; do
 [[ -z $(sed -n -e 's/^\(CONFIG_[A-Z0-9_]*\)=.*/\1/p' -e 's/^# \(CONFIG_[A-Z0-9_]*\) is not set$/\1/p' "$file" | sort | uniq -d) ]]
done
while IFS= read -r line; do
 key=${line%%=*}; [[ $line != '# '* ]] || key=${line#'# '}; key=${key%% *}
 if [[ $distro == ubuntu ]]; then
  case $key in CONFIG_SECURITY_PATH|CONFIG_SECURITY_APPARMOR|CONFIG_DEFAULT_SECURITY_SELINUX|CONFIG_LSM) continue;; esac
 fi
 grep -Fxq "$line" "$config" || { printf 'Baseline protection changed: %s\n' "$line" >&2; exit 1; }
done < <(grep -E '^(# )?CONFIG_(SECURITY|DEFAULT_SECURITY|LSM|AUDIT|MODULE_SIG|LOCK_DOWN)[A-Z0-9_]*(=| is not set$)' "$baseline")
if [[ $distro == ubuntu ]]; then
 for symbol in SECURITY NET AUDIT SECURITYFS SECURITY_NETWORK SECURITY_PATH SECURITY_APPARMOR SECURITY_APPARMOR_INTROSPECT_POLICY SECURITY_APPARMOR_HASH SECURITY_APPARMOR_HASH_DEFAULT SECURITY_APPARMOR_EXPORT_BINARY SECURITY_APPARMOR_COMPRESSED_POLICY SECURITY_APPARMOR_PARANOID_LOAD DEFAULT_SECURITY_APPARMOR; do
  grep -Fxq "CONFIG_$symbol=y" "$config" || { echo "Required Ubuntu security feature missing: $symbol" >&2; exit 1; }
 done
 grep -Fxq 'CONFIG_LSM="lockdown,yama,apparmor,bpf"' "$config"
 while IFS= read -r line; do
  [[ $line =~ ^CONFIG_[A-Z0-9_]+= || $line =~ ^\#\ CONFIG_[A-Z0-9_]+\ is\ not\ set$ ]] || continue
  grep -Fxq "$line" "$config" || { printf 'Ubuntu AppArmor requirement missing: %s\n' "$line" >&2; exit 1; }
 done < "$here/ubuntu.config"
 # Forced command lines must not disable or supersede the selected MAC.
 if grep '^CONFIG_CMDLINE=' "$config" | grep -Eq '(^|[ ="])(apparmor=0|security=selinux|selinux=0|enforcing=0|lsm=)'; then
  echo 'Conflicting security command line refused' >&2; exit 1
 fi
else
 grep -Fxq '# CONFIG_SECURITY_APPARMOR is not set' "$config"
 grep -Fxq 'CONFIG_DEFAULT_SECURITY_SELINUX=y' "$config"
 grep -Fxq 'CONFIG_LSM="lockdown,yama,selinux,bpf"' "$config"
fi
printf 'Explicit %s kernel security configuration passed; runtime enforcement is untested.\n' "$distro"
