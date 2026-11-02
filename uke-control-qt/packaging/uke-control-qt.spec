Name:           uke-control-qt
Version:        1.0.0
Release:        1%{?dist}
Summary:        Native Kirigami interface for Uke Control
License:        MIT
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  make
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  extra-cmake-modules >= 6.6
BuildRequires:  cmake(KF6I18n) >= 6.8
BuildRequires:  kf6-kirigami
BuildRequires:  kf6-kirigami-addons >= 1.1
BuildRequires:  cmake(KF6KirigamiAddons) >= 1.1
BuildRequires:  kf6-qqc2-desktop-style
BuildRequires:  cmake(KF6WidgetsAddons) >= 6.6
BuildRequires:  cmake(KF6IconThemes) >= 6.6
BuildRequires:  cmake(KF6ConfigWidgets) >= 6.6
BuildRequires:  pkgconfig(uke-control-client) >= 2.8.0
BuildRequires:  %{_includedir}/uke-control/frontend.hpp
Requires:       uke-control >= 2.8.0
Requires:       %{_libexecdir}/uke-control/frontend-request
Requires:       plasma-breeze-qt6
Requires:       plasma-integration
Requires:       breeze-icon-theme
Requires:       qt6-qtdeclarative
Requires:       kf6-kirigami
Requires:       kf6-kirigami-addons >= 1.1
Requires:       kf6-qqc2-desktop-style
Requires:       konsole

%description
A native Qt 6 Kirigami frontend with a typed C++ adapter for Uke Control.
Daily task pages provide typed requests, asynchronous observations and explicit
review before handing guarded operations to the distribution terminal.

%prep
%autosetup

%build
%cmake -DBUILD_TESTING=OFF
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_bindir}/uke-control-qt
%{_datadir}/applications/org.uke.ControlQt.desktop
%{_datadir}/icons/hicolor/scalable/apps/org.uke.ControlQt.svg

%changelog
* Sun Nov 01 2026 Uke contributors - 1.0.0-1
- Add the native Qt Widgets frontend with Breeze style and reviewed requests.
