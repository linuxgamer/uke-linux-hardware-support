%global debug_package %{nil}
Name: uke-desktop-metas
Version: 1.0.0
Release: 8%{?dist}
Summary: Explicit desktop package selections for the Uke development platform
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-desktop-metas
Source0: %{name}-%{version}.tar.xz
ExclusiveArch: aarch64
BuildRequires: tar xz bash jq diffutils
Requires: uke-core-meta >= 1.0.0-3
Obsoletes: kde-plasma-uke-meta < 1.0.0-3
%description
Desktop selection metadata for Uke. The base installs no graphical session.
Display, touch, GPU and tablet boot remain independently unqualified.

%package -n kde-plasma-uke-meta
Summary: Original Fedora Plasma applications for the Uke development platform
Requires: senemos-uke-linux-kernel-mainline >= 7.2.9-1.35
# BEGIN ORIGINAL FEDORA PLASMA PACKAGES
Requires: PackageKit
Requires: ark
Requires: bash
Requires: bluedevil
Requires: breeze-gtk-gtk3
Requires: breeze-gtk-gtk4
Requires: chromium
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

%description -n kde-plasma-uke-meta
An optional application selection for Fedora 44 AArch64. All desktop
applications remain the original Fedora packages, including their original
distribution-provided helpers. The selection installs no Uke desktop fork,
login-manager preset, display configuration or hardware startup service.
The existing login manager is retained. Hardware support is tested separately.

%package -n gnome-uke-meta
Summary: Original Fedora GNOME applications for the Uke development platform
Requires: senemos-uke-linux-kernel-mainline >= 7.2.9-1.35
# BEGIN ORIGINAL FEDORA GNOME PACKAGES
Requires: firefox
Requires: gnome-calendar
Requires: gnome-console
Requires: gnome-control-center
Requires: gnome-session-wayland-session
Requires: gnome-settings-daemon
Requires: gnome-shell
Requires: gnome-software
Requires: gnome-text-editor
Requires: loupe
Requires: nautilus
Requires: papers
Requires: xdg-desktop-portal-gnome
# END ORIGINAL FEDORA GNOME PACKAGES

%description -n gnome-uke-meta
An optional original Fedora GNOME Wayland session and common applications.
Installation does not replace the login manager or activate a graphical session.
Display, touch and tablet behavior require separate validation.

%package -n plasma-mobile-uke-meta
Summary: Experimental original Fedora Plasma Mobile selection for Uke
Requires: senemos-uke-linux-kernel-mainline >= 7.2.9-1.35
# BEGIN ORIGINAL FEDORA PLASMA MOBILE PACKAGES
Requires: plasma-keyboard
Requires: plasma-mobile
# END ORIGINAL FEDORA PLASMA MOBILE PACKAGES

%description -n plasma-mobile-uke-meta
An experimental original Fedora Plasma Mobile session and on-screen keyboard.
Installation does not replace the login manager or activate a graphical session.
Tablet compatibility has not been tested.

%package -n phosh-uke-meta
Summary: Experimental original Fedora Phosh selection for Uke
Requires: senemos-uke-linux-kernel-mainline >= 7.2.9-1.35
# BEGIN ORIGINAL FEDORA PHOSH PACKAGES
Requires: phosh
# END ORIGINAL FEDORA PHOSH PACKAGES

%description -n phosh-uke-meta
An experimental original Fedora Phosh Wayland session with Fedora dependencies.
Installation does not replace the login manager or activate a graphical session.
Tablet compatibility has not been tested.

%prep
%setup -q
%build
%install
install -Dm644 src/profile.json %{buildroot}%{_datadir}/senemos/uke/desktops/profile.json
install -Dm644 src/plasma.json %{buildroot}%{_datadir}/senemos/uke/desktops/plasma.json
install -Dm644 src/gnome.json %{buildroot}%{_datadir}/senemos/uke/desktops/gnome.json
install -Dm644 src/plasma-mobile.json %{buildroot}%{_datadir}/senemos/uke/desktops/plasma-mobile.json
install -Dm644 src/phosh.json %{buildroot}%{_datadir}/senemos/uke/desktops/phosh.json
%check
bash tests/plasma-profile.sh
bash tests/other-desktop-profiles.sh
%files
%license LICENSE
%doc README.md
%{_datadir}/senemos/uke/desktops/profile.json

%files -n kde-plasma-uke-meta
%license LICENSE
%doc README.md
%{_datadir}/senemos/uke/desktops/plasma.json

%files -n gnome-uke-meta
%license LICENSE
%doc README.md
%{_datadir}/senemos/uke/desktops/gnome.json

%files -n plasma-mobile-uke-meta
%license LICENSE
%doc README.md
%{_datadir}/senemos/uke/desktops/plasma-mobile.json

%files -n phosh-uke-meta
%license LICENSE
%doc README.md
%{_datadir}/senemos/uke/desktops/phosh.json

%changelog
* Fri Oct 16 2026 Uke maintainers <213606978+linuxgamer@users.noreply.github.com> - 1.0.0-8
- Target Fedora 44 in the desktop profile metadata.

* Fri Oct 16 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-7
- Add optional Fedora GNOME selection and experimental Plasma Mobile and Phosh selections.

* Fri Oct 16 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-6
- Keep the base desktop policy independent of the optional Plasma selection.

* Thu Oct 15 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-5
- Permit original Fedora Python dependencies in console and desktop selections.

* Wed Oct 14 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-4
- Add an optional original-Fedora Plasma application selection with PackageKit and Dolphin administration support.
- Keep the strict console policy and the installed login manager unchanged.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-3
- Withdraw the derivative-dependent KDE selection and retain upstream desktop policy metadata.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-2
- Require audited native runtime variants for Python-free Plasma dependencies.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Build an independently scoped Uke development package with explicit readiness.
