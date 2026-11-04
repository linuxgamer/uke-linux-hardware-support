#!/usr/bin/env bash
# COPR only: reproduce the observed mask fault with real POSIX filesystem ACLs.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
node=$stage/synthetic-node
touch "$node"
chmod 0600 "$node"
test "$(id -u)" != 65534
setfacl -m u:65534:rw-,g::---,o::--- "$node"
snapshot() { getfacl --omit-header --numeric --absolute-names "$node"; }
before=$(snapshot)
grep -Fx 'user:65534:rw-' <<< "$before"
grep -Fx 'mask::rw-' <<< "$before"

# chmod updates ACL_MASK but preserves the existing named-user rw entry.
chmod 0600 "$node"
masked=$(snapshot)
grep -E '^user:65534:rw-[[:space:]]+#effective:---$' <<< "$masked"
grep -Fx 'mask::---' <<< "$masked"

# Use the actual package rule's mode, as applied before the uaccess builtin.
rule=$(sed -n '/KERNEL=="fm-ntag"/p' "$root/70-uke-nfc-ir.rules")
test "$(wc -l <<< "$rule")" = 1
mode=$(sed -n 's/.*MODE="\([0-7]\{4\}\)".*/\1/p' <<< "$rule")
test "$mode" = 0660
for attempt in 1 2; do
    chmod "$mode" "$node"
    repaired=$(snapshot)
    grep -Fx 'user:65534:rw-' <<< "$repaired"
    grep -Fx 'mask::rw-' <<< "$repaired"
    grep -Fx 'group::---' <<< "$repaired"
    grep -Fx 'other::---' <<< "$repaired"
done

# Mode repair does not prevent logind from revoking the named-user grant.
setfacl -x u:65534 "$node"
revoked=$(snapshot)
! grep -q '^user:65534:' <<< "$revoked"
test "$(stat -c %a "$node")" = 600
grep -Fx 'group::---' <<< "$revoked"
grep -Fx 'other::---' <<< "$revoked"
printf '%s\n' 'Real POSIX ACLs: masked named-user grant reproduced, exact NFC rule mode repairs it, other access stays zero and revocation succeeds; no device operation'
