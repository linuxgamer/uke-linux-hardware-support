# Ubuntu complete kernel packaging

2 November 2026. The normal Debian producer now builds one complete AppArmor
cohort, using the same source preparation and two-pass embedded-initramfs
producer already validated natively. It stages four version-matched packages:
`uke-kernel-core`, `uke-kernel-dtbs`, `uke-kernel-modules`, `uke-kernel-boot`.
The core depends on the exact DT/module versions; boot depends on all three.
The boot package retains both wrappers and records that a singleboot userdata
root pairs with `userdata.img`, filesystem label `UKE_LINUX` and physical
partition label `userdata`. DT storage does not apply a replacement runtime DT.

The default build requires a native ARM64 build environment and the existing
root-prepared hwdb provenance receipt. It no longer substitutes the nonbootable
first-pass Image for a complete boot cohort. The complete producer passed in
server run 41390762590 before this packaging integration; a new default source
package build has not been performed. Source checks and artifact-package
readback must not be described as a repeated source compilation.

For private image qualification, the explicit build profile
`pkg.uke-linux-kernel.verified-cohort` imports the already independently checked
1006u2 bytes. Its immutable receipt is
`verified-artifacts/1006-apparmor.json`. It verifies source/adapter/run identity,
every selected input and complete notice/alias inventories before snapshotting,
then repeats verification and module/signature/initramfs checks on the snapshot.
Its `build_origin=verified-artifact` record distinguishes it from a source build.
It invokes no upstream compiler, preparer, key generator or boot producer.
Combining the historical u1 and current u2 import profiles is rejected.

All four DEBs undergo complete payload byte, mode, control and exact dependency
readback. They have no maintainer hooks, automatic boot selection or runtime
hardware capability providers. Only the matched module package uses a normal
module directory; installing it does not load a module or activate a kernel.
Normal package construction is progress toward image integration, not proof of
hardware APIs, OEM DT handoff, AppArmor runtime enforcement or redistribution
clearance. These remaining gates are independent of package solvability.
