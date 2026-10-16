Name: libssc
Version: 0.4.4
Release: 4%{?dist}
Summary: Native Qualcomm Sensor Core client library
License: GPL-3.0-or-later
URL: https://codeberg.org/DylanVanAssche/libssc
Source0: libssc-v%{version}.tar.gz
Patch0: 0001-optional-python-mocks.patch
Patch1: 0002-report-qrtr-error.patch
Patch2: 0003-gyroscope-cli-units.patch
BuildRequires: gcc ninja-build pkgconf-pkg-config
BuildRequires: meson >= 1.4.0
BuildRequires: python3
BuildRequires: pkgconfig(glib-2.0) pkgconfig(gio-2.0) pkgconfig(qmi-glib)
BuildRequires: pkgconfig(libprotobuf-c) protobuf-c-compiler protobuf-compiler
ExclusiveArch: aarch64

%description
Upstream SSC library and diagnostic client. Sensor availability and calibration
depend on device firmware.

%package devel
Summary: Development files for libssc
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
Headers and pkg-config metadata for native SSC clients.

%prep
echo '716d6bd6b34d2d753060c6b54c9a87e34fae75b724c763bf9ef487efa3621587  %{SOURCE0}' | sha256sum -c -
%autosetup -n libssc -p1

%build
%set_build_flags
meson setup build --prefix=%{_prefix} --libdir=%{_lib} --buildtype=release -Dtests=false
meson compile -C build

%install
DESTDIR=%{buildroot} meson install -C build --no-rebuild

%check
test -s build/src/libssc.so.2

%files
%license LICENSE
%{_libdir}/libssc.so.2
%{_bindir}/ssccli

%files devel
%{_includedir}/libssc/
%{_libdir}/libssc.so
%{_libdir}/pkgconfig/libssc.pc

%changelog
* Fri Oct 16 2026 Uke maintainers <213606978+linuxgamer@users.noreply.github.com> - 0.4.4-4
- Use distribution build tools without exact Rawhide package pins.

* Thu Oct 15 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.4.4-3
- Drop the unnecessary Python file exclusion check.

* Wed Oct 14 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.4.4-2
- Publish the reviewed SSC preview with numeric RPM release metadata.

* Wed Oct 14 2026 Uke Linux project <75160848+MCC45TR@users.noreply.github.com> - 0.4.4-1.uke1
- Package the pinned upstream native library without Python mock payloads.
- Report QRTR errors and label gyroscope output in radians per second.
