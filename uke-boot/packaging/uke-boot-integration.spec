%global debug_package %{nil}
Name: uke-boot-integration
Version: 0.1.0
Release: 14%{?dist}
Summary: Uke image configuration and read-only boot prerequisite inspection
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-boot
Source0: %{name}-%{version}.tar.xz
ExclusiveArch: aarch64
BuildRequires: gcc-c++ make systemd-rpm-macros
BuildRequires: pkgconfig(libsystemd) pkgconfig(wayland-client) pkgconfig(wayland-server)
Requires: kmod bash systemd NetworkManager openssh-server

%description
Reviewed Uke partition-role, command-line and dracut input templates, with a
native read-only prerequisite inspection command. Templates require explicit
host-composer application; installation does not regenerate initramfs, select
a default boot entry, alter EFI variables or write Android partitions.
No UEFI firmware or proprietary firmware blob is included. Package success
does not establish RAM/UFS/firmware handoff or physical tablet boot.

%package -n uke-esp32-cdc
Summary: Native ESP32-S3 CDC journal forwarding for the Uke Core debug profile
Requires: %{name} = %{version}-%{release}
Requires: systemd
%description -n uke-esp32-cdc
Native raw-TTY configuration and real-root journal forwarding to an explicitly
configured ttyACM USB-host port. Core composition may apply the separate original
systemd debug-shell override for HID-driven VT2 commands and journal output.
Installation activates no service, gadget, boot entry or partition write.

%package -n uke-boot-recovery
Summary: Native Uke reboot commands and a three-minute startup recovery guard
Requires: %{name} = %{version}-%{release}
Requires: systemd systemd-udev
%description -n uke-boot-recovery
C++ recovery/bootloader commands and a native readiness observer. Installing
this optional development package enables the guard on subsequent boots and
provides local reboot aliases without replacing Fedora-owned executables.
The host composer must include its dracut module to cover initramfs stalls.
No Android, boot, EFI or partition data is written by package installation.

%package -n uke-cdc-debug
Summary: Explicit local USB serial development console for Uke
Requires: %{name} = %{version}-%{release}
Requires: bash systemd
%description -n uke-cdc-debug
Original Bash development console over the standard CDC ACM gadget. It requires
the explicit senemos.debug=cdc-acm kernel argument and manual service enablement.
The console grants local root access over USB. USB networking stays optional.

%package -n uke-wifi-factory
Summary: Read-only Uke factory WLAN identity for NetworkManager
Requires: systemd NetworkManager
%description -n uke-wifi-factory
Reads and validates each installation's factory WLAN address from its physical
read-only persist partition. Publishes a private runtime NetworkManager default
before network startup. No device address or factory record is redistributed.
The image must provide the protected persist mount and enable the service.

%prep
%setup -q
%build
%{__cxx} %{optflags} -std=c++17 -ffile-prefix-map=$PWD=. src/boot-status.cpp -o uke-boot-status %{build_ldflags}
%{__cxx} %{optflags} -std=c++17 -ffile-prefix-map=$PWD=. src/esp32-cdc.cpp -o uke-esp32-cdc %{build_ldflags}
%{__cxx} %{optflags} -std=c++17 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/wifi-factory.cpp -o uke-wifi-factory %{build_ldflags}
%{__cxx} %{optflags} -std=c++17 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/boot-guard.cpp -o uke-boot-guard $(pkg-config --cflags --libs libsystemd wayland-client) %{build_ldflags}
%{__cxx} %{optflags} -std=c++17 -Wall -Wextra -Werror -ffile-prefix-map=$PWD=. src/reboot.cpp -o uke-reboot $(pkg-config --cflags --libs libsystemd) %{build_ldflags}
%check
%{__cxx} -std=c++17 -Wall -Wextra -Werror -O2 tests/wifi-factory.cpp -o wifi-factory-contract
./wifi-factory-contract
bash tests/startup-contract.sh
bash tests/startup-native.sh
%install
install -Dm755 uke-boot-status %{buildroot}%{_bindir}/uke-boot-status
install -Dm755 uke-wifi-factory %{buildroot}%{_libexecdir}/senemos-uke/uke-wifi-factory
install -Dm644 src/units/uke-wifi-factory.service %{buildroot}%{_unitdir}/uke-wifi-factory.service
install -d %{buildroot}%{_datadir}/senemos/boot/uke
install -m644 src/profiles/* %{buildroot}%{_datadir}/senemos/boot/uke/
install -Dm644 src/units/uke-cdc-acm.service %{buildroot}%{_unitdir}/uke-cdc-acm.service
install -Dm644 src/units/uke-cdc-shell.service %{buildroot}%{_unitdir}/uke-cdc-shell.service
install -Dm755 src/uke-usb-network.sh %{buildroot}%{_libexecdir}/senemos-uke/uke-usb-network
install -Dm644 src/units/uke-usb-network.service %{buildroot}%{_unitdir}/uke-usb-network.service
install -Dm644 src/units/uke-usb-network-shell.service %{buildroot}%{_unitdir}/uke-usb-network-shell.service
install -Dm644 src/profiles/uke-coresight-bringup.conf %{buildroot}%{_prefix}/lib/modprobe.d/uke-coresight-bringup.conf
install -Dm755 uke-esp32-cdc %{buildroot}%{_libexecdir}/senemos-uke/uke-esp32-cdc
install -Dm644 src/units/uke-esp32-cdc-log.service %{buildroot}%{_unitdir}/uke-esp32-cdc-log.service
install -Dm755 uke-reboot %{buildroot}%{_bindir}/uke-reboot
install -Dm755 uke-boot-guard %{buildroot}%{_libexecdir}/senemos-uke/uke-boot-guard
install -Dm644 src/units/uke-boot-guard.service %{buildroot}%{_unitdir}/uke-boot-guard.service
install -Dm644 src/profiles/uke-reboot.conf %{buildroot}%{_prefix}/lib/modules-load.d/uke-reboot.conf
install -Dm755 src/dracut/93uke-boot-guard/module-setup.sh %{buildroot}%{_prefix}/lib/dracut/modules.d/93uke-boot-guard/module-setup.sh
install -d %{buildroot}%{_unitdir}/sysinit.target.wants %{buildroot}/usr/local/{bin,sbin}
ln -s ../uke-boot-guard.service %{buildroot}%{_unitdir}/sysinit.target.wants/uke-boot-guard.service
ln -s ../../../usr/bin/uke-reboot %{buildroot}/usr/local/bin/reboot
ln -s ../../../usr/bin/uke-reboot %{buildroot}/usr/local/sbin/reboot
%files
%license LICENSE
%doc README.md
%{_bindir}/uke-boot-status
%{_datadir}/senemos/boot/uke
%{_unitdir}/uke-cdc-acm.service
%{_libexecdir}/senemos-uke/uke-usb-network
%{_unitdir}/uke-usb-network.service
%{_unitdir}/uke-usb-network-shell.service
%{_prefix}/lib/modprobe.d/uke-coresight-bringup.conf

%files -n uke-esp32-cdc
%license LICENSE
%{_libexecdir}/senemos-uke/uke-esp32-cdc
%{_unitdir}/uke-esp32-cdc-log.service

%files -n uke-cdc-debug
%{_unitdir}/uke-cdc-shell.service

%files -n uke-wifi-factory
%license LICENSE
%{_libexecdir}/senemos-uke/uke-wifi-factory
%{_unitdir}/uke-wifi-factory.service

%files -n uke-boot-recovery
%license LICENSE
%{_bindir}/uke-reboot
/usr/local/bin/reboot
/usr/local/sbin/reboot
%{_libexecdir}/senemos-uke/uke-boot-guard
%{_unitdir}/uke-boot-guard.service
%{_unitdir}/sysinit.target.wants/uke-boot-guard.service
%{_prefix}/lib/modules-load.d/uke-reboot.conf
%{_prefix}/lib/dracut/modules.d/93uke-boot-guard

%changelog
* Sat Oct 17 2026 Uke maintainers <213606978+linuxgamer@users.noreply.github.com> - 0.1.0-14
- Recognize the reviewed OEM Uke device-tree identity during stock boot.

* Fri Oct 16 2026 Uke maintainers <213606978+linuxgamer@users.noreply.github.com> - 0.1.0-13
- Accept an explicitly selected userdata Linux root in read-only boot inspection.

* Thu Oct 15 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-12
- Use the Fedora C++ runtime without a special Python-free package dependency.

* Wed Oct 14 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-11
- Read the local factory WLAN identity from protected persist before NetworkManager.
- Validate unicast records without shipping device-specific addresses.

* Wed Oct 14 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-10
- Package the opt-in serial development console without readline or TTY resets.

* Wed Oct 14 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-9
- Add native reboot routing and a three-minute initramfs/real-root readiness guard.
- Keep Fedora reboot executables and the stock Uke reboot-mode mapping intact.
* Wed Oct 14 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-8
- Recognize the reviewed 7.2.10 kernel while retaining the 7.2.9 fallback.
- Keep foreign and unreviewed kernel releases outside the identity check.
* Mon Oct 12 2026 Mürşit Can Cihan <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-7
- Avoid blocking readline and terminal resets on the development gadget shell.
- Preserve the bounded Uke identity and USB TTY prerequisite check.
* Sun Oct 11 2026 Mürşit Can Cihan <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-6
- Allow native boot helpers to coexist with original Fedora KDE dependencies.
- Keep the Core image's independent no-Python payload audit unchanged.
* Sun Oct 11 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-5
- Contain unqualified CoreSight autoload during initial stock-ABL bring-up.
- Add opt-in USB ACM/ECM units and explicit identity/image input templates.
- Accept the physical linux partition name in read-only root inspection.
* Tue Oct 06 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-4
- Accept the filesystem label used by dynamically sized Core images.
- Preserve rejection of a later foreign root command-line override.
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-3
- Add native ESP32-S3 USB-host journal forwarding and HID/VT2 Core inputs.
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Add native CDC prerequisite inspection and opt-in Core debug integration.
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Provide explicit image inputs and native read-only prerequisite inspection.
