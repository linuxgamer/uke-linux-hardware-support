%global source_name uke-control-qt
Name:           uke-control-gui-qt
Version:        1.0.0
Release:        3%{?dist}
Summary:        Native Kirigami interface for Uke Control
License:        MIT
Source0:        %{source_name}-%{version}.tar.gz
Provides:       uke-control-qt = %{version}-%{release}
Obsoletes:      uke-control-qt < 1.0.0-3
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
BuildRequires:  desktop-file-utils
BuildRequires:  plasma-breeze-qt6 plasma-integration breeze-icon-theme
BuildRequires:  pkgconfig(uke-control-client) >= 2.8.0
BuildRequires:  %{_includedir}/uke-control/frontend.hpp
BuildRequires:  uke-control(frontend-sdk-v1) >= 1
Requires:       uke-control >= 2.8.1-7
Requires:       uke-control(frontend-v1) >= 1
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
%autosetup -n %{source_name}-%{version}

%build
%cmake -DBUILD_TESTING=ON
%cmake_build

%install
%cmake_install
desktop-file-validate %{buildroot}%{_datadir}/applications/org.uke.ControlQt.desktop

%check
%ctest

%files
%license LICENSE
%doc README.md
%{_bindir}/uke-control-qt
%{_datadir}/applications/org.uke.ControlQt.desktop
%{_datadir}/icons/hicolor/scalable/apps/org.uke.ControlQt.svg

%changelog
* Tue Nov 03 2026 Uke contributors - 1.0.0-3
- Publish the Qt GUI under its canonical name with matched backend requirements.

* Mon Nov 02 2026 Uke contributors - 1.0.0-2
- Package the Kirigami interface and run standalone native UI checks.

* Sun Nov 01 2026 Uke contributors - 1.0.0-1
- Add the native Qt Widgets frontend with Breeze style and reviewed requests.
