# Package channels

30 October 2026: a failed native transaction reports its RPM, signature and
transaction diagnostics while retaining the pending channel. Correct the
reported problem before running `channel sync`. Failure before RPM execution
may have no DNF history entry.

30 October 2026: Fedora 46 Rawhide reports `releasever=rawhide` to DNF.
The RPM dist check uses the numeric `VERSION_ID` from os-release in this case.
Numeric DNF and os-release versions must agree; an unresolved numeric version
still blocks the transaction.

29 October 2026: unstable uses the separate `mcc45tr/uke-linux-unstable`
repository, available only on Fedora Rawhide AArch64. Its signing key is pinned
independently. With `automatic=0`, package synchronization installs the matched
kernel bundle and reports `kernel_activation_pending` without writing a boot
slot or rebooting. Activation remains a separate guarded operation.

Switching to unstable clears earlier channel RC exclusions while retaining
unrelated custom exclusions. Stable and test retain their non-prerelease policy.

29 October 2026: source routing selects Fedora 44, Fedora 45 or Rawhide from the
installed os-release data. Channel changes stay in that release's COPR chroot;
a distribution upgrade is a separate operation. Candidate RPM dist tags must
match DNF's installed Fedora release. Signatures, vendor restrictions, private
touch-data exclusions and exact kernel guards remain required. Native Fedora 44
x86_64 compilation is separate from AArch64 RPM and tablet acceptance.


21 October 2026 vendor correction: planned replacements use an exact package
NEVRA with its selected repository and vendor as per-job constraints. This
permits declared renames under `allow_vendor_change=False` without changing
that global setting or allowing unrelated vendor transitions. libdnf5 5.4.6
applies these exceptions through spec jobs; its explicit package-ID jobs ignore
the vendor setting. The resolved transaction must still contain each exact
replacement and may not remove packages or alter protected identities.

Reviewed implementation: DNF5 5.4.6.0, commit
`1b2de4bdc2f25a6de2884fb2cf81f80a8ce2a567`,
[`add_rpms_to_goal`, `add_install_to_goal` and `add_reinstall_to_goal`](https://github.com/rpm-software-management/dnf5/blob/1b2de4bdc2f25a6de2884fb2cf81f80a8ce2a567/libdnf5/base/goal.cpp).
Signed host fixtures run with global vendor changes disabled; downgrade,
same-NEVRA replacement, combined update/rename and original Fastfetch return
pass, while an unrelated foreign-vendor candidate remains unchanged. The old
package-ID path reproduces the vendor refusal. These fixtures do not activate
a tablet kernel or establish physical behavior.

21 October 2026 private-data correction: `uke-local-touch-data` is a retained
device-private package, outside public channel synchronization. Its absence from
COPR does not prevent sync/update. Public candidates with that exact name are
excluded; any indirect transaction that replaces or removes it is refused.
The exception does not cover other `uke-local-*` names or obsolete generic
firmware providers. Calibration is never uploaded or substituted from COPR.

29 October 2026: the exact `uke-video-firmware-local` package is also retained
outside public synchronization. Its locally extracted VPU firmware has no
reviewed redistribution license. Public candidates cannot replace or remove it;
other similarly named packages receive no exemption.

21 October 2026 correction: without a saved selection, status and implicit
operations use exactly one enabled normal stable/test/unstable COPR. They do not silently
select stable over an enabled test repository. Missing, multiple or custom-only
repositories require an explicit selection. Unreadable, empty, malformed,
symlink or hardlinked channel records are refused. Status reads no package
metadata from the network; private pending state remains root-only.

18 October 2026. Native C++ implementation against libdnf5 5.4.6 or later,
initially scoped to Fedora Rawhide AArch64 on Uke. Host fixture results and
tablet acceptance are separate. The original Fedora repositories and unrelated
third-party repositories retain their configuration.

```sh
uke-control channel status
sudo uke-control channel set stable --dry-run
sudo uke-control channel set stable
sudo uke-control channel set test
sudo uke-control channel set unstable
sudo uke-control channel sync
sudo uke-control update --check
sudo uke-control update
```

`branch` and `brunch` remain compatibility aliases for `channel`; use `channel set NAME` in new commands. Mutations show the
resolved transaction and ask for confirmation; `--yes` accepts that transaction.
`--dry-run`, also spelled `--check` for `update`, resolves without RPM, channel,
repository or boot changes. It may refresh metadata and requires root for the
system package lock. It returns zero for a successful preview, including one
containing updates; it is not DNF's check-upgrade exit-code interface.

| Selection | Enabled Uke COPR | Kernel preference |
|---|---|---|
| stable | mcc45tr/uke-linux | Latest non-prerelease Uke kernel in that repository |
| test | mcc45tr/uke-linux-test | Latest non-prerelease Uke kernel in that repository |
| unstable | mcc45tr/uke-linux-unstable | Rawhide AArch64 only; latest available Uke RC, otherwise its non-prerelease kernel |

The requested unstable selection is retained when no RC exists. A later sync
or update checks again. An RC older than or equal to the newest final package
is superseded and does not prevent that fallback. RC versions must be valid RPM versions, for example
`7.4~rc7` in the `senemos-uke-linux-kernel-mainline` family, with matching
core/modules/DTB/boot packages. A new package-family name requires a reviewed
updater/selector extension. This command does not
download an arbitrary upstream kernel: the package must carry Uke integration
and pass the existing kernel updater. Prerelease versions are excluded from
ordinary DNF selection in stable/test. Package selection and the running kernel
are reported separately; no automatic reboot occurs.

Synchronization covers installed packages also offered by the selected COPR,
packages with a recorded Uke COPR origin, and project package names `uke-*` and
`senemos-uke-*`. It does not install every optional package from a repository.
The native solver upgrades or downgrades those packages to the selected exact
NEVRA. A matching version from another repository or a local RPM is reinstalled
from the selected repository, so matching version labels cannot hide a different
payload. An installed project package missing from the target blocks the whole
transition. Promote its matching package or resolve the obsolete component
explicitly; the tool does not silently delete it or its dependants.
The target `uke-control` must provide `uke-control(channel-api) = 1`; otherwise
the transition is refused before changes. Promote a compatible manager into
the destination first. Downgrading to a manager without these commands would
remove the return path even if the RPM transaction itself succeeded.

20 October 2026: renamed child packages are accepted only through paired
Provides/Obsoletes in the selected signed repository. Different replacement
names are ambiguous and fail. The solver must replace the old package and
install the declared new package together; a virtual Provide alone is
insufficient. Kernel packages, including the old KVM namespace, remain retained.
The normal-kernel selector does not infer a normal kernel from an isolated KVM
core. Source families and child names are defined in
[`packaging/families`](../packaging/families/README.md).

The withdrawn Fastfetch fork is a deliberate exception: the exact replacement
must come from Fedora's `rawhide` repository, including a necessary downgrade.
No other project package is handed to an arbitrary outside repository. If the
original replacement is unavailable, the whole transaction is refused.

`sync` repairs the saved selection; without a saved selection it infers exactly
one enabled stable/test COPR and refuses missing or ambiguous selections.
`update` performs the same synchronization and upgrades the other
distribution packages in the same dependency transaction. Dependency failures,
version locks, missing candidates, duplicate repository definitions and
unintended package removal are errors. There is no `nodeps`, signature bypass,
skip-broken, skip-unavailable or automatic dependency-erasure fallback.

The active Uke COPR has priority 1 and package signature/TLS checks enabled.
Existing Uke aliases and custom COPR directories are disabled. Other repositories
continue to supply dependencies. Canonical missing definitions are added to
`/etc/yum.repos.d/uke-control-channels.repo`; selections and security settings
are stored in `/etc/dnf/repos.override.d/zz-uke-control.repo`. Both definitions
may remain visible in `dnf repo list --all`; only the selected Uke channel is
enabled for normal DNF use. Files are retained for recovery rather than deleted.
Manual changes made by a root administrator can override policy; `channel status`
shows the saved selection and configured repository enablement without network
access. Synchronization checks the persisted repository policy before running RPM.

New COPR key imports accept only these reviewed fingerprints:

- stable: `30DFB81A724CE0762F9DF77F844A293E5068FAFE`
- test: `DAFC3C5A881FB49C7167EE2D6F772E3D487BD13E`
- unstable: `2204A661EA630A5303500085538797E72A3E4ECA`

A changed key requires review and an updated application; it is not silently
trusted. The selected repository's canonical HTTPS metadata/key URLs are used.
Package signatures are checked before repository policy is changed.

All installed kernel tuples are retained. Same-NEVRA kernels are not reinstalled
to change their origin. The manager explicitly retains normal kernel selection
RPMs as installonly packages as well as the payloads carrying that capability;
other configured installonly selectors remain in force. This also preserves an
older singleboot profile's exact dependency without replacing its selection RPM.
A candidate with a different RPM identity cannot
reuse an installed `kernel-uname-r` module directory. This is a deliberate
exception to ordinary-package replacement: the running and recovery modules
must survive channel changes. The selected core, modules, DTBs and boot bundle
must match exactly. On 30 October 2026, Control 2.3.2 routes successful package
transactions through updater 1.8.7's `transaction` interface. The updater reads
both automatic policies and the trusted selected userdata profile, retains a
pending trial and verifies an idempotent match with any RPM hook. Installing a
newer kernel leaves an older selected userdata profile in force. The command
does not enable automatic updates or select another default. The
`kernel_activation_pending` field remains true while the package-selected
kernel differs from the running kernel, including after staging. It does not
establish that the selected boot profile changed. Updater failure keeps the channel pending and
reports that the tablet must not reboot. Stock firmware, Android A, storage
layout, persist policy and the updater's battery/backup checks are not bypassed.

Downloads and signatures precede persistent channel changes. Root-private
`channel-pending`, versioned `channel-operation`, `channel-transaction.json` and
`channel-repositories.before` records under `/var/lib/uke-control` describe
interrupted work. The saved successful channel is committed only after the RPM
transaction and required boot selection succeed. If an operation is interrupted,
`channel sync` can replan only before RPM starts. Once RPM is durably complete,
it resumes the recorded original kernel's boot/selection finalization without
replaying RPM or resolving a new transaction. An unknown RPM outcome refuses
automatic retry and requires DNF history/boot-state inspection. Root channel
status reports operation IDs and phases; no userspace rollback is promised.
An interrupted `.new` file is refused for inspection. RPM/scriptlet failure can
leave some packages installed; read DNF history and the retained record. Do not
reboot an incomplete kernel transition.

Build admission uses the original native libdnf5 and librpm. No shell command,
DNF4/Python backend, donor installer or project Python payload is invoked.
`make check` covers parser/refusal contracts. `make check-repositories`, run as
root in a disposable host container with rpm-sign, GnuPG and createrepo_c,
exercises signed local repositories and an isolated RPM database. None of these
checks establishes a tablet channel switch, kernel boot or rollback result.

30 October 2026: preview includes download/RPM payload totals and filesystem
bytes/inodes. Staging peak remains unknown; native RPM space checks remain
required. No kernel cleanup occurs without complete running/selected/recovery,
module, firmware and boot reference ownership. The boot helper must be trusted
before mutation or supplied by the planned helper package. A common maintenance
reservation excludes prepared layout/resize. Catchable termination is deferred
during mutation; SIGKILL/power loss can still leave uncertain RPM completion.
