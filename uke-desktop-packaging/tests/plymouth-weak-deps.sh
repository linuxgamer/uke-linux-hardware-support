#!/usr/bin/env bash
# Isolated metadata-only packages exercise DNF's real recommendation policy.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../.."; pwd)
stage=$(mktemp -d)
trap 'status=$?; if ((status)); then tail -n 20 "$stage"/*.log >&2; fi; rm -rf -- "$stage"' EXIT
mkdir -p "$stage"/{RPMS,BUILD,BUILDROOT,SPECS,SOURCES,SRPMS,repo,empty}
recommend=$(sed -n '/^Recommends: uke-plymonth /p' "$root/uke-core-meta/packaging/uke-core-meta.spec")
[[ $recommend == 'Recommends: uke-plymonth >= 1.0.2-3' ]]
build() {
    local name=$1 version=$2 release=$3 dependency=$4
    cat >"$stage/SPECS/fixture.spec" <<EOF
Name: $name
Version: $version
Release: $release
Summary: Isolated DNF policy fixture
License: MIT
BuildArch: noarch
$dependency
%description
Metadata-only host fixture. No target payload or scriptlet.
%files
EOF
    rpmbuild -bb --define "_topdir $stage" "$stage/SPECS/fixture.spec" >"$stage/build.log" 2>&1
}
build uke-plymonth 1.0.2 3 ''
build uke-fixture-core 1 1 ''
build uke-fixture-core 2 1 "$recommend"
cp "$stage"/RPMS/noarch/*.rpm "$stage/repo/"
createrepo_c --quiet "$stage/repo"
dnf=(dnf5 -y --installroot="$stage/root" --releasever=46 --setopt="reposdir=$stage/empty" --repofrompath="fixture,file://$stage/repo" --nogpgcheck)
"${dnf[@]}" install uke-fixture-core-1-1 >"$stage/install.log" 2>&1
"${dnf[@]}" upgrade >"$stage/upgrade.log" 2>&1
rpm --root "$stage/root" -q uke-plymonth
"${dnf[@]}" remove uke-plymonth >"$stage/remove.log" 2>&1
rpm --root "$stage/root" -q uke-fixture-core
build uke-fixture-core 3 1 "$recommend"
cp "$stage"/RPMS/noarch/uke-fixture-core-3-1.noarch.rpm "$stage/repo/"
createrepo_c --quiet --update "$stage/repo"
"${dnf[@]}" clean all >"$stage/clean.log" 2>&1
"${dnf[@]}" --refresh upgrade >"$stage/reupgrade.log" 2>&1
[[ $(rpm --root "$stage/root" -q --qf '%{VERSION}' uke-fixture-core) == 3 ]]
if rpm --root "$stage/root" -q uke-plymonth; then exit 1; fi
"${dnf[@]}" --installroot="$stage/no-weak" --setopt=install_weak_deps=False install uke-fixture-core >"$stage/no-weak.log" 2>&1
if rpm --root "$stage/no-weak" -q uke-plymonth; then exit 1; fi
printf '%s\n' 'PASS: DNF upgrade installs the new recommendation; removal keeps Core; later upgrades respect removal and disabled weak dependencies.'
