Name: uke-control-gui-check
Version: 1.0.0
Release: 4%{?dist}
Summary: Remote checks of published Uke desktop GUI dependency transactions
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
Source1: copr-public-key.asc
Source2: LICENSE
BuildArch: noarch
BuildRequires: gcc-c++ pkgconf-pkg-config libdnf5-devel
BuildRequires: rpm-sign gnupg2 curl jq coreutils findutils bash gzip

%description
Text receipt from remote signature, package-identity and real dependency-solver
checks. The solver resolves fresh desktop selections and an installed KDE meta
upgrade without executing RPM transactions or accessing hardware.

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
* Tue Nov 03 2026 Uke contributors - 1.0.0-4
- Derive the source RPM output filename from the spec identity.

* Tue Nov 03 2026 Uke contributors - 1.0.0-3
- Load repository filelists for the actual boot and core path dependencies.
- Require a real systemd-growfs provider before checking desktop transactions.

* Tue Nov 03 2026 Uke contributors - 1.0.0-2
- Import the pinned signing key into each isolated installed-package database.
- Preserve signature enforcement while resolving the published GUI cohort.
