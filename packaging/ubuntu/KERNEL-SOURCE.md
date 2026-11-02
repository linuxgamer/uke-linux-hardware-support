# Shared kernel source and Ubuntu candidate boundary

Recorded 1 November 2026. The resumed Ubuntu test-image request identifies
7.3.1-1006 in **uke-linux-test:custom:power-20261031**, not main unstable.
COPR build 11240463 succeeded for Rawhide AArch64. Its source commit is
`fcbdae4b757732f0f0e3162c610f19abae290832`; emitted source RPM SHA-256 is
`70f151855f727c9d83c3bedc35ff52dd60cb222b2d741818472dced458bcd60c`.
The Fedora DE release remains on main unstable 115. Numeric release ordering
does not promote an experimental channel or replace its source composition.

`prepare-kernel.sh` extracts that emitted source RPM, verifies the unchanged
Linux source, adaptation/profile/boot archive identities, native profile
inventory and complete boot source manifest. The retained profile describes
release1003 plus four bounded LMH overlays, with physical acceptance false.
Git checkout alone is not this composition. No COPR worker, native preparer,
compiler fixture, boot producer or new kernel was executed during assembly.

The signed Linux archive is retained byte for byte as two source chunks of at
most 80 MiB, allowing a future Git recipe without an oversized blob or LFS.
Assembly compares the joined bytes to the original. Native preparation checks
the parts, whole-archive digest and detached Linux signature before extraction.
Source tests independently verify the expected signer.

Candidate release **7.3.1-1006u1** makes `sizeof("uke" UTS_RELEASE)` exactly
16, satisfying the unchanged shared ALSA assertion that rejected the old
115 Ubuntu suffix. The driver, inspector and fixtures use the same identity;
oversized future identifiers fail before upstream compilation. The Debian
version is `7.3.1-1006ubuntu1~tenacious1`.

The default recipe defines three candidate-only ARM64 packages: Image/config,
DT and modules, beneath `/usr/lib/uke-kernel-candidates/7.3.1-1006u1/`.
Exact Debian dependencies bind them. It installs no live boot/module tree,
hook, firmware, boot selection or boot ABI provider. The first-pass Image
retains INITRAMFS_FORCE with an empty embedded initramfs and is **not bootable**.
Do not deploy this intermediate Image. The separate explicit boot trial action
now invokes the complete producer after requiring a root-prepared host hwdb
receipt; it does not change the ordinary candidate-package build.

The separate [artifact-only Debian profile](ARTIFACT-PROFILE.md) now packages
the already verified complete native1006 cohort into four inert roles through
Debhelper. It retains the exact existing bytes and does not run the producer
or qualify a live boot/kernel ABI. Normal source-build integration and runtime
admission remain separate work.

The earlier server execution approval covered the prepared release-115 source
set. The new 1006 profile differs in 28 files, including LMH policy lifecycle,
SCMI negotiation, CPU observation and O82 VRR behavior. Its server-native build
received source-specific server approval and passed a clean Tenacious ARM64
build, including shared native helpers and the disposable module-signing key.
The user's private upload key remains on the tablet and is unrelated.

## Full boot cohort

The inspector validates actual content inventories, ARM64 Image and module
metadata, Uke/SM7675 DT compatibility and enforced signing configuration. Its
refusal fixtures pass. Independent inspection of the complete1006 producer
additionally verifies all 823 module signatures against the certificate embedded
in Image, both boot wrappers and the embedded payload. Runtime OEM DT/driver
and root-image admission remain separate requirements.

The immutable shared producer builds a systemd/dracut base and records Fedora
RPM ownership/licenses. The [prepared host overlay](BOOT-ADAPTER.md) replaces
that inventory with dpkg provenance and admits the bounded Debian `u1` suffix.
Metadata fixtures, static staging and native dracut execution pass.
Root-selector, filesystem, timeout, offline-charge, kernel configuration and
recovery code remain unchanged. Do not suppress those checks or run RPM scripts
against Ubuntu. The overlay does not select or change any security setting.

A complete boot cohort must be rebuilt in one job with its embedded initramfs,
DT and signed modules. Preserve the private ephemeral module key until the
final Image pass, bind its public certificate and module signatures, then
discard the private key before artifact export. The candidate-only action
deletes that key after module packaging; its Image cannot be promoted later by
appending an unrelated initramfs or regenerating a different trust key.

The shared configuration selects SELinux and lacks Ubuntu's default AppArmor
support. The optional SELinux compatibility package test passed without Python,
but that did not select the final image security profile or qualify enforcing
handoff during the original u1 trial.
On 2 November the user authorized an explicit Ubuntu-only AppArmor profile.
The [distro security contract](DISTRO-SECURITY.md) defines the separate `u2`
cohort and preserves the Fedora baseline; native and runtime results remain
separate. The older `u1` artifact cannot establish this new configuration.
Do not force Fedora assumptions or disable security to get a boot. See
[TEST-IMAGE-GATES.md](TEST-IMAGE-GATES.md).
