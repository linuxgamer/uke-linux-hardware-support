# Userdata boot A/B updates

23 October 2026. Version 1.8.1 corrects repeated DNF hooks while a verified
trial awaits reboot. The recorded boot ID, accepted running Image, candidate
digest, GPT selection and UFS boot LUN must agree; another candidate is refused.
On Uke, DNF staging from B/7.2.12-14 reached A/7.2.12-22, but acceptance was
blocked by a stale backup-header array CRC. This boot is not yet an accepted
A/B update or a physical fallback result.

Acceptance and fallback can restore that CRC only if the primary array CRC,
both header CRCs, immutable header identities and both identical entry arrays
validate. Admission normalizes a private memory copy without physical writes.
Raw observed metadata is backed up; commit uses the existing concurrency,
allowed-field and strict readback checks. Error restoration retains the original
partition selection with a valid backup CRC. Corrupt primary data, divergent
arrays, invalid headers or changed identities remain refused. Normal staging
and check commands still require two fully valid GPT copies.

23 October 2026. Version 1.8.0 adds a failed-trial fallback action. Native
Fedora Rawhide host fixtures cover both selection directions and refusal
controls. Physical A/B transitions, bootloader retry exhaustion, interrupted
metadata writes, power loss and independent recovery remain unqualified.

22 October 2026 source update: version 1.6.0 adds explicit restoration of an
unaccepted current-slot trial. This addition has passed native host fixtures;
it has not yet passed a tablet restoration or power-loss test.

22 October 2026. Version 1.3.1 source and native host fixtures, Fedora Rawhide x86-64.
This implementation has not yet passed an on-tablet A/B update or power-loss test.

The `senemos-uke-linux-kernel-mainline-singleboot` profile selects the physical
`userdata` root, using F2FS, EXT4, XFS or single-device Btrfs.
`uke-kernel-update` 1.1.0 adds a separate boot A/B path for
that profile. The existing EXT4 `linux` profile retains its opt-in `boot_b`
writer. Userdata boot A/B uses one mutable root filesystem; it does not roll
back userspace packages, user data or the entire operating system.

## Package and boot flow

The installed core, modules, DTB and boot bundle must have the same signed RPM
tuple. DNF verifies package signatures; the native writer then verifies the
installed RPM digests, complete module inventory and shared Image. Retain the
running tuple and repository signature checking. A distinct kernel release is
required. Replacing the running module directory is refused.

`/etc/senemos/userdata-update.conf` is an administrator-preserving RPM config.
It defaults to `automatic=1` and the bounded userdata VRR argument at 30 Hz.
Automatic staging requires the exact value `automatic=1` in both
this file and `/etc/senemos/kernel-update.conf`. The global default is
`automatic=0`. Missing, duplicate or malformed settings fail closed before any
state directory, slot, GPT or boot-LUN operation. A valid disabled policy returns
from the `auto` command without accepting a boot or staging a kernel. VRR arguments alter only the
checked Android v4 wrapper; the package Image remains byte-identical. This
argument does not establish physical VRR quality or cadence.

The RPM-verified manifest and unique aligned `UKE` note must agree. Its type is
`0x554b4501`. Retained F2FS images use the 72-byte v1 note with descriptor
`root-selector-v1;userdata=f2fs;slot=a,b;vrr-min=0,30`; v2 is 84 bytes with
`root-selector-v2;userdata=f2fs,ext4,xfs,btrfs;slot=a,b;vrr-min=0,30`.
Descriptors include their terminating NUL. Non-F2FS roots require v2 even with
VRR disabled. F2FS retains legacy no-note admission only when VRR is disabled.
Old executable string literals do not satisfy the note contract. The RPM exposes
`uke-kernel-update(root-filesystem-v2)` for coordinated image/kernel dependencies.

A transaction file trigger runs after all package post-transaction scripts.
It acts only on an exact Uke userdata installation. An ordinary Linux root or
an offline composition does not enter the slot writer. At graphical startup,
the packaged service first runs explicit software-health acceptance, then runs
the separately gated automatic command. A disabled automatic policy therefore
does not skip marking a proven healthy running slot successful. Acceptance
retains the physical root, running Image, signed package/module, UFS boot-LUN,
GPT identity and software-health checks. It does not select the other slot or
enable automatic staging. A failed acceptance prevents the subsequent command.
Normal acceptance and staging do not reboot the device.

Before a write, the updater requires a unique physical UFS root and 96 MiB boot
slots, the matching running Image, coherent GPT active flags and UFS boot LUN.
If ABL omits its current-slot argument, exactly one slot must contain the running
package Image. Identical or ambiguous slots are refused, rather than treating
the active slot as proof of the running slot. Paired early firmware must match;
different firmware cohorts require separate qualification. Firmware contents,
partition geometry, names, unique GUIDs, Android `misc`, persist and calibration
are not write targets.

GPT changes are serialized by original libfdisk into an anonymous sparse memory
file. Geometry, partition identities, CRCs and the allowed slot-field changes
are checked before writing the physical GPT arrays and headers. The protective
MBR is preserved byte for byte; libfdisk's PMBR normalization remains in memory.
Restoration verifies both headers, both arrays and the protective MBR.

The accepted slot and GPT metadata receive synced private backups. With at least
50% measured battery, or 30% plus external power, the inactive boot slot is
written, synced and fully read back. Only then are the QTI GPT type GUIDs and
slot flags selected, followed by the UFS boot LUN and its readback. The accepted
boot slot is retained. A metadata failure attempts verified restoration before
returning an error. Keep the private transaction backups and a fastboot route.

The durable state records staging, verification and trial separately. A boot
ID distinguishes a pending trial before reboot from a return to the old slot.
A second package transaction in the same boot does not mark the pending trial
failed. Only a matching trial release and digest with the required services
active and the internal DRM output enabled is accepted. This is a software
health check; it cannot prove visible output, touch quality, sensor accuracy or
interactive performance. Returning to the accepted slot on a later boot records
the failed candidate and refuses an automatic retry of the same release.

Internal-output admission requires a connected, enabled DSI connector. Its
numeric suffix is allocated during DRM registration and may change with probe
order; DSI-2 is not evidence of a different panel. An active external DP/HDMI
output alone does not satisfy this check.

## Failed alternating trials

The acceptance service permits five starts per boot, with a finite timeout for
each start. Its start-limit interval is infinite so long-running failures cannot
expire the counter and retry indefinitely. Only terminal failure activates
`uke-userdata-fallback.service`. Stable installations, staging/verified states,
current-slot maintenance and the pre-reboot staging boot do not trigger a
fallback reboot.

The native `fallback` action requires the booted candidate's exact slot,
release, image digest and a different boot ID. It verifies the retained accepted
image and its complete installed RPM/module bundle, coherent paired GPT active
flags and the UFS boot LUN. It then backs up metadata, selects the accepted slot
as successful and marks the failed trial unbootable without writing either boot
image. Metadata or UFS selection failures attempt verified restoration. An
already selected accepted slot is recognized for a repeated request.

Trial state remains durable until the accepted kernel actually boots. A normal
logind reboot request respects shutdown inhibitors and refuses a pending special
reboot target. Refused reboots leave the accepted slot selected and the trial
unaccepted. Subsequent healthy acceptance records the candidate as failed.
Missing or changed accepted images/modules and incoherent metadata fail closed.

This path requires a working userspace and completed startup ordering. It does
not recover initramfs stalls or hard hangs. Hardware watchdog reset, stock ABL
retry behavior and independent recovery need physical qualification. Converting
both boot slots to Linux must preserve a separately verified recovery route;
the existing kernel-less OrangeFox image inherits the selected boot stack.
Different early-firmware cohorts remain blocked unless all 42 hashes match the
single entry-tested Uke cohort. Its canonical inventory digest is
`7aa233025d51221ff142c8940d696671e18e2d53c82611683fb9610d070f47c7`.
On 23 October, stock fastboot selected A with release 21, then returned to the
retained B/release 14. Authenticated Linux reads and complete boot-image digests
confirmed both entries; firmware contents and update policies stayed unchanged.
Release 21 failed touch startup, so it was not accepted as healthy. These were
external-fastboot tests, not native/DNF A/B, retry-exhaustion or power-loss tests.
Unknown, reversed or altered cohorts fail closed. No firmware is copied.

## Validation and limits

An explicit current-slot update requires an already accepted stable B baseline.
Before writing boot B, the updater atomically records a `current-slot-trial`
with the old accepted release/hash, new candidate release/hash and staging boot
identity. It retains the old state until the matching healthy candidate boots
in B. An interrupted write restores the prior state only after the old image
is read back exactly. A later boot of the restored old image records the failed
candidate. This path cannot replace an outstanding alternating-slot trial or
select slot A. It does not provide automatic recovery for a damaged boot B.

Native state/refusal fixtures cover both trial types and repeated current-slot
transitions. Full physical current-slot update, reboot, acceptance and restored
baseline tests remain separate gates for this revision.

On 20 October, read-only tablet comparison found different early firmware in
the two slots, including XBL and ABL. Automatic A/B is therefore refused on that
installation. Firmware is not copied to make the guard pass.

Version 1.1.1 adds an explicit maintenance path:

```
sudo uke-kernel-update check-current /usr/lib/senemos/kernel-updates/RELEASE
sudo uke-kernel-update apply-current /usr/lib/senemos/kernel-updates/RELEASE
```

Both commands require the userdata profile and a proven running boot B. The
first checks without writing. The second reuses the existing full backup,
power, distinct-release, module preservation and complete write/readback checks
to update only boot B. It does not change GPT, boot-LUN attributes, firmware or
boot A. This is an explicitly requested single-slot update, not seamless A/B;
it is not power-loss atomic. The automatic path never silently selects it.
Keep the backup and a working external fastboot recovery route before reboot.

Native fixtures cover 512/4096-byte GPT sectors, both selection directions,
unchanged geometry and unrelated flags, CRC damage, concurrent changes, torn
writes and metadata restoration. Mocked UFS ioctls cover read/write/readback,
transport errors and malformed responses. Durable regular-file fixtures cover
phase transitions, stale writes, hardlinks, symlinks and ambiguous current-slot
identity. GCC warning-as-error and Clang AddressSanitizer/UndefinedBehaviorSanitizer
checks are host evidence, not tablet qualification.

QTI retries depend on the stock boot chain. A panic timeout can permit another
attempt; it cannot recover every hard hang. GPT updates on multiple UFS LUNs and
the UFS boot-LUN attribute are not one atomic hardware transaction. Abrupt power
loss between them can require external recovery. Actual repeated upgrades,
fallback, interruption and recovery tests remain required before a release
claim. A successful DNF transaction does not prove that a boot write occurred.

## Restore an unaccepted current-slot trial

For a booted current-slot trial that should not be accepted as healthy, use its
already installed, recorded accepted kernel bundle:

```sh
uke-kernel-update check-restore-current /usr/lib/senemos/kernel-updates/ACCEPTED_RELEASE
uke-kernel-update restore-current /usr/lib/senemos/kernel-updates/ACCEPTED_RELEASE
```

The first command checks admission without a partition write. The second writes
only physical boot B, using the same complete backup, power check, sync, readback
and write-error restoration as `apply-current`. It requires the currently running
release and boot B hash to match the recorded candidate, and the target release
and image hash to match the accepted B baseline. The trial must have booted since
staging. Other phases, slots, releases and images are refused.

Restoration leaves the trial state unchanged and does not mark the candidate
healthy. After an explicit reboot into the accepted kernel, normal acceptance
records the returned trial as failed. The command does not reboot, select a slot,
write boot A or restore shared userdata packages. Keep matching modules, the
complete backup and external recovery available. A single-slot write remains
non-atomic under power loss. Host tests do not qualify physical recovery.

## Source comparison

- Uke's Lineage common-device configuration `f477733d60197b312ab2ed42f6b75dfade54ada2`
  selects the QTI boot service. AOSP `misc` boot-control metadata is not assumed.
- QTI boot-control `25092672e644459db328b43fd1c7c57cbff65c91` defines GPT slot
  attributes and UFS boot-LUN control.
- OpenABL `aef4e86045bd151a54d33bab8ae5e8e62462a8f4`,
  `QcomModulePkg/Library/BootLib/PartitionTableUpdate.c`, supplies the compared
  `SetActiveSlot`, `SwitchPtnSlots`, `MarkPtnActive` and retry/success semantics.
  These are donor-source findings, not proof of every installed firmware build.
- Original util-linux libfdisk writes and verifies GPT metadata. Upstream
  `v2.42.4` is pinned at `d76cbf8f13e65ff657344f7f6a90042cf755ba59`.
  The implementation does not execute donor boot-control scripts or qbootctl.
- [RPM transaction ordering](https://rpm.org/docs/latest/manual/file_triggers.html)
  places transaction file triggers after package post-transaction scripts.

Private lessons retain the comparison details and failed trials. No device
identifiers, raw GPT backups, credentials or calibration are public payloads.
