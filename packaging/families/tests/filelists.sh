#!/usr/bin/env bash
# Expand the generated manifests against a shared root with unrelated sibling files.
set -Eeuo pipefail
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
touch "$stage/rename"
for source in upower libaccounts-glib; do
    mkdir -p "$stage/$source"
    cat > "$stage/input" <<EOF
Name: $source
Version: 1
Release: 1.fc46
Summary: File ownership fixture
License: MIT
%description
Fixture.
%build
: __UKE_CWD__=fixture
: __UKE_BUILDROOT__=/buildroot
: __UKE_BUILDDIR__=/builddir
%files
EOF
    if [[ $source == upower ]]; then
        owned=(/usr/share/dbus-1/system.d/org.freedesktop.UPower.conf
               /usr/lib/udev/rules.d/60-upower-battery.rules
               /usr/lib/udev/hwdb.d/60-upower-battery.hwdb
               /usr/share/dbus-1/system-services/org.freedesktop.UPower.service
               /usr/lib/systemd/system/upower.service)
        patterns=(/usr/share/dbus-1/system.d/'*.conf' /usr/lib/udev/rules.d/'*.rules'
                  /usr/lib/udev/hwdb.d/'*.hwdb' /usr/share/dbus-1/system-services/'*.service'
                  /usr/lib/systemd/system/'*.service')
    else
        owned=(/usr/share/dbus-1/interfaces/com.google.code.AccountsSSO.Accounts.Manager.xml)
        patterns=(/usr/share/dbus-1/interfaces/'*.xml')
    fi
    printf '%s\n' "${patterns[@]}" >> "$stage/input"
    for file in "${owned[@]}"; do
        mkdir -p "$stage/root${file%/*}"
        touch "$stage/root$file" "$stage/root${file%/*}/foreign.${file##*.}"
    done
    awk -v out="$stage/$source" -v mapping="$stage/rename" -f host/split-spec.awk "$stage/input"
    : > "$stage/matched"
    while IFS= read -r pattern; do
        [[ $pattern == /* ]] || continue
        compgen -G "$stage/root$pattern" >> "$stage/matched"
    done < "$stage/$source/tail"
    ! grep -q '/foreign\.' "$stage/matched"
    for file in "${owned[@]}"; do grep -Fxq "$stage/root$file" "$stage/matched"; done
done
printf '%s\n' 'PASS: generated UPower and Accounts manifests retain their files and exclude shared-root siblings.'
