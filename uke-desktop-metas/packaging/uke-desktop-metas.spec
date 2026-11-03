%global debug_package %{nil}
Name: uke-desktop-metas
Version: 1.0.0
Release: 19%{?dist}
Summary: Explicit desktop package selections for the Uke development platform
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-desktop-metas
Source0: %{name}-%{version}.tar.xz
ExclusiveArch: aarch64
BuildRequires: tar xz bash jq diffutils firewalld-filesystem firewalld libxml2
BuildRequires: make gcc-c++ pkgconf-pkg-config pkgconfig(Qt6Gui) pkgconfig(Qt6Qml)
BuildRequires: libplasma plasma5support kf6-kirigami
Requires: uke-core-meta >= 1.0.0-3
%description
Desktop selection metadata for Uke. The base installs no graphical session.
Display, touch, GPU and tablet boot remain independently unqualified.

%package -n uke-kde-plasma-meta
Provides: kde-plasma-uke-meta = 1.0.0-19
Obsoletes: kde-plasma-uke-meta < 1.0.0-19
Conflicts: kde-plasma-uke-meta
Summary: Original Fedora Plasma applications for the Uke development platform
Requires: (senemos-uke-linux-kernel-mainline >= 7.2.11-16 or senemos-uke-linux-kernel-mainline-singleboot >= 7.2.11-16)
Requires(posttrans): firewalld
Requires(postun): firewalld
Recommends: uke-plasma-controls >= 1.0.0
# Keep this optional selection independent of the Python-free console meta.
# BEGIN ORIGINAL FEDORA PLASMA PACKAGES
Requires: PackageKit
Requires: ark
Requires: bash
Requires: bluedevil
Requires: breeze-gtk-gtk3
Requires: breeze-gtk-gtk4
Recommends: chromium
Requires: color-filesystem
Requires: colord-kde
Requires: coreutils
Requires: curl
Requires: dnf5-plugins
Requires: dolphin
Requires: dolphin-plugins
Requires: fastfetch
Requires: filesystem
Requires: firewalld
Requires: glibc-all-langpacks
Requires: gstreamer1-plugins-good-qt6
Requires: gwenview
Requires: gzip
Requires: kde-connect
Requires: kde-gtk-config
Requires: kdeplasma-addons
Requires: kdialog
Requires: kcm-plasmalogin
Requires: kinfocenter
Requires: kio-admin
Requires: konsole
Requires: kscreen
Requires: kwalletmanager5
Requires: kwin
Requires: kwrite
Requires: lcms2
Requires: libcanberra-backend-pulse
Requires: mesa-vulkan-drivers
Requires: nano
Requires: okular
Requires: pipewire-utils
Requires: plasma-camera
Requires: plasma-desktop
Requires: plasma-discover
Requires: plasma-discover-notifier
Requires: plasma-discover-offline-updates
Requires: plasma-discover-packagekit
Requires: plasma-keyboard
Requires: plasma-login-manager
Requires: plasma-milou
Requires: plasma-nm
Requires: plasma-pa
Requires: plasma-systemmonitor
Requires: plasma-systemsettings
Requires: plasma-welcome
Requires: plasma-workspace
Requires: plasma5support
Requires: powerdevil
Requires: pulseaudio-utils
Requires: spectacle
Requires: system-config-printer
Requires: systemd-pam
Requires: tar
Requires: udisks2
Requires: vulkan-tools
Requires: wireplumber
Requires: xsettingsd
# END ORIGINAL FEDORA PLASMA PACKAGES

%description -n uke-kde-plasma-meta
An optional application selection for Fedora Rawhide AArch64. All desktop
applications remain the original Fedora packages, including their original
distribution-provided helpers. The selection installs no Uke desktop fork,
display configuration or hardware startup service. Original Fedora Plasma Login
Manager and its settings module are selected without activation scriptlets.
Hardware support is tested separately.
KDE Connect uses Fedora's session autostart and a local-address-only firewalld
policy. Pairing still requires confirmation in the original application.

%package -n uke-plasma-controls
Summary: Removable Plasma system tray controls using uke-control
License: GPL-3.0-or-later
BuildArch: noarch
Requires: uke-control >= 1.10.2
Requires: plasma-workspace plasma5support polkit

%description -n uke-plasma-controls
Uke Tablet Controls for the original Fedora Plasma system tray. Every device
request goes through uke-control. Unsupported controls remain unavailable.
This independent RPM is recommended by the KDE selection and can be removed
without removing the KDE meta package. No GNOME integration is included.

%prep
%setup -q
%build
%install
install -Dm644 src/profile.json %{buildroot}%{_datadir}/senemos/uke/desktops/profile.json
install -Dm644 src/plasma.json %{buildroot}%{_datadir}/senemos/uke/desktops/plasma.json
install -Dm644 src/uke-kdeconnect.xml %{buildroot}%{_prefix}/lib/firewalld/policies/uke-kdeconnect.xml
install -d %{buildroot}%{_datadir}/plasma/plasmoids/org.senemos.uke.controls
cp -a src/plasma-applet/. %{buildroot}%{_datadir}/plasma/plasmoids/org.senemos.uke.controls/
%check
xmllint --nonet --noout --schema /usr/lib/firewalld/xmlschema/policy.xsd src/uke-kdeconnect.xml
bash tests/plasma-profile.sh
%make_build test-applet-load

%posttrans -n uke-kde-plasma-meta
%firewalld_reload

%postun -n uke-kde-plasma-meta
if [ "$1" -eq 0 ]; then
    %firewalld_reload
fi

%files
%license LICENSE
%doc README.md
%{_datadir}/senemos/uke/desktops/profile.json

%files -n uke-kde-plasma-meta
%license LICENSE
%doc README.md
%{_datadir}/senemos/uke/desktops/plasma.json
%{_prefix}/lib/firewalld/policies/uke-kdeconnect.xml

%files -n uke-plasma-controls
%license src/plasma-applet/LICENSE
%doc src/plasma-applet/SOURCE.md
%{_datadir}/plasma/plasmoids/org.senemos.uke.controls/

%changelog
* Sun Oct 25 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-19
- Recommend Chromium by default without requiring it to retain the KDE meta package.

* Sun Oct 25 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-17
- Require the corrected original LED-class provider discovery.

* Sun Oct 25 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-16
- Require the suspend-safe native torch supervisor.

* Sun Oct 25 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-15
- Route bounded torch and low-brightness RGB controls through uke-control.
- Keep both controls unavailable without the exact reviewed kernel providers.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-14
- Give each RPM revision a distinct reproducible source timestamp for QML cache invalidation.
- Exercise the cached invalid-to-corrected QML transition in the mandatory package check.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-13
- Size the USB confirmation dialog without assigning a read-only label property.
- Compile the actual QML component with the original Plasma imports in the package check.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-12
- Document the backend's internal-panel discovery independently of connector numbering.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-11
- Add a separate removable Uke Controls applet recommended only by KDE.
- Delegate all settings and device access to the native uke-control backend.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-10
- Require the package that owns the firewalld policy schema for the mandatory native check.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-9
- Allow original Fedora KDE Connect discovery and connections from local addresses by default.
- Reload the vendor firewalld policy without changing pairing, zone definitions or session overrides.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-5
- Select the original Fedora Plasma Login Manager and settings module.

* Wed Oct 14 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-4
- Add an optional original-Fedora Plasma application selection with PackageKit and Dolphin administration support.
- Keep the strict console policy and the installed login manager unchanged.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-3
- Withdraw the derivative-dependent KDE selection and retain upstream desktop policy metadata.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-2
- Require audited native runtime variants for Python-free Plasma dependencies.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Build an independently scoped Uke development package with explicit readiness.
