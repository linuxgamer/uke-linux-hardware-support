%global debug_package %{nil}
Name: plymouth-uke
Version: 1.0.0
Release: 1%{?dist}
Summary: Optional Senemos Uke Plymouth theme
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-desktop-packaging
Source0: %{name}-%{version}.tar.xz
BuildArch: noarch
BuildRequires: tar xz
Requires: plymouth plymouth-plugin-script
%description
A renderer-size-aware text theme for Uke development images. It installs
optional theme data only; it does not set the default theme, regenerate an
initramfs, select a boot entry or infer working Uke graphics.
%prep
%setup -q
%build
%install
install -d %{buildroot}%{_datadir}/plymouth/themes/senemos-uke
install -m644 src/plymouth-uke/* %{buildroot}%{_datadir}/plymouth/themes/senemos-uke/
%check
test -s %{buildroot}%{_datadir}/plymouth/themes/senemos-uke/plymouth-uke.script
! grep -Eq '(DeviceScale|/dev/|nabu)' %{buildroot}%{_datadir}/plymouth/themes/senemos-uke/*
%files
%license LICENSE
%doc README.md
%{_datadir}/plymouth/themes/senemos-uke/

%changelog
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Build an independently scoped Uke development package with explicit readiness.
