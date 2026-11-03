#!/usr/bin/env bash
# Host fixture: real RPM scriptlets and Fedora's original theme selector.
set -Eeuo pipefail
package=$(realpath "${1:?binary theme RPM}")
rpm -qp --requires "$package" | grep -Fxq plymouth-plugin-label
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
mkdir -p "$stage/payload" "$stage/config" "$stage/policy"
rpm2cpio "$package" >"$stage/payload.cpio"
(cd "$stage/payload"; cpio -idm --quiet <"$stage/payload.cpio")
export PLYMOUTH_DATADIR="$stage/payload/usr/share"
export PLYMOUTH_CONFDIR="$stage/config/"
export PLYMOUTH_POLICYDIR="$stage/policy/"
export PLYMOUTH_PLUGIN_PATH
PLYMOUTH_PLUGIN_PATH=$(plymouth --get-splash-plugin-path)
export PATH="/usr/sbin:$PATH"
for theme in bgrt custom; do
    mkdir -p "$PLYMOUTH_DATADIR/plymouth/themes/$theme"
    printf '[Plymouth Theme]\nModuleName=script\n' >"$PLYMOUTH_DATADIR/plymouth/themes/$theme/$theme.plymouth"
done
printf '[Daemon]\nTheme=bgrt\n' >"$stage/policy/plymouthd.defaults"
printf '[Daemon]\nTheme=bgrt\nDeviceScale=2\n' >"$stage/config/plymouthd.conf"
rpm -qp --qf '%{POSTIN}\n' "$package" >"$stage/post"
rpm -qp --qf '%{PREUN}\n' "$package" >"$stage/preun"
sh "$stage/post" 1
[[ $(plymouth-set-default-theme) == senemos-uke ]]
grep -Fxq DeviceScale=2 "$stage/config/plymouthd.conf"
sh "$stage/preun" 1
[[ $(plymouth-set-default-theme) == senemos-uke ]]
sh "$stage/preun" 0
[[ $(plymouth-set-default-theme) == bgrt ]]
plymouth-set-default-theme custom
sh "$stage/post" 2
sh "$stage/preun" 0
[[ $(plymouth-set-default-theme) == custom ]]
theme="$PLYMOUTH_DATADIR/plymouth/themes/senemos-uke"
[[ $(identify -format '%wx%h' "$theme/xiaomi-logo.png") == 312x312 ]]
[[ $(identify -format '%wx%h' "$theme/fedora-logo.png") == 427x120 ]]
for item in start:boot poweroff:shutdown reboot:reboot halt:shutdown; do
    unit=${item%%:*}; mode=${item#*:}
    config="$stage/payload/usr/lib/systemd/system/plymouth-$unit.service.d/50-uke-theme.conf"
    grep -Fxq Environment=PLYMOUTH_FORCE_SCALE=1 "$config"
    grep -Fxq "ExecStart=/usr/bin/plymouthd --mode=$mode --pid-file=/run/plymouth/pid --attach-to-session --graphical-boot" "$config"
done
! rpm -qpl "$package" | grep -Eq '(^/boot/|[.]py[co]?$|/dev/|/dracut/|/nabu)'
printf '%s\n' 'PASS: real RPM activation, upgrade/removal, custom-theme preservation, artwork and service modes.'
