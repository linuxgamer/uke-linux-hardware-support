%global debug_package %{nil}
Name: uke-plymonth
Version: 1.0.2
Release: 4%{?dist}
Summary: Senemos Uke boot and shutdown Plymouth theme
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-desktop-packaging
Source0: %{name}-%{version}.tar.xz
BuildArch: noarch
BuildRequires: tar xz
BuildRequires: systemd-rpm-macros
BuildRequires: gcc-c++ plymouth-plugin-script plymouth-plugin-label abattis-cantarell-fonts
Requires: plymouth plymouth-plugin-script plymouth-plugin-label
Requires: abattis-cantarell-fonts
Requires(post): plymouth-scripts
Requires(preun): plymouth-scripts
Provides: plymouth-uke = 1.0.2-1
Obsoletes: plymouth-uke < 1.0.2-1
Conflicts: plymouth-uke
%description
Xiaomi and Fedora artwork with boot/update progress and a pulsing shutdown bar.
The original Fedora Plymouth services show the theme after mounting the root
filesystem and during shutdown/reboot. Installation selects the theme when a
distribution default is in use; local custom themes remain selected.
No initramfs, boot image, partition or boot-slot selection is changed.
%prep
%setup -q
%build
%install
install -d %{buildroot}%{_datadir}/plymouth/themes/senemos-uke
install -m644 src/plymouth-uke/*.png src/plymouth-uke/*.script src/plymouth-uke/*.plymouth %{buildroot}%{_datadir}/plymouth/themes/senemos-uke/
for item in start:boot poweroff:shutdown reboot:reboot halt:shutdown; do
    unit=${item%%:*}
    mode=${item#*:}
    directory=%{buildroot}%{_unitdir}/plymouth-$unit.service.d
    install -d "$directory"
    cat >"$directory/50-uke-theme.conf" <<EOF
[Service]
Environment=PLYMOUTH_FORCE_SCALE=1
ExecStart=
ExecStart=/usr/bin/plymouthd --mode=$mode --pid-file=/run/plymouth/pid --attach-to-session --graphical-boot
EOF
done
%check
c++ -std=c++17 -Wall -Wextra -Werror tests/plymouth-parser.cpp -ldl -o plymouth-parser
./plymouth-parser src/plymouth-uke/plymouth-uke.script "$(plymouth --get-splash-plugin-path)/script.so"
test -s %{buildroot}%{_datadir}/plymouth/themes/senemos-uke/plymouth-uke.script
test -s %{buildroot}%{_datadir}/plymouth/themes/senemos-uke/senemos-uke.plymouth
! grep -Eq '(DeviceScale|/dev/|python)' src/plymouth-uke/*.script
%post
case "$(plymouth-set-default-theme)" in
    bgrt|spinner|charge|text|details|tribar|senemos-uke|'')
        plymouth-set-default-theme senemos-uke ;;
esac
%preun
if [ "$1" -eq 0 ] && [ "$(plymouth-set-default-theme)" = senemos-uke ]; then
    plymouth-set-default-theme --reset
fi
%files
%license LICENSE
%license src/plymouth-uke/LICENSE.Nabu
%doc README.md
%{_datadir}/plymouth/themes/senemos-uke/
%{_unitdir}/plymouth-start.service.d/
%{_unitdir}/plymouth-poweroff.service.d/
%{_unitdir}/plymouth-reboot.service.d/
%{_unitdir}/plymouth-halt.service.d/

%changelog
* Fri Oct 23 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.2-4
- Require the original text renderer used by the theme and script console viewer.
- Check actual label rendering as well as script syntax in the package build.

* Thu Oct 22 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.2-3
- Adapt the attributed Nabu artwork for removable Uke boot and shutdown visuals.
- Use original Fedora services without rebuilding or writing a boot image.

* Tue Oct 20 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.2-1
- Rename the optional theme to uke-plymonth with a versioned upgrade path.

* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-1
- Build an independently scoped Uke development package with explicit readiness.
