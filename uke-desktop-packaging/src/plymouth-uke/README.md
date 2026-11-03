# Uke Plymouth theme

22 October 2026. Target: Fedora Rawhide AArch64, Xiaomi Pad 7 / POCO Pad X1.

The artwork and script originate from
[`Nabu-Fedora-Rawhide-Builder` at `67b6bb2398195bd026dae7e6258681a9c3420f08`](https://github.com/MCC45TR/Nabu-Fedora-Rawhide-Builder/tree/67b6bb2398195bd026dae7e6258681a9c3420f08/packaging/copr/senemos-nabu-plymouth/source).
`LICENSE.Nabu` retains its MIT copyright and permission notice. Xiaomi and
Fedora artwork retain their original identities; this package claims no
trademark ownership or endorsement.

The archived source SHA-256 is
`f2869dd426dc7f0c2f7c7f8d02852f0eb4293661dad0dcb801f22d64b5f718ec`.
The five PNG assets are unchanged. Xiaomi is 312 × 312 pixels and Fedora is
427 × 120 pixels, with a 40-pixel lower margin. The script centers artwork using
the current renderer dimensions and pulses the progress bar during shutdown.
Boot and offline-update progress, messages, questions and password prompts
retain the original callbacks.

The existing package name is `uke-plymonth`. Core recommends it through RPM
weak dependencies. Install/upgrade selects this theme when the current choice
is a distribution default; a custom theme remains selected. Removal resets the
selection only when this theme is selected. Distribution service conditions,
including `plymouth.enable=0`, remain authoritative. The four service drop-ins
use Plymouth's standard `--graphical-boot` option and `PLYMOUTH_FORCE_SCALE=1`;
they do not edit the kernel command line or global DPI configuration.

The original Fedora daemon, plugins and systemd units are dependencies. No Nabu
COPR, patched daemon, kernel setting, device geometry, boot writer or Python
payload is installed. The existing minimal initramfs remains unchanged; the
boot splash starts after root mount, and shutdown uses Fedora's original units.
Source, package and host-renderer checks do not establish physical Uke boot,
display handoff, shutdown or offline-update acceptance.
