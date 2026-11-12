#!/usr/bin/env bash
# Compose complete source recipes into a source family; no binary repackaging.
set -Eeuo pipefail
# Expand upstream source recipes in the selected Fedora worker, without binary retagging.
fedora_number=$(rpm --eval '%{fedora}')
[[ $fedora_number =~ ^[0-9]+$ ]]
fedora_target=${FEDORA_TARGET:-$fedora_number}
if [[ -z ${FEDORA_TARGET:-} && $fedora_number != 44 && $fedora_number != 45 ]]; then fedora_target=rawhide; fi
case $fedora_target in
    44|45) [[ $fedora_number == "$fedora_target" ]] || { echo 'Use a matching Fedora source worker' >&2; exit 1; };;
    rawhide) grep -Eq '^VERSION="?(Rawhide|[0-9]+ \(Rawhide( Prerelease)?\))"?$|^VERSION_ID="?rawhide"?$' /etc/os-release || { echo 'Use a Rawhide source worker' >&2; exit 1; };;
    *) echo 'Fedora target must be 44, 45 or rawhide' >&2; exit 1;;
esac
dist=.fc$fedora_number
rpm_target=(--define "dist $dist" --define "fedora $fedora_number")
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
cd "$root"
family=${1:?source family}; target=${TARGET_ARCH:-aarch64}
standalone=${SOURCE_PACKAGE:-}
integration=false
if [[ $family == uke-control || $family =~ ^uke-(meta|power|sensor|audio|video|desktop-extras)-integration$ ]]; then integration=true; fi
[[ $integration == true || $family =~ ^uke-(core|hw|power|sensors|extras|additional|metas|camera|sound)$ ]]
[[ $integration == false || -z $standalone ]]
[[ $target == aarch64 || $target == x86_64 ]]
# Sound contains metadata and desktop data; other families compile native payloads.
[[ $family == uke-sound || $(rpm --eval '%{_host_cpu}') == "$target" ]] || {
    printf '%s\n' 'Generate a family source on its native architecture; --target alone does not retarget RPM build/host triplets.' >&2
    exit 1
}
catalog=packaging/families/families.json
if [[ $integration == true ]]; then catalog=packaging/families/integrations.json; fi
profile=$(jq -ce --arg name "$family" '.families[]|select(.name==$name)' "$catalog")
if [[ $integration == true && $family == uke-control ]];then
    standalone=uke-control
    [[ $(jq -r '.sources[0].name' <<<"$profile") == "$standalone" ]]
fi
debug_owner=$(jq -r '.debug_owner // ""' <<<"$profile")
if [[ $integration == true ]]; then
    [[ $fedora_target == rawhide && $(jq -er .fedora "$catalog") == "$fedora_number" ]]
fi
version=$(jq -er .version <<<"$profile")
release=$(jq -er '.release // 1' <<<"$profile")
[[ $release =~ ^[1-9][0-9]*$ ]]
output=$(realpath -m "${2:-packaging/families/build/$family}")
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
bundle="$stage/$family-$version"
mkdir -p "$bundle/inputs" "$stage/parts" "$output"/{SOURCES,SPECS,SRPMS}
printf '%s\n' '20 October 2026. Source-family packaging; device acceptance remains separate.' >"$bundle/README"
cp packaging/families/{families.json,source-inputs.json} "$bundle/"
cp LICENSE "$bundle/LICENSE"
if [[ $integration == true && $family == uke-meta-integration ]];then
    cp uke-core-meta/src/profile.json "$bundle/core-profile.json"
    cp uke-desktop-metas/src/profile.json "$bundle/desktops-profile.json"
fi
cp packaging/families/{migrations.json,host/service-rename.sh} "$bundle/"
jq -r '.rename|to_entries[]|[.key,.value]|@tsv' packaging/families/families.json >"$stage/rename"
# Local native sources are produced by the existing owning-component recipes.
if [[ $integration == true ]]; then
    while IFS=$'\t' read -r key url hash; do
        [[ $key =~ ^uke-[a-zA-Z0-9+_.-]+$ ]]
        case $url in
            https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/*.src.rpm|https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-consolidation-archive-20261101/*.src.rpm) ;;
            *) echo 'Unreviewed integration source URL' >&2; exit 1;;
        esac
        [[ $hash =~ ^[a-f0-9]{64}$ ]]
        input="$root/packaging/families/source-cache/integrations/$key.src.rpm"
        mkdir -p "$(dirname "$input")"
        if [[ ! -s $input ]]; then
            curl --fail --location --silent --show-error --proto '=https' --proto-redir '=https' --max-time 240 "$url" -o "$input.part"
            mv "$input.part" "$input"
        fi
        printf '%s  %s\n' "$hash" "$input" | sha256sum -c - >&2
        printf '%s\t%s\n' "$key" "$input" >>"$stage/inputs"
    done < <(jq -r '.sources[]|[.name,.url,.sha256]|@tsv' <<<"$profile")
    cp "$catalog" "$bundle/"
elif [[ -n $standalone ]]; then
    [[ $standalone =~ ^[a-zA-Z0-9+_.-]+$ && -f ${SOURCE_RPM:?independent source RPM} ]]
    printf '%s\t%s\n' "$standalone" "$(realpath "$SOURCE_RPM")" >"$stage/inputs"
else
case $family in
uke-sound)
    make -C uke-desktop-integration speaker-controls-srpm outdir="$stage/speaker-controls" >"$stage/local.log"
    controls=("$stage/speaker-controls/"*.src.rpm); [[ ${#controls[@]} == 1 && -f ${controls[0]} ]]
    printf '%s\t%s\n' uke-speaker-controls "${controls[0]}" >"$stage/inputs"
    make -C uke-desktop-integration displayport-audio-srpm outdir="$stage/audio-policy" >>"$stage/local.log"
    policies=("$stage/audio-policy/"*.src.rpm); [[ ${#policies[@]} == 1 && -f ${policies[0]} ]]
    printf '%s\t%s\n' uke-displayport-audio "${policies[0]}" >>"$stage/inputs";;
uke-hw)
    make -C uke-boot srpm outdir="$stage/boot" >"$stage/local.log"
    boots=("$stage/boot/"*.src.rpm); [[ ${#boots[@]} == 1 && -f ${boots[0]} ]]
    printf '%s\t%s\n' uke-boot-integration "${boots[0]}" >"$stage/inputs";;
uke-core)
    make -C uke-control srpm outdir="$stage/control" >"$stage/local.log"
    controls=("$stage/control/"*.src.rpm); [[ ${#controls[@]} == 1 && -f ${controls[0]} ]]
    printf '%s\t%s\n' uke-control "${controls[0]}" >"$stage/inputs";;
uke-sensors)
    make -C uke-sensors proximity-capture-srpm outdir="$stage/proximity-capture" >"$stage/local.log"
    captures=("$stage/proximity-capture/SRPMS/"*.src.rpm); [[ ${#captures[@]} == 1 && -s ${captures[0]} ]]
    printf '%s\t%s\n' uke-proximity-capture "${captures[0]}" >"$stage/inputs"
    make -C uke-sensors front-color-srpm outdir="$stage/front-color" >>"$stage/local.log"
    colors=("$stage/front-color/SRPMS/"*.src.rpm); [[ ${#colors[@]} == 1 && -s ${colors[0]} ]]
    printf '%s\t%s\n' uke-front-color "${colors[0]}" >>"$stage/inputs"
    make -C uke-sensors srpm PACKAGE=libssc outdir="$stage/libssc" >"$stage/local.log"
    libraries=("$stage/libssc/"*.src.rpm); [[ ${#libraries[@]} == 1 && -f ${libraries[0]} ]]
    printf '%s\t%s\n' libssc "${libraries[0]}" >>"$stage/inputs"
    make -C uke-sensors srpm PACKAGE=uke-ssc-fileservice outdir="$stage/ssc" >>"$stage/local.log"
    services=("$stage/ssc/"*.src.rpm); [[ ${#services[@]} == 1 && -f ${services[0]} ]]
    printf '%s\t%s\n' uke-ssc-fileservice "${services[0]}" >>"$stage/inputs";;
uke-power)
    make -C uke-desktop-integration srpm outdir="$root/uke-desktop-integration/build/srpm" >"$stage/local.log"
    make -C uke-platform-runtime srpm outdir="$stage/battery" >>"$stage/local.log"
    printf '%s\t%s\n' uke-adaptive-display "$root/uke-desktop-integration/build/srpm/uke-adaptive-display-1.0.16-1$(rpm --eval '%{?dist}').src.rpm" >"$stage/inputs"
    batteries=("$stage/battery/rpmbuild/SRPMS/"*.src.rpm)
    [[ ${#batteries[@]} == 1 && -f ${batteries[0]} ]]
    printf '%s\t%s\n' uke-battery-observer "${batteries[0]}" >>"$stage/inputs";;
uke-extras)
    make -C uke-desktop-packaging srpm PACKAGE=plymouth-uke outdir="$root/uke-desktop-packaging/build/srpm/plymouth-uke" >"$stage/local.log"
    printf '%s\t%s\n' uke-plymonth "$root/uke-desktop-packaging/build/srpm/plymouth-uke/uke-plymonth-1.0.2-4$(rpm --eval '%{?dist}').src.rpm" >"$stage/inputs";;
uke-additional)
    make -C uke-video srpm outdir="$stage/video" >"$stage/local.log"
    backends=("$stage/video/SRPMS/"*.src.rpm)
    [[ ${#backends[@]} == 1 && -s ${backends[0]} ]]
    printf '%s\t%s\n' uke-vaapi-v4l2 "${backends[0]}" >"$stage/inputs";;
uke-metas)
    for component in uke-core-meta uke-desktop-metas; do
        make -C "$component" srpm outdir="$stage/$component" >"$stage/local.log"
        files=("$stage/$component/"*.src.rpm); [[ ${#files[@]} == 1 ]]
        printf '%s\t%s\n' "$component" "${files[0]}" >>"$stage/inputs"
    done;;
esac
touch "$stage/inputs"
# Fedora variants are regenerated from the current verified Rawhide source lock.
jq -r --arg family "$family" '.packages[]|select(.family==$family)|.name' uke-desktop-packaging/manifests/native-runtime.json >"$stage/native-packages"
while IFS= read -r package; do
    make -C uke-desktop-packaging srpm PACKAGE="$package" outdir="$stage/native/$package" >"$stage/native-$package.log"
    files=("$stage/native/$package/"*.src.rpm)
    [[ ${#files[@]} == 1 && -s ${files[0]} ]]
    printf '%s\t%s\n' "$package" "${files[0]}" >>"$stage/inputs"
done <"$stage/native-packages"
while IFS=$'\t' read -r key url hash; do
    input="$root/packaging/families/source-cache/inputs/$key.src.rpm"
    mkdir -p "$(dirname "$input")"
    if [[ ! -s $input ]]; then
        case "$key:$url" in
        *:https://download.copr.fedorainfracloud.org/results/mcc45tr/*|*:https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-migration-archive-20261020/*.src.rpm) ;;
        *) echo 'Unreviewed source RPM URL' >&2; exit 1;;
        esac
        curl --fail --location --silent --show-error --proto '=https' --proto-redir '=https' --max-time 240 "$url" -o "$input.part"
        mv "$input.part" "$input"
    fi
    printf '%s  %s\n' "$hash" "$input" | sha256sum -c - >&2
    printf '%s\t%s\n' "$key" "$input" >>"$stage/inputs"
done < <(jq -r --arg family "$family" 'map(select(.family==$family))|sort_by(if .name=="libssc" then 0 else 1 end)[]|[.name,.url,.sha256]|@tsv' packaging/families/source-inputs.json)
fi
# Generated SensorProxy inputs need the same provider-first order as archives.
awk -F '\t' '$1=="libssc"' "$stage/inputs" >"$stage/ordered-inputs"
awk -F '\t' '$1!="libssc"' "$stage/inputs" >>"$stage/ordered-inputs"
mv "$stage/ordered-inputs" "$stage/inputs"
while IFS=$'\t' read -r key input; do
    [[ $key =~ ^[a-zA-Z0-9+_.-]+$ ]]
    source="$bundle/inputs/$key"; part="$stage/parts/$key"
    mkdir -p "$source" "$part"
    rpm2cpio "$input" >"$part/source.cpio"
    cpio -t --quiet <"$part/source.cpio" >"$part/source-files"
    awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$part/source-files"
    (cd "$source"; cpio -idm --quiet --no-absolute-filenames <"$part/source.cpio")
    if [[ $key == uke-front-color && $integration == false ]]; then
        for path in src/front-color.cpp src/front-color-state.hpp src/front-color-file.hpp src/ssc-discovery.cpp src/ssc-sample.cpp tests/front-color.cpp; do
            cmp "$root/uke-sensors/$path" <(tar -xOf "$source/uke-front-color-0.1.0.tar.gz" "uke-front-color-0.1.0/$path")
        done
    fi
    specs=("$source/"*.spec); [[ ${#specs[@]} == 1 ]]
    if [[ $key == uke-firmware && $integration == false ]]; then
        cp "${specs[0]}" "$source/uke-firmware.spec.original"
        # Mandatory paths must resolve without DNF's optional filelists metadata.
        sed -i '/^AutoReqProv: no/aProvides: /usr/lib/firmware/qcom/uke/sensors/sns_tppe.so\nProvides: /usr/share/uke-sensors/odm/json.lst\nProvides: /usr/share/uke-sensors/sns_reg.conf' "${specs[0]}"
        # Retain unsuffixed virtual versions and cover the installed Fedora builds.
        sed -i '/^Provides: uke-owner-.*1\.owner$/ {p;s/1\.owner$/1.owner%{?dist}/;}; /^Obsoletes: uke-owner-/s/1\.owner$/1.owner%{?dist}/' "${specs[0]}"
        # Compare only this child's files in the shared buildroot.
        sed -i 's@^diff -r usr .*@find usr -type f -print0 | while IFS= read -r -d '\'''\'' file; do cmp -- "$file" "%{buildroot}/$file" || exit; done@' "${specs[0]}"
    fi
    if [[ $integration == true && $family == uke-meta-integration && $key == uke-core-meta && $(jq -r '.fedora_returns // false' <<<"$profile") == true ]]; then
        bash packaging/families/host/fedora-return-meta.sh "${specs[0]}" "$part/fedora-return.spec"
        cp "$part/fedora-return.spec" "${specs[0]}"
    fi
    if [[ $integration == true && $family == uke-power-integration && $key == uke-battery-observer ]]; then
        bash packaging/families/host/fedora-cxx-runtime.sh "${specs[0]}" "$part/fedora-cxx.spec"
        cp "$part/fedora-cxx.spec" "${specs[0]}"
    fi
    awk '/^%build([[:space:]]|$)/ {print ": __UKE_CWD__=%{buildsubdir}\n: __UKE_BUILDROOT__=%{buildroot}\n: __UKE_BUILDDIR__=%{_builddir}"} {print}' "${specs[0]}" >"$part/input.spec"
    rpmspec --parse --target "$target" "${rpm_target[@]}" --define "_builddir /__UKE_BUILD__/$key" --define "_sourcedir $source" --define 'debug_package %{nil}' "$part/input.spec" >"$part/expanded.spec"
    native_version=$(jq -r --arg key "$key" '.native_versions[$key] // ""' packaging/families/families.json)
    release_override=$(jq -r --arg key "$key" '.release_overrides[$key] // ""' packaging/families/families.json)
    preserve_identity=0
    if [[ $integration == true ]]; then native_version=;release_override=;preserve_identity=1; fi
    [[ -z $release_override || $release_override =~ ^[1-9][0-9]*[.]fc[0-9]+$ ]]
    [[ -z $release_override ]] || release_override=${release_override%%.fc*}$dist
    part_standalone=$standalone
    if [[ $integration == true && $key != "$standalone" ]];then part_standalone=;fi
    awk -v release_increment="$release" -v preserve_identity="$preserve_identity" -v standalone="$part_standalone" -v dist="$dist" -v out="$part" -v mapping="$stage/rename" -v native_version="$native_version" -v release_override="$release_override" -f packaging/families/host/split-spec.awk "$part/expanded.spec"
    printf '%s\t%s\n' "$key" "$(sha256sum "$input" | cut -d' ' -f1)" >>"$bundle/INPUT-SRPMS.tsv"
done <"$stage/inputs"
group=$family
input_bundle=$bundle
if [[ -n $standalone ]]; then
    IFS=$'\t' read -r family version release <"$stage/parts/$standalone/standalone-identity"
    if [[ $bundle != "$stage/$family-$version" ]]; then mv "$bundle" "$stage/$family-$version"; fi
    bundle="$stage/$family-$version"
    printf '%s\n' '1 November 2026. Reviewed source package; device acceptance remains separate.' >"$bundle/README"
fi
spec="$output/SPECS/$family.spec"
{
    if [[ -n $standalone ]]; then
        printf '%%global debug_package %%{nil}\nURL: https://github.com/MCC45TR/uke-linux-hardware-support\nSource0: %s-%s.tar.xz\nExclusiveArch: %s\n' "$family" "$version" "$target"
    else
    printf '%%global debug_package %%{nil}\nName: %s\nVersion: %s\nRelease: %s%%{?dist}\nSummary: Uke %s package selection and source family\nLicense: MIT\nURL: https://github.com/MCC45TR/uke-linux-hardware-support\nSource0: %s-%s.tar.xz\nExclusiveArch: %s\n' "$family" "$version" "$release" "${family#uke-}" "$family" "$version" "$target"
    fi
    if [[ -z $standalone || $integration == true ]];then
    printf 'SourceLicense: MIT'
    for part in "$stage/parts/"*; do [[ ! -f $part/preamble ]] || awk '/^License:/{sub(/^License:[[:space:]]*/, "");print}' "$part/preamble"; done | LC_ALL=C sort -u | while IFS= read -r license; do printf ' AND (%s)' "$license"; done
    printf '\n'
    fi
    if [[ -n $debug_owner ]]; then printf '%%global __debug_package 1\n%%global _debugsource_packages 1\n'; fi
    if [[ $group == uke-hw && ( -z $standalone || $standalone == uke-firmware ) ]]; then
        # DSP firmware ELF data is not a host executable and has no build-id link.
        printf '%%global _build_id_links none\n%%global _binaries_in_noarch_packages_terminate_build 0\n'
        printf 'Obsoletes: uke-fastfetch-config <= 1.0.1-4.fc46\n'
    fi
    if [[ $family == uke-power ]]; then
        printf 'Obsoletes: uke-power-profile-backend < 1.3.2\nConflicts: uke-power-profile-backend\n'
        printf 'Requires(pre): rpm systemd coreutils sed\nRequires(posttrans): systemd coreutils sed\n'
    fi
    if [[ -z $standalone && $integration == false ]]; then
        jq -r '.requires[]|"Requires: "+.' <<<"$profile"
        jq -r '.suggests[]|"Suggests: "+.' <<<"$profile"
    fi
    for part in "$stage/parts/"*; do [[ ! -f $part/buildrequires ]] || cat "$part/buildrequires"; done | LC_ALL=C sort -u | if [[ $family == uke-sensors ]]; then sed '/^BuildRequires: libssc-devel /d;s/pkgconfig(libssc)//g'; else cat; fi
    if [[ -z $standalone && $integration == false ]]; then
        printf '\n%%description\nLogical installation selection. Child RPMs remain separately installable.\nThis package adds no process and does not enable hardware or prove support.\n'
    elif [[ $integration == true && -z $standalone ]]; then
        printf '\n%%description\nReviewed component sources with independent binary package identities.\nThe source package adds no runtime selection RPM or service.\n'
    fi
    for part in "$stage/parts/"*; do
        [[ -f $part/preamble ]] || continue
        if [[ $standalone == uke-core-meta ]]; then
            jq -r --arg dist "$dist" '. as $catalog | .families[] | (.version+"-"+(($catalog.retired_selection_releases[.name] // .release // 1)|tostring)+$dist) as $evr | "Provides: "+.name+" = "+$evr+"\nObsoletes: "+.name+" <= "+$evr' packaging/families/families.json >"$stage/retired-selections"
        else
            : >"$stage/retired-selections"
        fi
        awk '
          FILENAME==ARGV[1] {compat=compat $0 "\n";next}
          /^%description -n uke-core-meta$/ {printf "%s",compat}
          /^%description -n uke-(iio-sensor-proxy|upower|accountsservice)$/ {
            print "Requires(pre): rpm systemd coreutils sed"
            print "Requires(posttrans): systemd coreutils sed"
            if($3=="uke-iio-sensor-proxy")print "Requires(posttrans): /usr/bin/semodule /usr/bin/selinuxenabled selinux-policy-targeted"
          }
          {print}
        ' "$stage/retired-selections" "$part/preamble"
    done
    if [[ -n $debug_owner ]]; then
        for suffix in debuginfo debugsource; do
            printf '\n%%package -n %s-%s\n' "$debug_owner" "$suffix"
            awk '/^(Version|Release|Epoch|License):/{print}' "$stage/parts/$debug_owner/preamble"
            printf 'Summary: Optional debug %s for Uke sensor integration\nAutoReq: 0\nAutoProv: 1\n%%description -n %s-%s\nDebug data from the matched sensor source build; no hardware service.\n' "$suffix" "$debug_owner" "$suffix"
        done
    fi
    printf '\n%%prep\n%%setup -q\n'
    for phase in prep generate_buildrequires build install check; do
        if [[ $phase == generate_buildrequires ]] && ! find "$stage/parts" -name generate_buildrequires -print -quit | grep -q .; then continue; fi
        [[ $phase == prep ]] || printf '\n%%%s\n' "$phase"
        if [[ $group == uke-hw && ( -z $standalone || $standalone == uke-firmware ) && $phase == check ]]; then
            printf 'mv "%%{_builddir}/%s-%s/firmware-unprocessed" "%%{buildroot}/usr/lib/firmware"\n' "$family" "$version"
        fi
        if [[ $family == uke-sensors ]]; then
            printf 'export PKG_CONFIG_PATH="%%{_builddir}/%s-%s/work/libssc/libssc/build/meson-uninstalled${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"\nexport LD_LIBRARY_PATH="%%{_builddir}/%s-%s/work/libssc/libssc/build/src${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n' "$family" "$version" "$family" "$version"
        fi
        # Manifest order is build order, including libssc before SensorProxy.
        while IFS=$'\t' read -r key input; do
            part="$stage/parts/$key"; [[ -s $part/$phase ]] || continue
            IFS=$'\t' read -r _ _ _ cwd builddir buildroot <"$part/identity"
            base="%{_builddir}/$family-$version"
            printf '(\n'
            if [[ $phase == prep ]]; then
                printf 'mkdir -p "%s/work/%s"\n' "$base" "$key"
            else
                printf 'cd "%s/work/%s/%s"\n' "$base" "$key" "$cwd"
            fi
            # Fully expanded recipes retain literal percents; only paths are late-bound.
            awk -v base="$base" -v key="$key" -v source="$input_bundle/inputs/$key" -v builddir="$builddir" -v buildroot="$buildroot" '
              function replace(s,old,new, p,r){r="";while((p=index(s,old))){r=r substr(s,1,p-1) new;s=substr(s,p+length(old))}return r s}
              {gsub(/%/,"%%");s=replace($0,buildroot,"%{buildroot}");s=replace(s,builddir,base "/work/" key);s=replace(s,source,base "/inputs/" key);print s}
            ' "$part/$phase"
            printf ')\n'
        done <"$stage/inputs"
        if [[ $integration == true && $family == uke-meta-integration && ( $phase == install || $phase == check ) ]];then
            # Retained child archives are nested source families; apply policy at their installed boundary.
            for selection in core desktops;do
                policy="%{_builddir}/$family-$version/$selection-profile.json"
                installed="%{buildroot}%{_datadir}/senemos/uke/$selection/profile.json"
                if [[ $phase == install ]];then printf 'install -Dm644 "%s" "%s"\n' "$policy" "$installed"
                else printf 'cmp "%s" "%s"\n' "$policy" "$installed";fi
            done
        fi
        if [[ $group == uke-hw && ( -z $standalone || $standalone == uke-firmware ) && $phase == install ]]; then
            # RPM's native post-install processing runs before %check.
            printf 'mv "%%{buildroot}/usr/lib/firmware" "%%{_builddir}/%s-%s/firmware-unprocessed"\n' "$family" "$version"
        fi
    done
    if [[ -z $standalone && $integration == false ]]; then printf '\n%%files\n%%license LICENSE\n%%doc README\n'; fi
    while IFS=$'\t' read -r key input; do
        part="$stage/parts/$key"; [[ -f $part/tail ]] || continue
        IFS=$'\t' read -r _ _ _ cwd builddir buildroot <"$part/identity"
        awk -v prefix="work/$key/$cwd/" -v base="%{_builddir}/$family-$version" -v key="$key" -v source="$input_bundle/inputs/$key" '
          function replace(s,old,new, p,r){r="";while((p=index(s,old))){r=r substr(s,1,p-1) new;s=substr(s,p+length(old))}return r s}
          /^%files/ {files=1;for(i=2;i<=NF;i++)if($i=="-f")$(i+1)=prefix $(i+1); print;next}
          /^%/ && !/^%(license|doc|dir|ghost|config|attr|defattr|exclude|verify|caps)/ {files=0;print;next}
          files && /^%(license|doc) / {for(i=2;i<=NF;i++)if($i!~/^\//)$i=prefix $i;print;next}
          files && /^%config / {sub(/^%config /,"%config(noreplace) ");print;next}
          {if(!files)gsub(/%/,"%%");print replace($0,source,base "/inputs/" key)}
        ' "$part/tail"
    done <"$stage/inputs"
    if [[ -n $debug_owner ]]; then
        printf '\n%%files -n %s-debuginfo -f debugfiles.list\n%%files -n %s-debugsource -f debugsourcefiles.list\n' "$debug_owner" "$debug_owner"
    fi
    while IFS=$'\t' read -r old new unit; do
        printf '\n%%pre -n %s\n' "$new"
        printf 'old=%s; new=%s; unit=%s; phase=save\n' "$old" "$new" "$unit"
        sed 's/%/%%/g' packaging/families/host/service-rename.sh
        printf '\n%%posttrans -n %s\n' "$new"
        printf 'old=%s; new=%s; unit=%s; phase=restore\n' "$old" "$new" "$unit"
        sed 's/%/%%/g' packaging/families/host/service-rename.sh
    done < <(jq -r --arg family "$group" '.[]|select(.family==$family)|[.old,.new,.unit]|@tsv' packaging/families/migrations.json | while IFS=$'\t' read -r old new unit; do
        if [[ -z $standalone ]] || grep -Fxq "Name: $new" "$stage/parts/$standalone/preamble" || grep -Fxq "%package -n $new" "$stage/parts/$standalone/preamble"; then printf '%s\t%s\t%s\n' "$old" "$new" "$unit"; fi
    done)
    if [[ -n $standalone ]]; then
        printf '\n%%changelog\n* Thu Oct 29 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - %s-%s\n- Build this component independently from its reviewed source recipe.\n' "$version" "$release"
    else
        printf '\n%%changelog\n* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - %s-%s\n- Build reviewed child recipes under one source family without binary repackaging.\n' "$version" "$release"
    fi
} >"$spec"
bash packaging/families/host/check-cohort.sh "$spec" "$target"
# The archive contains complete upstream sources, patches, licenses and original specs.
(cd "$bundle"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | LC_ALL=C sort -z | xargs -0 sha256sum >SOURCE-SHA256SUMS)
tar --sort=name --mtime='2026-10-20 00:00:00Z' --owner=0 --group=0 --numeric-owner -cJf "$output/SOURCES/$family-$version.tar.xz" -C "$stage" "$family-$version"
rpmspec --parse --target "$target" "${rpm_target[@]}" "$spec" >"$output/parsed.spec"
rpmbuild -bs --nodeps --target "$target" "${rpm_target[@]}" --define "_topdir $output" "$spec"
