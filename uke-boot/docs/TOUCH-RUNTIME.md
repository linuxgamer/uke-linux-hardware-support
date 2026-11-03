# Touch runtime migration

23 October 2026. The 1.7.0 source candidate merges the existing public touch
runtime into boot integration. It does not bundle firmware, configuration or
private reference packets. Those remain in `uke-local-touch-data`.

The service keeps its name and resource limits. Its native launcher checks the
Uke/SM7675 identity and a unique reviewed SPI leaf. The legacy DT binding uses
the pinned, unchanged legacy processor with its packaged panel-standby fix.
The new kernel THP binding executes only
`/usr/lib/modules/<running-release>/uke/uke-touch-capture`. Missing, untrusted
or mismatched clients fail closed; the new binding never falls back to SPI.

The standard firmware alias points to the separately provisioned normal image.
The kernel validates that image before use. Neither the image nor calibration
bytes enter the public source or binary RPMs. The old touchscreen module
dependency, uinput/modprobe settings, udev rules and service preset remain.
The former package name is retained only as a compatibility capability.

Bluetooth and touch migration records use distinct keys. Each expanded RPM
fragment runs in its own subshell, so an absent Bluetooth package cannot skip
touch-state transfer. Existing enabled/active state is restored; disabled and
masked state is respected. Failed restoration retains its receipt.

The native updater requires `touch_client_sha256` when the exact kernel THP
module is in the module inventory. It verifies the executable AArch64 ELF,
digest and membership in the matching signed module RPM. A stray client field
or missing client is refused. Legacy bundles remain supported, including the
accepted rollback image. The kernel package must require the updater's
`kernel-touch-client-v1` capability before carrying this format.

Host source/export, launcher, queue, expanded scriptlet, updater and USB
fixtures are separate from physical touch acceptance. Preserve the accepted
kernel and old signed RPMs until paired COPR/DNF deployment, boot, contacts,
display lifecycle, VRR and suspend trials pass. No hardware case is closed by
this source candidate.
