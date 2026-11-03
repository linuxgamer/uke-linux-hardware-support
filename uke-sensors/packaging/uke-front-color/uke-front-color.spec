Name: uke-front-color
Version: 0.1.0
Release: 5%{?dist}
Summary: Native raw front-color snapshot publisher for Uke
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: aarch64
BuildRequires: gcc-c++ make systemd-rpm-macros
Requires: uke-ssc-fileservice >= 2026.09.21-8
%{?systemd_requires}

%description
An explicitly activated native SSC client publishes bounded raw front-color
snapshots for local readers. Units, calibration and DSP clock mapping remain
unverified. Original SensorProxy continues to provide ambient light. No Python,
factory data, firmware or per-device identifiers are included.

%prep
%setup -q

%build
%set_build_flags
%make_build build/uke-front-color

%install
install -Dm755 build/uke-front-color %{buildroot}%{_libexecdir}/uke-front-color
install -Dm644 packaging/systemd/uke-front-color.service %{buildroot}%{_unitdir}/uke-front-color.service
install -Dm644 packaging/systemd/90-uke-front-color.preset %{buildroot}%{_presetdir}/90-uke-front-color.preset

%check
%make_build test-front-color

%post
%systemd_post uke-front-color.service
%preun
%systemd_preun uke-front-color.service
%postun
%systemd_postun uke-front-color.service

%files
%license LICENSE
%doc docs/FRONT-COLOR.md
%{_libexecdir}/uke-front-color
%{_unitdir}/uke-front-color.service
%{_presetdir}/90-uke-front-color.preset

%changelog
* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-5
- Match OEM wakeup delivery for attribute requests as well as SUID discovery.
- Report startup phases without sensor identifiers or measurement values.

* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-4
- Match OEM wakeup delivery for SUID discovery; keep measurements non-wakeup.

* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-3
- Keep the SUID subscription alive until sensor readiness without timed retries.
- Publish unavailable startup snapshots and test late readiness and cancellation.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Wait for bounded SUID readiness events before metadata and sample admission.
- Log one reason on startup failure without retrying a failed sensor.

* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Add an opt-in raw front-color endpoint with explicit freshness and invalidation.
