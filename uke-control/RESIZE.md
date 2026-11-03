# Expand the installed F2FS userdata filesystem

23 October 2026. Uke Control 1.5.1 provides an explicit capacity-repair command for
the provisioned Fedora F2FS root on the existing `userdata` partition:

```sh
sudo uke-control resize system --check
sudo uke-control resize system
# Type yes when asked to prepare maintenance and reboot now.
# After boot:
sudo uke-control resize system --check
df -h /
```

The check prints the filesystem and partition sizes without scheduling a write
or reboot. When growth is required, it prints the command to run next; this is
an informational result, not a preparation failure. A real refusal starts with
`UKE_RESIZE_SYSTEM_REFUSED` and includes its reason. Failed tool output appears
before that line and must be retained when reporting a problem.
If the filesystem already fills the partition within F2FS section alignment,
both forms report that result and exit without preparing maintenance. F2FS
metadata and reserved space mean usable capacity is smaller than raw capacity.

When growth is needed, the normal command checks power and asks in an interactive
terminal whether to prepare expansion and reboot now. Only the exact answer
`yes` proceeds. Other answers or end of input cancel before new preparation;
piped input is refused. A filesystem already at full capacity exits without
asking or rebooting. There is no unattended reboot option.

After approval, the command prepares a one-use Fedora dracut shutdown environment
in executable tmpfs at `/run/initramfs`, protected by a shutdown/sleep inhibitor.
It waits for that inhibitor to exit, verifies the prepared request and power
again, then requests normal reboot while respecting other inhibitors. Preparation
failure prevents a reboot request. A rejected reboot is not forced; its RAM
maintenance remains pending for ten minutes after preparation. Resolve the
blocker and rerun the command to renew preparation and confirm reboot again.
A forced reset or loss of power bypasses normal shutdown; it does not run this
maintenance. The RAM preparation disappears at reboot.

Original dracut shutdown first unmounts the old root. Only then does the native
helper check exclusive access to the exact userdata block device, run original Fedora
`fsck.f2fs`, expand with `resize.f2fs`, and run the checker again. There is one
attempt and no automatic retry. Filesystem operations are not killed by an
application timeout. Both superblocks must show full-partition growth with the
same UUID and provisioned label before success is printed to the shutdown
console. Verify actual capacity after boot; preparing maintenance is not a
completed resize. A refusal leaves capacity unchanged, except that an already
started filesystem checker may have repaired filesystem metadata.

The operation changes filesystem metadata. Keep a backup and a working external
recovery route, and do not interrupt power during checking or growth. Physical
Uke shutdown/growth and power-interruption recovery require their own tests.
The command does not format, convert, shrink, alter GPT, write boot/firmware
partitions or change the selected boot slot. It expands only to the capacity
of the current userdata partition; it does not reclaim space belonging to
other partitions.

Admission requires the exact Uke/SM7675 device, the system mount namespace, a
full writable F2FS root on the physical Uke UFS userdata partition, consistent
4 KiB F2FS geometry, `UKE_LINUX` label, nonzero UUID, no device holders and a
writable block device. LUKS/device-mapper, loop, multi-device, subvolume and
other filesystem/root layouts are refused. Preparation requires battery of
at least 50%, or at least 30% with external power. The shutdown request binds
the device number, UUID, capacity, old size and boot identity; stale or changed
requests are refused before filesystem tools run. Exclusive block access also
rejects a device mounted outside the helper's namespace.

Existing restored/custom shutdown environments are preserved and refused.
Preparation uses a shutdown/sleep inhibitor; failed RAM activation restores
the previous directory. Its earlier logs/state remain in the private RAM
staging directory until reboot. The generated payload is checked for required
tools, account-secret files, a 128 MiB size ceiling and Python
payloads. SELinux systems use their original active policy type and `setfiles`
before activation. Distribution dracut, f2fs-tools and policycoreutils are
runtime dependencies (dracut 111 or newer and f2fs-tools 1.16.0 or newer); no
project Python code or Python runtime is introduced.

Original dracut creates the standard `dev/null`, `random`, `urandom`, `kmsg`
and `console` character nodes when the build has permission to do so. These
five root-owned, single-link nodes are admitted only with their exact Linux
major/minor numbers. Block devices and all other special files are refused.
This corrects 1.4.0/1.5.0's overly strict audit on a native privileged build;
the earlier host container lacked those nodes and did not expose the refusal.

The generic geometry reader in `shared/userdata-grow.hpp` is copied unchanged
from this project's MIT-licensed kernel recipe,
`src/boot/userdata-grow.hpp` at
`6847ccabffdb9de3815e4754a9075eb61659f1fd`, SHA-256
`83fc0c69059dd818a2c8c9047ab4192a0514fccdd2d7bf04e7a427bfe8ea5efd`.
The kernel recipe's `uke-userdata-grow.service` already orders automatic growth
after `uke-root-guard.service` and before `sysroot.mount` in a capable initrd.
That early-boot path operates on the unmounted selected userdata root. Older
installed boot images may lack it. This confirmed-reboot command supplies their
repair path during shutdown before the next boot, without rebuilding or replacing
the boot image. Installation order alone does not prove why a given device
missed early growth; its running boot image and logs remain necessary.

Source, host image, VM, package and tablet results are separate. Host fixtures
cover geometry/request checks, mount refusal, private files, environment
admission and activation rollback. A populated 512-to-768 MiB host image grew
through the actual tool pipeline and retained its file hash and filesystem
identity. These results do not establish physical tablet acceptance.
