%global debug_package %{nil}
Name: uke-core-meta
Version: 1.0.0
Release: 4%{?dist}
Summary: Console package selection for the Senemos Uke build candidate
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-core-meta
Source0: %{name}-%{version}.tar.xz
ExclusiveArch: aarch64
BuildRequires: tar xz
Requires: senemos-uke-linux-kernel-mainline >= 7.2.9-1.3
Requires: bash coreutils util-linux-core kmod
Requires: NetworkManager openssh-server sudo
%description
A minimal console package selection and explicit Uke readiness record.
No unqualified firmware, Nabu hardware services or boot selection is installed.
Package installation does not establish a working Uke console or tablet boot.
%prep
%setup -q
%build
%install
install -Dm644 src/profile.json %{buildroot}%{_datadir}/senemos/uke/core/profile.json
%files
%license LICENSE
%doc README.md
%{_datadir}/senemos/uke/core/profile.json

%changelog
* Thu Oct 15 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-4
- Allow Fedora Python dependencies and use the distribution C++ runtime.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-3
- Require the native GNU C++ runtime without inherited Python debugger helpers.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-2
- Enforce the tablet's no-Python runtime policy during dependency solving.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Build an independently scoped Uke development package with explicit readiness.
