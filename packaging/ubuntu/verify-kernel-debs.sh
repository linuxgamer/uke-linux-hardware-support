#!/bin/bash
# Read back the complete DEB payload/control against the validated staging tree.
set -euo pipefail
[[ $# == 3 ]]
stage=$(realpath "$1"); debs=$(realpath "$2"); version=$3
[[ $version == 7.3.1-1006ubuntu2~tenacious1 ]]
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
for role in core dtbs modules boot; do
 pkg=uke-kernel-$role
 deb=$debs/${pkg}_${version}_arm64.deb
 [[ -f $deb && ! -L $deb && -d $stage/$pkg ]]
 [[ $(dpkg-deb -f "$deb" Package) == "$pkg" && $(dpkg-deb -f "$deb" Version) == "$version" && $(dpkg-deb -f "$deb" Architecture) == arm64 ]]
 for field in Provides Pre-Depends Recommends Suggests Enhances Conflicts Replaces; do [[ -z $(dpkg-deb -f "$deb" "$field") ]]; done
 expected=''
 case $role in
  core) expected="uke-kernel-dtbs (= $version), uke-kernel-modules (= $version)";;
  boot) expected="uke-kernel-core (= $version), uke-kernel-dtbs (= $version), uke-kernel-modules (= $version)";;
 esac
 [[ $(dpkg-deb -f "$deb" Depends) == "$expected" ]]
 dpkg-deb -e "$deb" "$work/control"
 [[ -z $(find "$work/control" -mindepth 1 ! \( -type f \( -name control -o -name md5sums \) \) -print -quit) ]]
 rm -rf "$work/control"
 dpkg-deb -x "$deb" "$work/$pkg"
 diff -r --no-dereference "$stage/$pkg" "$work/$pkg"
 (cd "$stage/$pkg"; find . -printf '%y %m %P %l\n' | sort) > "$work/expected"
 (cd "$work/$pkg"; find . -printf '%y %m %P %l\n' | sort) > "$work/actual"
 cmp "$work/expected" "$work/actual"
 [[ ! -e $work/$pkg/boot && ! -e $work/$pkg/etc ]]
 (cd "$work/$pkg"; sha256sum --check --strict --quiet "usr/share/doc/$pkg/payload.sha256")
done
echo 'Four complete DEBs preserve payload bytes/modes, exact version dependencies and no activation hooks/providers.'
