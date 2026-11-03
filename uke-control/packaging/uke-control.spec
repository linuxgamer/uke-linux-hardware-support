Name: uke-control
Version: 1.10.2
Release: 1%{?dist}
Summary: Native Uke settings backend and command-line client
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: aarch64 x86_64
BuildRequires: gcc-c++ make pkgconfig(libdrm) pkgconfig(libsystemd) >= 257
BuildRequires: pkgconfig(wayland-client) wayland-devel wayland-protocols-devel
BuildRequires: pkgconfig(libselinux) systemd-rpm-macros
BuildRequires: libdnf5-devel >= 5.4.6.0 pkgconfig(rpm)
BuildRequires: dbus-daemon
Requires: util-linux-core util-linux systemd procps-ng
Requires: coreutils findutils rpm
Requires: ethtool >= 7.1
Requires: systemd-udev
Requires: dracut >= 111
Requires: f2fs-tools >= 1.16.0
Requires: policycoreutils
Requires: kmod
Requires: libdnf5 >= 5.4.6.0
Provides: uke-control(channel-api) = 1
%{?systemd_requires}

%description
C++ backend and terminal client for Uke device settings. A systemd service
records the partition layout and prefers an existing linux_swap partition;
otherwise it creates a swap file using half RAM by default. Existing partitions
are never formatted. Charge and USB controls follow real driver capabilities.

%package tests
Summary: Development-only Uke display, touch and sensor diagnostics
Requires: %{name}%{?_isa} = %{version}-%{release}
Requires: sysstat drm-utils

%description tests
Development diagnostics for DPU refresh measurement, Wayland presentation,
continuous touch, SensorProxy and SSC sensors. Excluded from release image
package selections. Numeric readings alone do not establish physical acceptance.

%prep
%setup -q

%build
%set_build_flags
%make_build

%install
install -Dm755 build/uke-control %{buildroot}%{_bindir}/uke-control
install -Dm755 build/uke-control-packages %{buildroot}%{_libexecdir}/uke-control/packages
install -Dm755 build/resize-system %{buildroot}%{_libexecdir}/uke-control/resize-system
install -Dm755 packaging/vrr-min-load %{buildroot}%{_libexecdir}/uke-control/vrr-min-load
install -Dm644 packaging/60-uke-control-vrr.conf %{buildroot}%{_prefix}/lib/modprobe.d/60-uke-control-vrr.conf
install -Dm755 build/libuke-control.so.0 %{buildroot}%{_libdir}/libuke-control.so.0
install -Dm644 src/backend.hpp %{buildroot}%{_includedir}/uke-control/backend.hpp
install -Dm644 src/charge-policy.hpp %{buildroot}%{_includedir}/uke-control/charge-policy.hpp
install -Dm644 src/front-color.hpp %{buildroot}%{_includedir}/uke-control/front-color.hpp
install -Dm644 src/desktop.hpp %{buildroot}%{_includedir}/uke-control/desktop.hpp
install -Dm644 src/vrr.hpp %{buildroot}%{_includedir}/uke-control/vrr.hpp
install -Dm755 build/uke-control-tests %{buildroot}%{_libexecdir}/uke-control/tests
install -Dm755 build/ssc-discovery %{buildroot}%{_libexecdir}/uke-control/ssc-discovery
install -Dm755 build/ssc-sample %{buildroot}%{_libexecdir}/uke-control/ssc-sample
install -Dm644 packaging/uke-control.service %{buildroot}%{_unitdir}/uke-control.service
install -Dm644 packaging/uke-control-wake.service %{buildroot}%{_unitdir}/uke-control-wake.service
install -Dm644 packaging/uke-control-cdc.service %{buildroot}%{_unitdir}/uke-control-cdc.service
install -Dm644 packaging/uke-control-flashlight.service %{buildroot}%{_unitdir}/uke-control-flashlight.service
install -Dm644 packaging/90-uke-control-cdc.rules %{buildroot}%{_udevrulesdir}/90-uke-control-cdc.rules
install -d %{buildroot}%{_unitdir}/sleep.target.requires
ln -s ../uke-control-wake.service %{buildroot}%{_unitdir}/sleep.target.requires/uke-control-wake.service
install -Dm644 packaging/80-uke-control.preset %{buildroot}%{_presetdir}/80-uke-control.preset
install -Dm644 packaging/uke-control-moisture-notify.service %{buildroot}%{_userunitdir}/uke-control-moisture-notify.service
install -Dm644 packaging/80-uke-control-user.preset %{buildroot}%{_userpresetdir}/80-uke-control-user.preset
install -Dm644 packaging/uke-control-tests.desktop %{buildroot}%{_datadir}/applications/uke-control-tests.desktop
install -d -m700 %{buildroot}%{_localstatedir}/lib/uke-control

%check
%make_build check

%post
%systemd_post uke-control.service uke-control-wake.service uke-control-cdc.service
/usr/bin/udevadm control --reload >/dev/null 2>&1 || :
%systemd_user_post uke-control-moisture-notify.service
%preun
%systemd_preun uke-control.service uke-control-wake.service uke-control-cdc.service uke-control-flashlight.service
%systemd_user_preun uke-control-moisture-notify.service
%postun
%systemd_postun uke-control.service uke-control-wake.service uke-control-cdc.service uke-control-flashlight.service
/usr/bin/udevadm control --reload >/dev/null 2>&1 || :
%systemd_user_postun uke-control-moisture-notify.service

%files
%license LICENSE
%doc README.md CHARGING.md DISPLAY.md CHANNELS.md SENSORS.md DESKTOP.md HARDWARE.md DIAGNOSTIC.md RESIZE.md LIGHTS.md
%{_bindir}/uke-control
%dir %{_libexecdir}/uke-control
%{_libexecdir}/uke-control/packages
%{_libexecdir}/uke-control/resize-system
%{_libexecdir}/uke-control/vrr-min-load
%{_prefix}/lib/modprobe.d/60-uke-control-vrr.conf
%{_libdir}/libuke-control.so.0
%{_includedir}/uke-control/*.hpp
%{_unitdir}/uke-control.service
%{_unitdir}/uke-control-wake.service
%{_unitdir}/uke-control-cdc.service
%{_unitdir}/uke-control-flashlight.service
%{_udevrulesdir}/90-uke-control-cdc.rules
%dir %{_unitdir}/sleep.target.requires
%{_unitdir}/sleep.target.requires/uke-control-wake.service
%{_presetdir}/80-uke-control.preset
%{_userunitdir}/uke-control-moisture-notify.service
%{_userpresetdir}/80-uke-control-user.preset
%dir %attr(0700,root,root) %{_localstatedir}/lib/uke-control

%files tests
%{_libexecdir}/uke-control/tests
%{_libexecdir}/uke-control/ssc-discovery
%{_libexecdir}/uke-control/ssc-sample
%{_datadir}/applications/uke-control-tests.desktop

%changelog
* Sun Oct 25 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.10.2-1
- Validate the parent controller DT link and exact LED label used by the original LED class.
- Cover providers without a per-LED OF symlink while retaining all mapping and current checks.

* Sun Oct 25 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.10.1-1
- Stop the torch service before sleep and count suspended time in the worker deadline.
- Exercise the actual worker's expiry, failure cleanup and ownership checks with simulated IO.

* Sun Oct 25 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.10.0-1
- Add exact-profile torch and RGB control through original kernel LED class interfaces.
- Limit torch to the 20 mA kernel profile and a supervised 60-second lifetime.
- Keep RGB at low fixed PWM duty and refuse missing or mismatched providers.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.9.1-1
- Restrict speed profiles to speed bits and preserve pause/other advertisements.
- Withdraw the uninstalled 1.9.0 candidate before delivery.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.9.0-1
- Add disconnected DP lane ceilings, per-device USB power and RTL8153 link profiles.
- Query original ethtool capabilities and require link-down speed changes with readback.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.8.1-1
- Resolve exactly one internal DSI output instead of assuming connector numbering.
- Preserve the published DisplayPort profile commands and their admission checks.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.8.0-1
- Add standard Type-C display lane profile requests and cached link observations
- Refuse unavailable firmware controls and the unimplemented MST profile
- Preserve the 1.7.0 KScreen backend and desktop commands

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.7.0-1
- Add bounded KScreen panel capability queries and session rotation/brightness commands.
- Keep device operations in the native backend for the removable KDE tray applet.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.6.2-1
- Record loaded MSM, panel and touch module build IDs and taint in private diagnostics.
- Preserve bounded collection, existing package dependencies and hardware policies.

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.6.1-1
- Combine saved CDC notification reload with the existing VRR minimum controls
- Preserve the Release 2 module loader and older-kernel fallback

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.6.0-2
- Keep the saved floor outside modprobe options to preserve older rollback kernels

* Sat Oct 24 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.6.0-1
- Add a validated per-boot VRR minimum policy and loaded/pending status
- Preserve normal module loading when the kernel lacks the floor policy API

* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.5.3-1
- Reapply saved CDC policy through reload without exhausting successful start limits
- Preserve explicit on/off application and propagate reload errors

* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.5.2-1
- Wait without spinning when a child closes its output before exiting
- Test the actual runner's EOF, exit status, timeout and output bounds

* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.5.1-1
- Confirm offline expansion and normal reboot; clarify read-only capacity checks
- Admit only the exact standard dracut character nodes in the audited RAM payload

* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.5.0-2
- Keep the CDC fixture process stub separate for GCC 16 AArch64 diagnostics.

* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.5.0-1
- Save explicit CDC ACM startup policy and reconcile USB readiness events.
- Retain one-shot requests, device-role guards and owned-gadget cleanup.

* Thu Oct 22 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.4.0-1
- Prepare explicit offline F2FS userdata expansion at the next normal reboot.
- Verify physical root, power, geometry, identity and unmounted exclusive access.
- Preserve existing shutdown environments and retain read-only capacity checks.

* Thu Oct 22 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.3.0-3
- Keep sensor, display and power reports within their selected subsystem.
- Read existing SensorProxy and UPower interfaces without activation.

* Thu Oct 22 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.3.0-2
- Construct collection results directly for GCC 16 LTO compatibility.
- Retain compiler warnings, fixture coverage and collector error receipts.

* Thu Oct 22 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.3.0-1
- Collect private single-file diagnostics in the caller's working directory.
- Include hardware paths, subsystem logs, RPM/DNF, boot and service evidence.
- Bound collection and preserve existing reports and unavailable-source receipts.

* Wed Oct 21 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.2.0-1
- Persist native wake permissions and apply them before system sleep.
- Retain a recovery key and disable unqualified Bluetooth UART wake by default.
- Report unsupported charge-event and touch-gesture controls explicitly.

* Wed Oct 21 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.1.2-1
- Capture bounded interval CPU/thread samples and cached GPU execution counters.
- Provide original Fedora diagnostic tools with the optional tests package.
- Correct the backend and private report application version.

* Wed Oct 21 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.1.1-1
- Exclude ordinary class attributes from hardware device and policy inventories.
- Retain directory containment and unknown metadata without reading controls.

* Wed Oct 21 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.1.0-1
- Include cached CPU cache, frequency-policy, DRM and accelerator inventories.
- Keep unknown geometry and configured DT paths separate from hardware acceptance.

* Wed Oct 21 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.24-1
- Scope reviewed vendor transitions to exact package, repository and vendor jobs.
- Retain global vendor restrictions, unrelated packages and existing boot/private-data guards.

* Wed Oct 21 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.23-1
- Retain device-private touchscreen calibration outside public channel synchronization.
- Exclude public candidates for that private identity and refuse indirect obsoletion.

* Wed Oct 21 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.22-1
- Infer an unset channel only from one enabled normal Uke COPR repository.
- Refuse malformed channel records and ambiguous defaults; preserve explicit choices.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.21-1
- Restore moisture notifications, desktop and hardware inspection, and USB routing reports.
- Preserve the F2FS swap allocator and signed DNF channel management.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-26
- Match the exact Uke USB-switch compatible separately from the audio codec.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-25
- Record cached audio, media, input and configured hardware paths without opening devices.
- Preserve ancestor status, missing endpoints and private report failure receipts.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-24
- Read existing standard desktop interfaces without activation or controls.
- Preserve units, unknown values and desktop evidence in private reports.
- Retain the cached UCSI and DisplayPort routing diagnostics from release23.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-23
- Include cached UCSI transport, PHY/AUX routing and Type-C alternate-mode evidence.
- Keep driver completion separate from link training and physical display acceptance.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-22
- Preserve read-only battery and protection evidence in private device reports.
- Retain unavailable-command receipts without exposing private report data.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-21
- Preserve presentation-deadline fixes with the moisture notification client.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-20
- Add cached moisture and charging-fault notifications in the desktop session.
- Distinguish firmware input-inhibit readback from physical cutoff acceptance.

* Mon Oct 19 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-19
- Preserve absolute presentation deadlines without accumulating rendering cost.

* Mon Oct 19 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-18
- Preserve both class object IDs and device names without duplicate JSON keys.

* Mon Oct 19 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-17
- Add cached SCMI and remote-processor status to the CLI and private reports.
- Keep firmware reads separate from feature qualification and active controls.

* Mon Oct 19 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-16
- Preserve diagnostic receipts when inventory or device-status collection fails.
- Include bounded process, cgroup, boot, filesystem and pressure context.

* Mon Oct 19 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-15
- Read GPT swap labels without a second whole-device filesystem scan.
- Preserve filesystem labels, ambiguity refusal and selected swap validation.
- Serialize saved inventory updates with the existing manager lock.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-14
- Keep public channel configuration readable when the administrator uses umask 077.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-13
- Refuse channel transitions that would remove the manager's channel interface.
- Require the native package API used by the channel backend at runtime.
- Include the coordinated one-shot existing charge-band client without retries.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-12
- Add native channel synchronization, distribution updates and live sensor views.
- Preserve installed kernel tuples and journal interrupted package changes.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-11
- Add the explicit preconfigured-cap command without a metadata-query fallback.
- Label the configured VRR mode as an upper limit beside measured DPU cadence.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-10
- Add an explicit native read-only firmware threshold query.
- Keep diagnostic GETs separate from metadata status and qualified control requests.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-9
- Report native charging metadata without probing guarded threshold attributes.
- Preserve helper failures and mark unavailable generic threshold reads explicitly.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-8
- Select the active internal DSI CRTC when external display resources also exist.
- Use its resource index for vblank measurements and reject ambiguous selection.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-7
- Query capability names and verify their provided ABI versions explicitly.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-6
- Read the optional native front-color snapshot with independent freshness checks.
- Invoke the one-shot manual CDC API without a persistent service or saved opt-in.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-5
- Connect qualified charge requests and metadata to the native backend
- Add descriptor-bound front color-sensor diagnostics to development tests

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-4
- Add cached USB topology, bounded observation, guarded Type-C requests and explicit CDC opt-in.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-3
- Integrate read-only charging policy previews with explicit unavailable actuator state.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Preserve distribution linker hardening and include the actual UTC report timestamp.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Add a shared C++ backend, persistent swap policy and separate development diagnostics.
