#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# COPR source worker only; never run against the tablet RPM database.
set -Eeuo pipefail
[[ ${UKE_COPR_FEDORA_RETURN_FIXTURE:-} == 1 ]]
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
test ! -e /etc/senemos/kernel-update.conf
project=$(cd -- "$(dirname -- "$0")/.."; pwd)
root=$(cd -- "$project/.."; pwd)
cache=$root/uke-desktop-packaging/source-cache/fedora-returns-20261101
bash "$root/packaging/families/tests/fedora-return-abi.sh"
fixture=$(mktemp -d)
trap 'rm -rf -- "$fixture"' EXIT
mkdir -p "$fixture"/{forks,originals,candidate,candidate-desktop,legacy,unknown,top/SPECS,top/BUILD,top/BUILDROOT,top/RPMS,top/SRPMS,top/SOURCES}
touch "$fixture/empty.conf"
mkdir "$fixture/key-home";chmod 0700 "$fixture/key-home"
export GNUPGHOME=$fixture/key-home
gpg --batch --passphrase '' --quick-generate-key 'Uke return fixture <fixture@example.invalid>' rsa2048 sign 1d > "$fixture/key.log" 2>&1
fingerprint=$(gpg --batch --with-colons --list-keys | awk -F: '$1=="fpr"{print $10;exit}')
gpg --batch --armor --export "$fingerprint" > "$fixture/fixture.asc"
while IFS=$'\t' read -r old original; do
    cp "$cache/${old##*/}" "$fixture/forks/"
    cp "$cache/${old##*/}" "$fixture/candidate/"
    cp "$cache/${original##*/}" "$fixture/originals/"
done < <(jq -r '.pairs[]|[.fork.url,.original.url]|@tsv' "$root/packaging/families/consolidation/fedora-return-inputs.json")
build_fixture() {
    local name=$1 version=$2 repo=$3 requires=${4-} provides=${5-}
    cat > "$fixture/top/SPECS/fixture.spec" <<EOF
Name: $name
Version: $version
Release: 1.fc46
Summary: Disposable source-worker dependency fixture
License: MIT
AutoReqProv: no
$requires
$provides
%description
Metadata fixture only. No programs or scriptlets.
%install
mkdir -p %{buildroot}/usr/share/uke-return-fixture/$name
echo fixture > %{buildroot}/usr/share/uke-return-fixture/$name/value
%files
/usr/share/uke-return-fixture/$name/value
EOF
    rpmbuild -bb --define "_topdir $fixture/top" "$fixture/top/SPECS/fixture.spec" > "$fixture/build.log" 2>&1 || { cat "$fixture/build.log"; return 1; }
    rpmsign --define "_openpgp_sign_id $fingerprint" --define '_openpgp_sign gpg' --addsign "$fixture/top/RPMS/aarch64/$name-$version-1.fc46.aarch64.rpm" > "$fixture/sign.log" 2>&1 || { cat "$fixture/sign.log"; return 1; }
    cp "$fixture/top/RPMS/aarch64/$name-$version-1.fc46.aarch64.rpm" "$fixture/$repo/"
}
build_fixture uke-core-meta 1 candidate 'Requires: libstdc++ >= 16.2.1' $'Provides: uke-core-meta(fedora-returns-v1) = 1\nProvides: uke-core-meta(fedora-cxx-runtime-v1) = 1'
build_fixture uke-return-caller 1 legacy 'Requires: senemos-native-runtime(libstdc++)'
cp "$fixture/legacy/"*.rpm "$fixture/candidate/"
build_fixture uke-libstdcxx 99 unknown
build_fixture libstdc++ 99 candidate
cp "$fixture/forks/"*.rpm "$fixture/candidate-desktop/"
build_fixture uke-core-meta 1 candidate-desktop 'Requires: senemos-native-runtime(libstdc++)' 'Provides: uke-core-meta(fedora-returns-v1) = 1'
for repo in forks originals candidate candidate-desktop; do createrepo_c --quiet "$fixture/$repo"; done
c++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror $(pkg-config --cflags libdnf5 rpm libsystemd) \
    "$project/tests/fedora-returns.cpp" "$project/src/cli.cpp" -I"$project/src" -o "$fixture/solver" $(pkg-config --libs libdnf5 rpm libsystemd)
curl --fail --silent --show-error --max-time 60 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/pubkey.gpg -o "$fixture/copr.asc"
for mode in fresh forward mixed weak-off update reverse missing unreviewed legacy-caller desktop-only; do
    target=$fixture/root-$mode
    mkdir -p "$target/usr/lib/sysimage/rpm"
    rpm --dbpath "$target/usr/lib/sysimage/rpm" --initdb
    rpmkeys --dbpath "$target/usr/lib/sysimage/rpm" --import "$root/packaging/families/keys/fedora-46.asc" "$fixture/copr.asc" "$fixture/fixture.asc"
    seed=()
    for file in "$fixture/forks/"*.rpm; do
        if [[ $mode == reverse || $mode == mixed && ${#seed[@]} -ge 6 ]]; then
            name=$(rpm -qp --qf '%{NAME}' "$file")
            original=$(jq -er --arg name "$name" '.pairs[]|select(.fork_name==$name)|.original.url|split("/")|last' "$root/packaging/families/consolidation/fedora-return-inputs.json")
            seed+=("$fixture/originals/$original")
        else seed+=("$file"); fi
    done
    if [[ $mode == fresh ]]; then seed=(); fi
    if [[ $mode == unreviewed ]]; then seed=("$fixture/unknown/"*.rpm); fi
    if [[ $mode == legacy-caller ]]; then seed+=("$fixture/legacy/"*.rpm); fi
    if ((${#seed[@]})); then rpm --define '__systemd_sysusers /usr/bin/true' --dbpath "$target/usr/lib/sysimage/rpm" -i --justdb --nodeps --noscripts --notriggers "${seed[@]}"; fi
    "$fixture/solver" "$fixture" "$target" "$mode"
done
