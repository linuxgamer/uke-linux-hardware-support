%global debug_package %{nil}
%global __os_install_post %{nil}
%global _build_id_links none

Name: uke-orangefox-recovery
Version: 12.0~preview3.20261010
Release: 2%{?dist}
Summary: OrangeFox recovery preview images for Xiaomi Pad 7 and POCO Pad X1
License: GPL-2.0-only AND GPL-3.0-or-later AND Apache-2.0 AND MIT AND BSD-2-Clause
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-orangefox-packaging
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: aarch64
BuildRequires: bash tar gzip coreutils jq
Provides: uke-recovery-image
# No installer, boot hook or firmware/runtime dependency is attached.

%description
DNF-managed, checksum-verified delivery of the existing OrangeFox v003 preview.
Installs images and records as data without flashing, unlocking, running an
installer or changing boot selection. The IMG requires the installed Android
boot stack; it is not a temporary fastboot boot image. ZIP installation is
unavailable. Firmware compatibility and physical rollback require separate tests.
The source snapshots and exact upstream revisions are listed in SOURCE-OFFER.

%prep
%setup -q
sha256sum -c PACKAGE-SHA256SUMS
jq -e '.device == "uke" and .validation.payload_privacy == true and .validation.physical_device == false' ARTIFACT-MANIFEST.json

%build
# Images were built at the exact published source checkpoint; this package
# verifies and delivers that release. It does not claim a fresh recovery build.

%install
destination=%{buildroot}%{_datadir}/senemos/recovery/uke/%{version}
mkdir -p "$destination"
install -m644 *.img *.zip ARTIFACT-MANIFEST.json release-lock.json \
    PACKAGE-SHA256SUMS ORANGEFOX-SOURCE-PINS.xml STOCK-KERNEL-SOURCE.json "$destination/"
# Source/license records describe the complete artifact; no tablet script runs.
install -m644 DELIVERY.md SOURCE-OFFER.md INSTALL.md LICENSE "$destination/"
cp -r licenses "$destination/"

%check
for image in *.img; do test "$(head -c8 "$image")" = 'ANDROID!'; done
sha256sum -c PACKAGE-SHA256SUMS

%files
%license LICENSE
%license licenses/*.txt
%doc DELIVERY.md SOURCE-OFFER.md
%{_datadir}/senemos/recovery/uke/%{version}/

%changelog
* Thu Oct 15 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 12.0~preview3.20261010-2
- Allow Python in the pinned recovery payload while retaining privacy and image checks.

* Wed Oct 14 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 12.0~preview3.20261010-1
- Deliver the published v003 preview and matching source records.
- Remove the dependency on the withdrawn Alpha 1 release.

* Sun Oct 04 2026 Senemos Maintainers <MCC45TR@users.noreply.github.com> - 12.0~alpha1.20260930-1
- Deliver the pinned Uke alpha images and evidence without device writes.
