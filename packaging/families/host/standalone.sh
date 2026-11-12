#!/usr/bin/env bash
# COPR source worker: retain reviewed component recipes without an umbrella RPM.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
cd "$root"
key=${1:?source package}; output=$(realpath -m "${2:?output directory}")
[[ $key =~ ^[a-zA-Z0-9+_.-]+$ ]]
grep -Eq '^VERSION="?(Rawhide|[0-9]+ \(Rawhide( Prerelease)?\))"?$|^VERSION_ID="?rawhide"?$' /etc/os-release || { echo 'Use a Rawhide source worker' >&2; exit 1; }
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
args=(); target=srpm
case $key in
    uke-control) group=uke-core; component=uke-control;;
    uke-boot-integration) group=uke-hw; component=uke-boot;;
    uke-adaptive-display) group=uke-power; component=uke-desktop-integration;;
    uke-battery-observer) group=uke-power; component=uke-platform-runtime;;
    uke-proximity-capture) group=uke-sensors; component=uke-sensors; target=proximity-capture-srpm;;
    uke-front-color) group=uke-sensors; component=uke-sensors; target=front-color-srpm;;
    libssc|uke-ssc-fileservice) group=uke-sensors; component=uke-sensors; args=("PACKAGE=$key");;
    uke-plymonth) group=uke-extras; component=uke-desktop-packaging; args=(PACKAGE=plymouth-uke);;
    uke-vaapi-v4l2) group=uke-additional; component=uke-video;;
    uke-core-meta|uke-desktop-metas) group=uke-metas; component=$key;;
    uke-speaker-controls) group=uke-sound; component=uke-desktop-integration; target=speaker-controls-srpm;;
    uke-displayport-audio) group=uke-sound; component=uke-desktop-integration; target=displayport-audio-srpm;;
    *)
        group=$(jq -er --arg key "$key" '.packages[]|select(.name==$key)|.family' uke-desktop-packaging/manifests/native-runtime.json) || group=
        if [[ -n $group ]]; then
            component=uke-desktop-packaging; args=("PACKAGE=$key")
        else
            entry=$(jq -ce --arg key "$key" '.[]|select(.name==$key)' packaging/families/source-inputs.json)
            group=$(jq -er .family <<<"$entry")
            input="$root/packaging/families/source-cache/inputs/$key.src.rpm"
            mkdir -p "$(dirname "$input")"
            url=$(jq -er .url <<<"$entry")
            case $url in
                https://download.copr.fedorainfracloud.org/results/mcc45tr/*|https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-migration-archive-20261020/*.src.rpm) ;;
                *) echo 'Unreviewed source URL' >&2; exit 1;;
            esac
            curl --fail --location --silent --show-error --proto '=https' --proto-redir '=https' --max-time 240 "$url" -o "$input"
            printf '%s  %s\n' "$(jq -er .sha256 <<<"$entry")" "$input" | sha256sum -c - >&2
            component=
        fi;;
esac
if [[ -n $component ]]; then
    make -C "$component" "$target" "${args[@]}" outdir="$stage/input"
    # Fedora provenance SRPMs in SOURCES are inputs, not generated results.
    mapfile -t inputs < <(find "$stage/input" -type f -path '*/SRPMS/*.src.rpm' | LC_ALL=C sort)
    [[ ${#inputs[@]} == 1 ]] || { echo 'Expected one generated component SRPM' >&2; exit 1; }
    input=${inputs[0]}
fi
SOURCE_PACKAGE="$key" SOURCE_RPM="$input" FEDORA_TARGET=rawhide bash packaging/families/host/compose.sh "$group" "$stage/output"
mapfile -t results < <(find "$stage/output/SRPMS" -maxdepth 1 -type f -name '*.src.rpm')
[[ ${#results[@]} == 1 ]]
name=$(rpm -qp --qf '%{NAME}' "${results[0]}")
[[ $name != "$group" ]]
if [[ $key == uke-boot-integration ]]; then
    expected=$(awk '$1=="Version:" {print $2; exit}' uke-boot/packaging/uke-boot-integration.spec)
    [[ $(rpm -qp --qf '%{VERSION}' "${results[0]}") == "$expected" ]] || {
        echo 'Boot source version differs from the owning component; update native_versions' >&2; exit 1;
    }
fi
mkdir -p "$output"
install -m644 "${results[0]}" "$output/"
