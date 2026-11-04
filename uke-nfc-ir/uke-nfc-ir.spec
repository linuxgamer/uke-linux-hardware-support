Name: uke-nfc-ir
Version: 1.1.0
Release: 2%{?dist}
Summary: Native read-only NFC tag client and Uke infrared session access
License: MIT AND (GPL-2.0-only WITH Linux-syscall-note)
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: %{name}-%{version}.tar.gz
Source1: LICENSE
ExclusiveArch: aarch64
BuildRequires: systemd-udev
BuildRequires: systemd-rpm-macros
BuildRequires: gcc-c++ clang compiler-rt libasan libubsan kernel-headers jq binutils
BuildRequires: acl
Requires: systemd-udev
Requires: /usr/bin/ir-ctl
Requires: uke-kernel(nfc-ir-v2) >= 1
Provides: uke-device-access(nfc-ir-v1) = 1
Provides: uke-nfc-tag-client(abi1) = 1

%description
Use logind access control for the exact FM19511 I2C tag and SPI infrared LIRC
providers. Includes the distribution infrared utility through its file
dependency. The matching kernel owns board/SKU selection and protocol handling.
The native tag client reads validated ABI information, cached RF notifications
and explicit bounded memory ranges. It does not write EEPROM or provide an NCI
reader, card-emulation or IR receiver interface.

%prep
%autosetup
cp %{SOURCE1} .

%build
test ! -e /etc/senemos/kernel-update.conf
g++ -std=c++17 %{optflags} -Wall -Wextra -Werror -I include -I src \
    src/main.cpp -o uke-nfc-tag %{build_ldflags}

%install
install -Dm644 70-uke-nfc-ir.rules %{buildroot}%{_udevrulesdir}/70-uke-nfc-ir.rules
install -Dm755 uke-nfc-tag %{buildroot}%{_bindir}/uke-nfc-tag

%check
udevadm verify --resolve-names=never 70-uke-nfc-ir.rules
bash tests/session-acl.sh
bash tests/check.sh
printf '%%s  %%s\n' "$(jq -er .sha256 uapi-source.json)" include/fm19511.h | sha256sum -c -
readelf -h uke-nfc-tag | grep -E 'Machine:.*AArch64'
./uke-nfc-tag --help
set +e
./uke-nfc-tag read 0xffff 2 > invalid.stdout 2> invalid.stderr
status=$?
set -e
test "$status" == 2
test ! -s invalid.stdout
grep -F 'without address wrap' invalid.stderr
test -x %{buildroot}%{_bindir}/uke-nfc-tag
cmp 70-uke-nfc-ir.rules %{buildroot}%{_udevrulesdir}/70-uke-nfc-ir.rules
if find %{buildroot} -type f \( -name '*.py' -o -name '*.pyc' -o -name '*test*' \) -print -quit | grep -q .; then
    echo 'Unexpected test or Python payload' >&2; exit 1
fi

%posttrans
if test -d /run/udev; then
    udevadm control --reload-rules >/dev/null 2>&1 || :
    udevadm trigger --action=change --subsystem-match=misc --sysname-match=fm-ntag >/dev/null 2>&1 || :
    udevadm trigger --action=change --subsystem-match=lirc >/dev/null 2>&1 || :
fi

%files
%license LICENSE LICENSES/GPL-2.0-only.txt LICENSES/Linux-syscall-note
%doc README.md uapi-source.json
%{_udevrulesdir}/70-uke-nfc-ir.rules
%{_bindir}/uke-nfc-tag

%changelog
* Wed Nov 04 2026 Uke contributors - 1.1.0-2
- Restore the exact NFC node's ACL mask before applying session access.
- Reproduce masked and repaired grants with real filesystem ACLs in COPR.

* Wed Nov 04 2026 Uke contributors - 1.1.0-1
- Add a bounded native client for the exact read-only FM19511 kernel API.
- Expose tag information, raw notifications, explicit memory reads and waits.
- Preserve session access and the distribution infrared transmitter utility.
