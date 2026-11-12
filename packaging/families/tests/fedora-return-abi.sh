#!/usr/bin/env bash
# Inspect exact signed RPMs; never execute their libraries or helpers.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
manifest="$root/packaging/families/consolidation/fedora-return-inputs.json"
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
cache="$root/uke-desktop-packaging/source-cache/fedora-returns-20261101"
mkdir -p "$cache" "$stage/keys"
for tool in rpm rpmkeys rpm2cpio cpio jq readelf nm comm; do command -v "$tool" >/dev/null; done
for key in fedora-46.asc; do rpmkeys --dbpath "$stage/keys" --import "$root/packaging/families/keys/$key"; done
curl --fail --location --silent --show-error --max-time 60 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/pubkey.gpg -o "$stage/copr.asc"
rpmkeys --dbpath "$stage/keys" --import "$stage/copr.asc"
rpmkeys --dbpath "$stage/keys" --list | awk '{print toupper($1)}' | LC_ALL=C sort > "$stage/keys.actual"
jq -r '.key_fingerprints[]' "$manifest" | LC_ALL=C sort > "$stage/keys.expected"
cmp "$stage/keys.expected" "$stage/keys.actual"
assert_superset() { LC_ALL=C comm -23 "$1" "$2" > "$stage/missing"; [[ ! -s $stage/missing ]]; }
printf 'kept\nlost\n' > "$stage/control-old"
printf 'kept\n' > "$stage/control-new"
if assert_superset "$stage/control-old" "$stage/control-new"; then echo 'Missing-symbol negative control accepted' >&2; exit 1; fi
while IFS= read -r pair; do
    name=$(jq -er .original_name <<<"$pair")
    arch=$(jq -er .arch <<<"$pair")
    for side in original fork; do
        url=$(jq -er --arg side "$side" '.[$side].url' <<<"$pair")
        hash=$(jq -er --arg side "$side" '.[$side].sha256' <<<"$pair")
        [[ $hash =~ ^[a-f0-9]{64}$ ]]
        case $url in
            https://kojipkgs.fedoraproject.org/packages/*/data/signed/91211fce/*.rpm|https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/*.rpm) ;;
            *) echo 'Unreviewed RPM URL' >&2; exit 1;;
        esac
        file="$cache/${url##*/}"
        if [[ ! -s $file ]]; then
            if ! curl --fail --location --silent --show-error --max-time 180 "$url" -o "$file.part"; then
                rm -f -- "$file.part"
                [[ $side == fork ]] || { echo 'Original Fedora input is unavailable' >&2; exit 1; }
                # COPR may prune completed builds. The existing source archive
                # retains these exact signed bytes; hashes and all trust/ABI
                # assertions below are unchanged. Never substitute a newer RPM.
                archive=https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-consolidation-archive-20261101
                printf 'Pinned COPR input unavailable; reading archived bytes: %s\n' "${url##*/}" >&2
                curl --fail --location --silent --show-error --max-time 180 "$archive/${url##*/}" -o "$file.part"
            fi
            printf '%s  %s\n' "$hash" "$file.part" | sha256sum -c - >/dev/null
            mv "$file.part" "$file"
        fi
        printf '%s  %s\n' "$hash" "$file" | sha256sum -c - >/dev/null
        rpmkeys --dbpath "$stage/keys" --checksig "$file" > "$stage/signature"
        [[ $(rpm -qp --qf '%{ARCH}' "$file") == "$arch" ]]
        expected=$(jq -er --arg side "$side" 'if $side=="original" then .original_name else .fork_name end' <<<"$pair")
        [[ $(rpm -qp --qf '%{NAME}' "$file") == "$expected" ]]
        rpm2cpio "$file" > "$stage/$side.cpio"
        cpio -t --quiet < "$stage/$side.cpio" > "$stage/$side.members"
        awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}' "$stage/$side.members"
        rpm -qp --qf '[%{FILENAMES}\t%{FILECOLORS}\t%{FILEMODES:perms}\t%{FILELINKTOS}\n]' "$file" > "$stage/$side.files"
        rpm --dbpath "$stage/keys" -qp --scripts "$file" > "$stage/$side.scripts"
        rpm --dbpath "$stage/keys" -qp --qf '[%{FILENAMES}\t%{FILEMODES}\t%{FILEUSERNAME}\t%{FILEGROUPNAME}\t%{FILEFLAGS}\t%{FILELINKTOS}\t%{FILEDIGESTS}\n]' "$file" |
            awk -F '\t' '$1~/^\/etc\/|^\/usr\/lib\/systemd\/|^\/usr\/share\/dbus-1\//' > "$stage/$side.policy"
    done
    cmp "$stage/fork.scripts" "$stage/original.scripts"
    cmp "$stage/fork.policy" "$stage/original.policy"
    dsos=0; interfaces=0; datafiles=0
    while IFS=$'\t' read -r path color mode link; do
        if [[ $color == 2 && $path == /usr/lib64/*.so* && -z $link ]]; then
            for side in fork original; do
                member=$(awk -v p="${path#/}" '{f=$0;sub(/^[.]\//,"",f);if(f==p)print}' "$stage/$side.members")
                [[ -n $member && $member != *$'\n'* ]]
                cpio -i --to-stdout --quiet "$member" < "$stage/$side.cpio" > "$stage/$side.elf"
                readelf -h "$stage/$side.elf" | grep -Eq 'Machine:[[:space:]]+AArch64'
                readelf -d "$stage/$side.elf" | sed -n 's/.*(SONAME).*\[\(.*\)\].*/\1/p' > "$stage/$side.soname"
                nm -D --defined-only --format=posix "$stage/$side.elf" | awk '{print $1}' | LC_ALL=C sort -u > "$stage/$side.symbols"
            done
            cmp "$stage/fork.soname" "$stage/original.soname"
            assert_superset "$stage/fork.symbols" "$stage/original.symbols" || { printf 'Missing ABI: %s %s\n' "$name" "$path" >&2; cat "$stage/missing" >&2; exit 1; }
            ((dsos+=1))
        elif [[ $path == /usr/include/* || $path == /usr/lib64/pkgconfig/* || $path == /usr/share/gir-1.0/* || $path == /usr/lib64/girepository-1.0/* || $path == /usr/share/libwacom/* ]]; then
            [[ -z $link && $mode == -* ]] || continue
            member=$(awk -v p="${path#/}" '{f=$0;sub(/^[.]\//,"",f);if(f==p)print}' "$stage/fork.members")
            [[ -n $member ]] || continue
            cpio -i --to-stdout --quiet "$member" < "$stage/fork.cpio" > "$stage/fork.interface"
            cpio -i --to-stdout --quiet "$member" < "$stage/original.cpio" > "$stage/original.interface"
            cmp "$stage/fork.interface" "$stage/original.interface" || { printf 'Changed interface: %s %s\n' "$name" "$path" >&2; exit 1; }
            if [[ $path == /usr/share/libwacom/* ]]; then ((datafiles+=1)); else ((interfaces+=1)); fi
        fi
    done < "$stage/fork.files"
    checked=false;((dsos==0)) || checked=true
    printf 'UKE_FEDORA_RETURN_STATIC package=%s dsos=%s interfaces=%s data_files=%s signatures=true abi_floor_checked=%s payload_executed=false admitted_for_replacement=false\n' "$name" "$dsos" "$interfaces" "$datafiles" "$checked"
done < <(jq -c '.pairs[]' "$manifest")
