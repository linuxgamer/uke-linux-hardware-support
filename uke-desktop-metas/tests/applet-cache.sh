#!/usr/bin/env bash
# Host/package check only: reproduce a stale QML cache without creating a widget.
set -Eeuo pipefail
cd "$(dirname "$0")/.."
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
mkdir -m700 "$stage/runtime" "$stage/cache"
export XDG_RUNTIME_DIR="$stage/runtime" XDG_CACHE_HOME="$stage/cache"
export QT_QPA_PLATFORM=offscreen QT_QPA_PLATFORMTHEME=generic QT_QUICK_BACKEND=software
cat > "$stage/label.qml" <<'QML'
import QtQuick
import org.kde.plasma.components as PC
PC.Label { implicitWidth: 360 }
QML
touch -d @1791158400 "$stage/label.qml"
if build/applet-load "$stage/label.qml"; then
    echo 'The known invalid label unexpectedly loaded' >&2
    exit 1
fi
sed -i 's/implicitWidth/width/' "$stage/label.qml"
# Use the actual source archive timestamp, including in the native RPM check.
touch -d "@$(stat -c %Y src/plasma-applet/contents/ui/main.qml)" "$stage/label.qml"
build/applet-load "$stage/label.qml"
printf '%s\n' 'Changed QML archive timestamp invalidates the old cache: PASS'
