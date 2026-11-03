Name: uke-proximity-capture
Version: 0.1.0
Release: 1%{?dist}
Summary: Bounded Uke proximity transport diagnostic
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.xz
ExclusiveArch: aarch64
BuildRequires: gcc-c++ make
Requires: uke-ssc-fileservice

%description
A root-only Uke diagnostic captures bounded raw events from one metadata-
qualified default proximity endpoint. Fixed modes compare report framing and
empty request bodies. No daemon, calibration, rate setting or near/far mapping
is installed. Raw output belongs in private laboratory records.

%prep
%setup -q

%build
%set_build_flags
%make_build test-proximity-capture

%install
install -Dm750 build/uke-proximity-capture %{buildroot}%{_sbindir}/uke-proximity-capture

%check
./build/proximity-capture-test

%files
%license LICENSE
%doc docs/PROXIMITY-CAPTURE.md
%{_sbindir}/uke-proximity-capture

%changelog
* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Package the bounded native proximity comparison with four fixed request modes.
- Refuse non-Uke hosts, ambiguous endpoints and malformed or foreign events.
