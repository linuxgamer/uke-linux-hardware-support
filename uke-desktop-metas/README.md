# Uke desktop selections

25 October 2026: release `1.0.0-19` recommends Chromium rather than requiring
it. Default DNF weak-dependency handling installs it with KDE, while removing
the browser no longer requires removal of the KDE meta package. Browser data
and original Fedora packages are not modified by this metadata change.

25 October 2026: release `1.0.0-17` requires Control 1.10.2 with corrected
original LED-class provider discovery and the bounded, suspend-aware supervisor.

25 October 2026: release `1.0.0-16` requires Control 1.10.1 so the flashlight
supervisor includes sleep ordering and a suspend-aware deadline.

25 October 2026: release `1.0.0-15` adds a bounded flashlight switch and low-duty
red/green/blue/off controls as clients of Control 1.10.0. Missing exact kernel
providers keep the rows unavailable. The widget performs no hardware writes.

24 October 2026: release `1.0.0-14` gives QML sources a distinct reproducible
timestamp for each monotonic RPM revision. This invalidates Qt's older disk
cache after an update. The package check covers the cached invalid-to-corrected
transition as well as the actual applet component.

24 October 2026: release `1.0.0-13` fixes widget loading on the original
Fedora Qt 6.11 stack by sizing the USB confirmation dialog instead of its
label's read-only implicit width. The package check and source CI compile
the actual applet QML component with original Plasma imports. This check
does not create the widget or invoke device commands; tablet presentation
and physical controls remain separate validation.

24 October 2026: `uke-kde-plasma-meta` recommends the separate
`uke-plasma-controls` RPM. The Uke Tablet Controls entry appears in the original
Plasma system tray. Every device request goes through `uke-control`; the applet
contains no hardware backend. The recommendation installs by default with KDE
and can be removed with `sudo dnf remove uke-plasma-controls` while retaining
the KDE meta package. Default DNF weak-dependency policy respects prior removal.
No GNOME dependency or integration is installed. Tablet UI acceptance remains
separate from package and host checks.

`uke-desktop-metas` records the console-first desktop policy. The optional
`uke-kde-plasma-meta` selects original Fedora Rawhide AArch64 Plasma applications.
It includes Discover's PackageKit backend, Dolphin plugins and administrative
access, KWrite, Chromium, display settings and the common desktop utilities.
Neither package contains a desktop application fork or a hardware service.

The Plasma selection includes original Fedora `plasma-login-manager` and
`kcm-plasmalogin`. It installs no login preset or display-manager activation scriptlet. On an
existing graphical installation, select the standard service explicitly:

```sh
sudo systemctl enable --force plasmalogin.service
```

Remove any previous display manager's explicit startup links before rebooting;
only one manager may run. Preserve the previous package for rollback during
validation. Sensor startup does not depend on a specific login manager.

24 October 2026: the Plasma selection keeps original Fedora `kde-connect` and
its session autostart. The packaged `uke-kdeconnect` firewalld policy permits
the distribution's KDE Connect service from private IPv4 ranges and IPv6
link-local/unique-local addresses, including networks in the `public` zone.
Other traffic continues through the existing firewall policy. No application
fork, extra daemon, automatic pairing or user autostart override is installed.
Installing, upgrading or removing the selection uses Fedora's firewall reload
macro; a reload retains connections and replaces runtime-only firewall edits
with permanent configuration. Globally routed and VPN-specific address ranges
need an administrator's explicit rule if they are used for KDE Connect.

To disable the packaged network permission, preserve an administrator override:

```sh
sudo firewall-cmd --permanent --policy=uke-kdeconnect --add-disable
sudo firewall-cmd --reload
```

The override survives package updates. Existing KDE Connect pairing records and
per-user autostart choices remain authoritative. Source/package validation and
actual discovery on the tablet are recorded separately.
The core console selection keeps
its strict no-Python policy. Since 14 October 2026, unmodified original Fedora
desktop RPMs may retain their Fedora-provided Python dependencies on the Fedora
root. This exception does not cover project-owned code, recovery, initramfs or
installers. The Plasma meta is independent of the strict console meta so these
policies do not conflict during dependency resolution.

The application list was compared with the Nabu KDE manifest and image profile at
[`03a5bdf09ce206410e7f7f1f97871110e66ffa82`](https://github.com/MCC45TR/Nabu-Fedora-Rawhide-Builder/tree/03a5bdf09ce206410e7f7f1f97871110e66ffa82).
Nabu hardware packages and first-run setup are excluded. Uke display,
touch, graphics and sensors require their own drivers and physical tests.

With Fedora `firewalld-filesystem` and `libxml2` installed on the build host,
run `make validate`, `make test-native` and `make srpm` for source and policy
checks. COPR's root `.copr/Makefile` builds both RPMs from this source family.
The optional Plasma RPM owns `plasma.json`, which records its exact application
selection for image composition. Install it explicitly on a Fedora graphical
root; installing the console metadata alone does not select Plasma.

As of 18 October 2026, the revised profile is a package candidate. RPM build, dependency
resolution, graphical application behavior and tablet hardware acceptance are
separate checks. Detailed test records remain in the private project journal.
