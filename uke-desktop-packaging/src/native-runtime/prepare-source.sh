# Host-only transformation of exact-pinned official Fedora source specifications.
# This file is not executable and is never installed in a target RPM.
set -Eeuo pipefail
name=${1:?package}
top=${2:?RPM topdir}
project=${3:?repository root}
profile=$(jq -ce --arg name "$name" '.packages[] | select(.name==$name)' "$project/manifests/native-runtime.json")
version=$(jq -er .version <<< "$profile")
release=$(jq -er .release <<< "$profile")
native_release=$(jq -er .native_release <<< "$profile")
source_name=$(jq -er '.source_name // .name' <<< "$profile")
archive="$project/referances/fedora-srpms/$source_name-$version-$release.src.rpm"
mkdir -p "$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS} "$(dirname "$archive")"
if [[ ! -s $archive ]]; then
    curl -fL --retry 3 "https://kojipkgs.fedoraproject.org/packages/$source_name/$version/$release/src/$source_name-$version-$release.src.rpm" -o "$archive.part"
    mv "$archive.part" "$archive"
fi
[[ $(sha256sum "$archive" | cut -d ' ' -f1) == "$(jq -er .sha256 <<< "$profile")" ]]
(cd "$top/SOURCES"; rpm2cpio "$archive" | cpio -idmu --quiet)
spec="$top/SOURCES/$name.spec"
[[ -s $spec ]]
if [[ $name == mesa ]]; then
    cp "$project/packaging/mesa/"*.patch "$project/packaging/mesa/mesa-host-features.cpp" "$top/SOURCES/"
    awk -v release="$native_release" '
        BEGIN { print "%global debug_package %{nil}" }
        /^Release:/ { print "Release: " release "%{?dist}"; next }
        /^License:/ { print; print "ExclusiveArch: aarch64"; next }
        /^Source0:/ { print; print "Patch9998: 0002-freedreno-adreno732-identity.patch"; print "Patch9999: 0001-gallivm-use-aarch64-host-features.patch"; print "Source9999: mesa-host-features.cpp"; next }
        /^BuildRequires:.*meson/ { print "BuildRequires: meson = 1.12.1-2.fc46"; next }
        /^BuildRequires:.*python3-devel/ { print "BuildRequires: python3-devel = 3.15.0-1.fc46"; next }
        /^BuildRequires:.*python3-mako/ { print "BuildRequires: python3-mako = 1.4.1-2.fc46"; next }
        /^BuildRequires:.*python3-pycparser/ { print "BuildRequires: python3-pycparser = 3.00-4.fc45"; next }
        /^BuildRequires:.*python3-pyyaml/ { print "BuildRequires: python3-pyyaml = 6.0.3-6.fc45"; next }
        /^BuildRequires:.*llvm-devel/ { print "BuildRequires: llvm-devel = 23.1.2-1.fc46"; next }
        /^%files filesystem/ {
            print "%check"
            print "awk '\''/^lp_build_fill_mattrs\\(/{copy=1; print \"void\"} copy {print} copy && /^}/ {exit}'\'' src/gallium/auxiliary/gallivm/lp_bld_misc.cpp > mesa-host-features.inc"
            print "%{__cxx} $(llvm-config --cxxflags) -I. %{SOURCE9999} -o mesa-host-features $(llvm-config --ldflags --libs --system-libs)"
            print "./mesa-host-features"
            print ""
        }
        /^%changelog/ {
            print; print "* Tue Oct 13 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - %{version}-" release
            print "- Identify exact Adreno 732 chips without changing their existing profile."
            print "- Honor available AArch64 CPU features in Gallivm JIT code generation."
            exit
        }
        { print }
    ' "$spec" > "$top/SPECS/mesa.spec"
    mv "$spec" "$top/SPECS/mesa.fedora-original.spec"
    rpmbuild -bs --nodeps --target aarch64 --define "_topdir $top" "$top/SPECS/mesa.spec"
    exit 0
fi
printf '%s\n' 'Only the Mesa device-fix source variant is supported.' >&2
exit 2
