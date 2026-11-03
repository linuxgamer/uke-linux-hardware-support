Name: uke-nfc-ir
Version: 1.0.0
Release: 1%{?dist}
Summary: Local-session access to the native Uke NFC tag and infrared transmitter
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
Source1: LICENSE
BuildArch: noarch
BuildRequires: systemd-udev
BuildRequires: systemd-rpm-macros
Requires: systemd-udev
Requires: /usr/bin/ir-ctl
Requires: uke-kernel(nfc-ir-v1) >= 1
Provides: uke-device-access(nfc-ir-v1) = 1

%description
Use logind access control for the exact FM19511 I2C tag and SPI infrared LIRC
providers. Includes the distribution infrared utility through its file
dependency. The matching kernel owns board/SKU selection and protocol handling.

%prep
%autosetup
cp %{SOURCE1} .

%install
install -Dm644 70-uke-nfc-ir.rules %{buildroot}%{_udevrulesdir}/70-uke-nfc-ir.rules

%check
udevadm verify --resolve-names=never 70-uke-nfc-ir.rules

%posttrans
if test -d /run/udev; then
    udevadm control --reload-rules >/dev/null 2>&1 || :
    udevadm trigger --action=change --subsystem-match=misc --sysname-match=fm-ntag >/dev/null 2>&1 || :
    udevadm trigger --action=change --subsystem-match=lirc >/dev/null 2>&1 || :
fi

%files
%license LICENSE
%doc README.md
%{_udevrulesdir}/70-uke-nfc-ir.rules
