Name: uke-nfc-tag-check
Version: 1.0.0
Release: 1%{?dist}
Summary: Remote checks of the native Uke NFC package upgrade
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
Source1: copr-public-key.asc
Source2: LICENSE
BuildArch: noarch
BuildRequires: gcc-c++ pkgconf-pkg-config libdnf5-devel
BuildRequires: rpm-sign gnupg2 curl coreutils findutils bash gzip

%description
Text receipt from remote signature, package-identity and native dependency
solver checks. Resolves the old noarch NFC package's ordinary global upgrade
and a fresh installation against actual public repository metadata, requiring
the matching native client and kernel capability. No payload transaction or
hardware operation is performed.

%prep
%autosetup
cp %{SOURCE1} %{SOURCE2} .

%build
g++ %{optflags} -std=c++20 -Wall -Wextra -Werror $(pkg-config --cflags libdnf5) solver.cpp -o solver $(pkg-config --libs libdnf5)

%install
install -d %{buildroot}%{_docdir}/%{name}

%check
bash verify.sh "$PWD/solver" | tee %{buildroot}%{_docdir}/%{name}/validation.txt
test "${PIPESTATUS[0]}" = 0

%files
%license LICENSE
%{_docdir}/%{name}/validation.txt

%changelog
* Wed Nov 04 2026 Uke contributors - 1.0.0-1
- Verify signed client packages and the noarch-to-AArch64 DNF upgrade.
- Require an actual NFC/IR v2 kernel provider without executing a transaction.
