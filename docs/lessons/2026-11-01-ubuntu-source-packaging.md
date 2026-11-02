# UKE-UBUNTU-20261101: separate source packaging from device admission

## Main source convergence checkpoint

1 November 2026. The local integration merges current Fedora hardware history
and Ubuntu packaging without discarding either parent. The first replacement
recipe selects canonical firmware inputs and common metadata from main using
the existing docs-only ancestor as its fixed base. The raw preset and original
notice retain their already published hashes. A shared host auditor now serves
both the Fedora source target and Debian source construction; no runtime or
hardware policy changes are introduced. Generated snapshot metadata is checked
against the common release overlay, and mixed main revisions or changed preset
bytes are refused. Native replacement and publication qualification remain
pending. Keep the existing Launchpad recipe until that transition is verified.

The snapshot 3.0 native version encodes the Ubuntu integration revision in the
upstream portion, so it outranks conventional Debian revisions at the same
vendor version. Stable promotion must preserve monotonic version ordering.
This is source/metadata preparation, not device acceptance or image readiness.

Date: 1 November 2026. Environment: Debian 13 x86_64; dpkg-source 1.22.22.

Evidence: the public mcc45tr/uke-linux-unstable COPR listing contains 43 source
entries, including superseded standalone recipes. Common source commit
cbecc441059ebffa97497e736edd2e0b0b0546a5 has no existing Debian packaging.
The Control RPM explicitly requires libdnf5, RPM, dracut and SELinux integration;
translating dependency names cannot port its transaction implementation.

Resolution: add Debian overlays without changing shared C++ or Fedora specs.
Unsigned battery-observer and front-color source packages assemble/extract with
checksums and no install-time systemd activation. The latter still depends on
an unported SSC service. All other source routes retain explicit coverage states.
Tests: packaging/ubuntu/tests/source.sh passes on the stated host; no upstream
binary was built or executed, and no physical acceptance is inferred.

Uncertainty: target dependency resolution, ARM64 build/install behavior, exact
kernel/DTB/initramfs cohort admission and current firmware redistribution rights.
Next validation: approved Tenacious ARM64 builds and service lifecycle tests;
port APT/dpkg transaction integration and boot cohorts before publication.
Parent subsequently confirmed PPA terms accepted and ARM64-only PPAs created;
source-signing key enrollment and signed upload remain pending.

## Subsequent source-port implementation

Expanded to fourteen Debian source routes, including native service policy,
UCM/KMix/module data, an admission-gated core meta selection, SSC, libssc,
SensorProxy and the VA backend. Verified the official Tenacious ARM64 package
indexes: distribution PipeWire/KDE/DDC versions do not yet satisfy all shared
requirements. This is now a concrete dependency record, not an assumed blocker.
Four data-only ARM64 DEBs build with official debhelper 14.5ubuntu1 on Debian
13 x86_64; exact UCM/link, launcher, module-policy and core-profile payload
checks pass. Build-dependency resolution was intentionally not claimed.

The upstream VA archive contained its own Debian directory. The initial source
assembly caught the differing libva-v4l2 source name. Reviewed its control,
rules and copyright, retained that directory as source evidence, and corrected
overlay placement. Its six shared patches now apply through quilt. Across SSC,
libssc, SensorProxy and VA, all 21 shared patches apply on extraction. No
upstream compiler, executable, generator or native contract was run.

Added metadata-only boot cohort checks and explicit negative fixtures. The
first fixture creation exposed inherited private directory modes; fixtures now
set umask 022 before dpkg-deb. Foreign series/architecture/device, mismatched
versions, promotion, automatic boot selection, traversal and DT corruption are
rejected. These tests do not inspect real kernel/DT/initramfs contents and must
not be treated as boot admission.

Baseline-to-current COPR family reconciliation remains unfinished; the current
core selection deliberately retains the >=1.4.1 battery requirement although
the initial component baseline recipe is 1.2.0. Source-only ports are not
qualified replacements for later family releases. The missing tracked THP
source also remains an implementation/provenance gap. Signing is independent.

## Superseding touchscreen source-gap finding

The missing tracked THP directory is generated, not an unavailable donor.
Reviewed `touch-thp-source.json` and `prepare-touch-core.sh`; recovered the two
source files and Apache license at upstream commit
34046210932d654a4c0df0121ecc31c008f8148c. Input hashes and both transformed
output hashes match the shared recipe after its four static patches. This
supersedes the earlier missing-source blocker. Added a Debian touchscreen
source recipe without private calibration or donor execution. Build/runtime and
current-family reconciliation are still pending.

Expanded the power baseline's source recipe to retain separate charge-control
and offline-charge binaries, their disabled/gated activation, and the exact
UPower/kernel/boot ABI prerequisites. Added UPower 1.91.5 with all five shared
patches, original Ubuntu binary names, Polkit data and standard service lifecycle.
All sixteen source routes pass extraction checks. The five external archives
use 26 quilt patches; the THP source importer applies four additional patches.

GitHub commit 85ced4b passed both Ubuntu source packaging contracts (run
41300695468) and Hardware source contracts (run 41300695434). These results
cover the fourteen-source snapshot; subsequent sources need the next CI run.

## Workspace branch validation

Root Ubuntu CI 41299730079 failed at configured branch validation with exit 128.
A fresh shallow hardware-support clone plus fetch of the pinned packaging SHA
reproduced the missing `origin/ubuntu` reference. Fetching only the configured
branch ref repairs validation without moving the pinned checkout. Root commit
33e237d retains all existing tests; run 41301170081 passed the workspace job.
This is branch-awareness, not a package or hardware qualification result.

## Release source reconciliation: battery protection inventory

Recorded 1 November 2026 on Debian 13 x86_64. Lesson ID:
`ubuntu-source-release-identity-20261101`. The current Control family reuses
the released battery 1.4.1 source RPM, whereas the common Git component is
older. Its `control-profile.cpp` requires an additional
`integrated_protection_inventory` digest; readiness metadata also demands
inventory review and leaves physical acceptance false. Pin both the source
RPM and its nested source archive and reuse that source verbatim in Debian.
Never advance a version label without reconciling these safety differences.
All 16 source assembly/extraction checks pass; no upstream code was compiled
or executed. Battery ARM64 build, runtime and physical qualification remain
pending. Front-color's 1.0.2 wrapper contains 0.1.0 source with five source
files identical to the common baseline; wrapper and application versions
must be reported separately.

## SSC release, recovery delivery and APT policy

Recorded 1 November 2026, Debian 13 x86_64. Lesson IDs:
`ubuntu-ssc-release26`, `ubuntu-recovery-immutable-zip`, `ubuntu-apt-policy`.
SSC release 26 adds verified OEM-reference selection with a packaged fallback
in its cache source. The six FastRPC patches and five systemd/udev files match
the common source, but the cache and its test must come from the pinned release
RPM. Member hashes are recorded in `release-overrides.tsv`. No cache program
or upstream test was executed.

Recovery's existing Fedora package delivers Android v003 artifacts as data.
The Debian equivalent preserves this scope and has no maintainer hooks. The
first payload check failed because `dh_strip_nondeterminism` rewrote its ZIP.
An explicit override preserves the published bytes; extracted payload checksum
tests must cover all release files, not only the IMG. This is packaging work,
not recovery recompilation, boot validation or a new Ubuntu image.

New project-owned APT policy fixture tests pass using official g++ without
running any package command. Managed source parsing, interrupted dpkg state,
holds and boot/private exclusions are implemented. They deliberately provide
no transaction authorization: authenticated metadata, solver, operation journal
and boot finalization remain implementation work, documented in APT-BACKEND.md.

## Kernel composition, attributed preset and dependency closure

Recorded 1 November 2026, Debian 13 x86_64. Lesson IDs:
`ubuntu-kernel-retained-source`, `ubuntu-firmware-file-attribution`,
`ubuntu-isolated-build-closure`.

The current kernel producer at 8983163763f708ec940b349ebb81ab43cc3725c3 composes
release 115 from source RPM build 11239296 plus nine exact overlays. A Git-only
archive would lose retained release-110 sources. The Debian source preparation
now verifies the parent RPM hash, Linux archive hash, retained-file inventory,
overlay identities and boot source assets without running the producer. Linux
signature verification initially stopped because this host lacked gpgv; the
official Debian package was checksum-verified and extracted locally. All 20
source routes then passed assembly/extraction checks, including the signature.
No native kernel preparer or compiled source program ran.

Kernel packaging is deliberately candidate-only. The first-pass Image retains
INITRAMFS_FORCE but has no embedded userspace, so it cannot be deployed. Three
packages use an isolated candidate path and distinct Ubuntu kernel release;
boot production explicitly fails. Actual payload inspection checks Image/DT
content, complete inventories and module ELF/vermagic/signature metadata, with
synthetic refusal fixtures only. Module cryptographic trust and real payload
qualification remain untested. Future native builds retain the shared enforced
module-signing configuration and its disposable key-generation behavior; no
key was generated during this work.

The complete firmware review is incomplete for 210 of 217 files. The separate
4,828-byte FS19xx preset has an exact original ODM notice mapping to Apache-2.0.
Source and data-package tests verify preset, compressed notice, content ID,
license identity and conffile preservation. No unrelated firmware or private
calibration is included. Six data-only ARM64 package builds pass; this is not
hardware playback or protection qualification. The optional OEM helper remains
unported because its unconditional SELinux mount context needs shared-source
Ubuntu portability without weakening read-only/ownership/extent guards.

Official APT 3.0.3 with isolated configuration and empty status resolves 19 of
20 Build-Depends expressions against the saved Tenacious ARM64 indexes.
SensorProxy requires the unpublished patched libssc-dev version and correctly
fails. An initial attempt combined --no-download with --simulate satisfy and
failed on uncached archives before meaningful resolution; the corrected path
uses explicit simulation in both configuration and arguments. It downloaded
and installed nothing, and did not run hooks. This proves metadata solvability,
not archive authentication, native compilation or runtime closure. Retain
dependency-first libssc publication rather than weakening SensorProxy's gate.

### Native server build admission, 1 November 2026

The prepared source set was approved for clean server ARM64 compilation,
including upstream helpers and disposable kernel module-signing keys. The
private upload key remains on the tablet. GitHub's native ubuntu-24.04-arm
runner is suitable; its host release is distinct from the build root.
The official ubuntu:devel ARM64 manifest at digest 73b76549166d2d32444ccbd2b79b34381a1b3e25fd150cdd7fb12383fe7ebdc2
still reports Stonking 26.10, and ubuntu:tenacious is absent. Bootstrap
Tenacious from the signed ports archive instead and assert version plus
architecture inside the disposable container. Retain logs and package outputs,
never entire build trees that may contain transient module private keys.
Compilation and hardware qualification remain separate evidence.

### Native build and first recipe results, 1 November 2026

Run 41308345325 compiled the shared sources in clean Tenacious ARM64 server
roots, including patched libssc and SensorProxy in dependency order. The
first eight downloaded route artifacts produced ten DEBs; their full artifact
checksums and direct payload audits passed, including ELF64 AArch64 identity
and absence of Python payloads or direct Python runtime dependencies.
Image tools later passed compilation without composing a system image.
Recovery delivery passed packaging of the existing Android artifacts only.
These findings do not qualify hardware or transitive runtime closure.

The first UPower build compiled successfully, then dh_missing rejected four
unassigned API/documentation files. Retain the D-Bus interface XML in the
development package and move the configuration-directory README into daemon
documentation; do not suppress dh_missing. The corrected build is retried.

Firmware validation run 41308699685 passed native DEB build, payload checks,
install, reinstall preserving a locally changed conffile, remove preserving
that file, purge and git-build-recipe 0.3.7build1 source construction.
dpkg-source creates .pc bookkeeping even without patches; exclude that
generated state from the recipe export. An upstream Git tag lets the official
recipe tool construct its original archive without changing quilt format.
The published launchpad/uke-speaker-firmware branch and upstream/3.0.304.0 tag
preserve shared source bytes. Launchpad import and recipe creation remain
pending in the parent UI session, not blocked on a server private GPG key.

UPower retry 41309136146 passed after the install-list correction, including
all four binary packages and direct payload audits. The initial matrix passed
18 of 20 routes; this retry brings qualified server construction to 19 of 20.
Six routes are data-only and thirteen compile native payloads. Kernel attempt
124853346238 stopped when the unchanged shared preparer required
senemos-applied-patches.txt, which the Debian driver had omitted. Record only
successfully applied patches, compare the full ledger against the verified
series, and retain all shared tests. Retry 41309327976 remains active; no
kernel binary or boot result is inferred from its preparer fixture tests.

### Paused checkpoint after terminal kernel retry

Kernel run 41309327976 terminated on 1 November with a real compiler failure
in sound/soc/qcom/uke-audio-card.c:411: the shared assertion limits
sizeof("uke" UTS_RELEASE) to 16, while the Debian kernel release string
7.3.1-115-uke-ubuntu1 is too long. Patch-ledger preparation passed. The next
implementation action is a shorter coordinated Debian release identity with
matching candidate manifests and inspectors, retaining the shared assertion;
no fix or new dispatch was made after the user's Fedora Rawhide DE priority
change. Nineteen non-kernel routes remain native-build-passed. All nineteen
recipe source branches are published with exact-original pristine-tar metadata.
The two-package colliding-version regression passes. Runtime metadata simulation
of all 27 actual DEBs resolves 16 closures and refuses 11 missing real ABI
providers; two resolved development closures include host Python and remain
server-only. Launchpad recipebuild 4108742 is currently building source; it is
not evidence of an ARM64 binary publication. New Ubuntu work is paused and
existing remote builds are monitored read-only.

### Resumed experimental kernel and minimal userspace preparation

The new user request authorizes preparing an Ubuntu test image, with no tablet
writes. Main unstable still publishes 115. Release1006 was found in the separate
uke-linux-test:custom:power-20261031 channel, successful COPR build 11240463,
source commit fcbdae4b757732f0f0e3162c610f19abae290832. Preserve its exact emitted
retained1003-plus-LMH composition rather than treating the current main Git
checkout or a numerically newer failed trial as equivalent. The source RPM
SHA256 is 70f151855f727c9d83c3bedc35ff52dd60cb222b2d741818472dced458bcd60c.
Its profile differs from the previously approved 115 in 28 files, including
LMH, SCMI/CPU observation and O82 VRR behavior. No new source helper executed.

The Debian recipe now stages that exact source, preserves original inventories
without recomputing away mismatches, and uses 7.3.1-1006u1. Its shared ALSA
identifier including prefix and NUL is exactly16 bytes. The recipe, candidate
inspector and fixtures agree; source checks reject oversized identities early.
Signed Linux source is represented as80MiB bounded chunks for Git service
compatibility. Their joined bytes retain the exact upstream digest and detached
signature. Source assembly, emitted inventories, full source checksums, signer
identity and candidate refusal fixtures passed. No source build or boot passed.

Boot inspection found two concrete port tasks: the shared producer accepts
numeric Fedora release arguments only, and its userspace SBOM uses RPM ownership
and licenses. Preserve root-selector, module, recovery and privacy checks while
adding reviewed Debian integration. Do not promote the empty-initramfs candidate
Image. The full cohort must retain one module key across both Image passes,
then remove its private key before export; the existing candidate-only action
deletes it after module packaging and cannot safely be promoted later.

The shared kernel selects SELinux and lacks AppArmor. Official Tenacious
dracut-core112-7 and SELinux packages3.11 exist. APT simulation of policycoreutils,
selinux-utils and selinux-policy-default resolves without Python. A disposable
native ARM64 userspace workflow installs only official runtime packages and the
exact published PPA preset, verifies the existing archive key, checks Python
exclusion and records policy/payload evidence. It emits no tablet image or boot
bundle. Policy labeling/enforcing handoff and real hardware boot remain separate
unimplemented checks, not signing or credential blockers.

### Host adapter and userspace qualification checkpoint

Native ARM64 userspace run41319778138 passed authenticated official Ubuntu/PPA
installation, preset identity and Python exclusion. First trials found dormant
Apport hooks and libstdc++ GDB helpers, including files initially extracted by
debootstrap before dpkg filters. The minimal target policy now records and
omits only those identified optional diagnostic files, preserving distro ELF
libraries and retaining a terminal audit for other scripts. No root image was
exported, and no MAC/enforcement configuration was selected.

The new host overlay stages against exact boot lock
dc2f9a7f0ded526e191147402403b9f66998004831cdec85d7756070cb1d4df4.
Original source assets are checked before copying. Three host-integration
changes add the bounded Debian suffix, dpkg copyright/source inventory and a
distro-neutral description; nostrip preserves package bytes for provenance.
The original lock stays intact and an explicit overlay receipt records changed
hashes. Kernel configuration, root guard, switch-root and userdata code compare
byte for byte with the shared input. No new1006 script/helper executed.

The dpkg backend resolves usrmerge/alternatives, Multi-Arch owners and relocated
dracut content. It compares selected bytes and records generated metadata
honestly. Real isolated-dpkg metadata fixtures pass and refuse changed owned
bytes, unknown ELF, non-executable interpreter scripts and foreign architecture.
The fixture initially omitted dpkg info/format1, making Multi-Arch list lookup
use the legacy unqualified path; the corrected fixture explicitly initializes
the database format. This was fixture setup, not a relaxed production check.
Actual native dracut/provenance execution remains pending source approval.

Ubuntu's documented default MAC is AppArmor; SELinux is an optional userspace
stack and not equivalent integration. The current shared source lacks AppArmor.
The optional SELinux package test was a compatibility probe, not authority to
force a Fedora image policy. No security setting is changed by the overlay.
The final profile requires a separately approved, validated decision before
root/boot artifacts can be emitted. Refer to Ubuntu's privilege-restriction
documentation and BOOT-ADAPTER.md for the concrete boundary.

Native ARM64 run41322854619 also passed the metadata backend against actual
official Tenacious package files: usrmerge-selected cat/readlink, systemd,
Multi-Arch libc and a relocated dracut script. Package bytes, ARM64 ELF headers,
copyright/common-license records and generated-metadata distinctions passed.
This ran project-owned host metadata code only; it did not invoke the1006 Uke
preparer, native helpers, kernel or boot producer and did not construct an
initramfs. Source-specific native execution remains pending authorization at
this checkpoint. The current source archive includes the bounded adapter and explicit
dracut/f2fs/systemd/provenance host dependencies; their APT simulation resolves.
No further implementation is started while that authorization remains pending.

### Superseding release1006 and main consolidation result

The later source-specific approval covered exact commit fcbdae4b, native boot
helpers and disposable module signing on the server. The clean Tenacious ARM64
trial succeeded with release7.3.1-1006u1. Every original artifact digest passed
after recovering the uploader-omitted config from the Image's unique IKCONFIG
stream and comparing its recorded hash. The original archive was not changed.
All823 module signatures verify against the certificate embedded in Image.
Both Android-v4 wrappers read back to the identical Image. The embedded base
matches318 file digests,116 links and all90 pinned early drivers; one legacy
shell link is deliberately absent for the typec-v1 producer. No early PAS,
Python, host credential database or firmware payload was introduced.

The tested host compatibility changes are now retained in the Debian adapter:
Ubuntu's blkid path, exact package-content replacements, the literal shared
unit drop-in, generated initrd release metadata, root-attested hwdb generation
and validation of locked generated account tables before their omission.
Unknown accounts, password hashes and empty password fields still fail.
An explicit complete-producer trial action retains one module key through both
Image passes. The normal DEB source build remains a first-pass candidate route;
Launchpad cannot yet prepare the required root-owned hwdb receipt. This is an
implementation gap, not an upload-key issue.

Four inert ARM64 trial DEBs from the verified cohort passed exact version
dependencies and byte readback without maintainer scripts, live module paths
or boot activation. Their build record explicitly says artifact packaging,
not source build. Full boot-userspace licensing collection remains required
before redistribution. No root filesystem image or public Ubuntu release was
produced; root security/layout, runtime OEM DT and handoff remain unqualified.

The unpublished Ubuntu history was reconciled with main through a9428ca,
preserving the newer TUI/GTK/Qt source and package dependency fixes. The first
canonical firmware main recipe passed source assembly and a clean native
Tenacious ARM64 binary build at e76e456. Generated overlay, same-main revision
guards and original source/license hashes passed. Initial host trials caught
Git's exact source-directory ownership check, git-build-recipe's cwd-relative
nest-part collision and an unsatisfied declared jq dependency. Correcting the
isolated host setup and installing declared Build-Depends resolved them;
neither package dependency checks nor source guards were bypassed.

The active Launchpad recipe is unchanged. Native server qualification alone
does not authorize deleting its source branch: import main, qualify the actual
replacement source and ARM64 Launchpad builds, then archive unique history
before removing obsolete heads. Ubuntu source CI follows main; expensive
native/user-root workflows are explicit dispatches rather than stale branch
push hooks. Other snapshot routes still need canonical-main adapters.

The first exact-main hardware source-contract CI reached the unchanged Fedora
return ABI audit and failed on a pruned pinned COPR RPM (HTTP404). The preceding
main validation and family metadata fixtures passed. The UI task had already
qualified the same bytes from the existing consolidation release after filling
its local cache; a clean CI worker lacked that cache. The host audit now falls
back only for a fork RPM to the existing archived filename, verifies its original
pinned SHA256 before admitting it to the cache, then retains every original
signature, architecture, scriptlet, policy, DSO and interface assertion. Original
Fedora input failures remain fatal. This changes neither runtime dependencies
nor ABI admission and does not execute a payload or publish a COPR package.

### Additional main data adapters, local preparation

Three additional native snapshot adapters select the shared desktop integration
component directly from main: DisplayPort/UCM policy, speaker-controls data and
GPU defaults. The source subtree, including its component license, remains
unchanged from the previously qualified baseline. Generated Debian overlays
derive from the release metadata, prefix source input paths and preserve every
runtime dependency and missing hardware ABI gate. No application fork, new
source pin, upstream execution, Launchpad mutation or branch deletion is used.
Generated-overlay and upgrade-order metadata fixtures pass. Real source recipe
assembly and package payload qualification are separate next checks; these
local adapters are not native Tenacious or Launchpad qualifications.

Subsequent local checks use official Ubuntu24 x86_64 host tools with network
disabled for recipe/package assembly. All three source recipes construct, and
data-only ARM64-tagged DEBs build with dependency checks enabled. Make is now
explicitly a build-host dependency (`make:native`), as cross-build metadata
correctly refused the previously unqualified tool dependency. No dependency
check was skipped. Every installed policy, desktop/XML file and both UCM links
read back unchanged. This is data assembly on x86_64, not native Tenacious or
runtime qualification; all existing hardware ABI dependencies remain required.

Historical source views can now be materialized from the exact saved archive
tags with `prepare-archived-recipe.sh`. It checks the source commit and pristine
metadata ancestry against the publication manifest, then creates only disposable
local compatibility branches for the official recipe tool. The published
repository does not need long-lived export or pristine branches for this recovery
path. The speaker-controls trial reconstructs the original archive byte for byte;
the complete nineteen-route reconstruction is being checked separately. These
historical views are not new main-based daily recipes or publication admission.

The complete nineteen-route historical reconstruction subsequently passed:
every original archive equals its recorded SHA256, using only saved tags and
disposable local compatibility refs. Five new main adapters also passed clean
native Tenacious ARM64 source/package builds at dcbf828: three data policies,
Bluetooth and speaker orientation. Artifact hashes, all three input commits,
target architecture and payload audits pass; all installed data bytes match
the common source. The shared component itself remains byte-identical to the
previously approved cbecc44 baseline. These five routes remain unenrolled.

The actual speaker replacement source recipe4108876 and ARM64 build33899943
succeeded. Its source manifest records20c7d84 for every nested input and its
payload preserves the pinned preset, notice, license and conffile declaration.
An isolated Ubuntu24 dpkg lifecycle test registered arm64 as a foreign
architecture, upgraded the actual old/new data DEBs with a synthetic local
preset modification, preserved that modification through removal, and purged
it only on explicit purge. No architecture or dependency bypass and no target
program execution occurred. Signed APT publication remains a separate gate;
source/build success alone does not authorize branch cleanup.

The root Ubuntu integration was found still following hardware's ubuntu branch.
Its .gitmodules now follows main and pins the checked shared source at dcbf828;
root workspace-link CI passed at ed75a0a. No Fedora root, image release,
security setting or device operation changed during this correction.

While the external publication gate remains pending, local main source adapters
are also prepared for core dependency selection, front-color and proximity
capture. Core's existing unqualified Ubuntu profile and missing ABI dependencies
are retained. Sensor source remains unchanged from cbecc44; front-color builds
through the shared component Makefile in its nested source directory, while
the root-only proximity diagnostic keeps its original permission mode. All
generated-overlay, dependency and upgrade-order fixtures pass. Source-package
construction and native qualification remain separate checks before enrollment.

These three further adapters subsequently passed native Tenacious ARM64
qualification at94f3f26. All artifact and source/input hashes verify. The core
profile is byte-identical to the canonical unqualified profile, and proximity
capture retains0750 mode. Front-color compiles through the unchanged shared
Makefile; no target helper is executed. They remain unenrolled and do not admit
the missing runtime ABI or a system image. Eight additional main routes now
pass native builds, in addition to the first speaker preset; ten historical
non-kernel adapters remain unfinished.

## Installation profile source adapter

1 November 2026. The installation classifier now has a canonical-main source
recipe. Its native snapshot overlay prefixes shared source inputs and retains
the original device/container guards in the maintainer script. The complete
owning boot component compares unchanged to the approved source baseline.
Generated metadata, runtime dependencies and version ordering pass static
checks; native compilation and package readback are still pending. This does
not qualify a root image, activate boot tools or establish device behavior.

Two remaining routes need input reconciliation before main adapters: the
battery 1.4.1 source archive includes the mandatory integrated-protection proof
that the common Git component lacks; touchscreen assembly requires the three
pinned Apache-licensed donor inputs. Neither may silently fall back to older
Git inputs or omit the reviewed preparation step.

## Actual Launchpad construction and publication gate

1 November 2026,21:53 UTC. Replacement source recipe4108876 and its ARM64 binary
build33899943 succeeded. The actual source manifest records20c7d841 for all
three nested main inputs; the extracted preset, notice, attribution and license
match canonical inputs. The actual old-to-new data DEB upgrade preserves a
synthetically modified conffile; removal retains it and purge removes it. This
used an official Ubuntu24 x86_64 container with registered ARM64 foreign
architecture and no target execution. The source publication is still Pending;
the signed PPA index continues to expose the earlier published version. Do not
infer APT availability or remove old heads until signed index, exact payload
and upgrade-solver checks pass. Both public source/contract CI checks passed
at5fa0f59. This records package construction, not runtime qualification.

## Dependency-first canonical libssc source

1 November 2026. The installation classifier subsequently passed native
Tenacious ARM64 construction at 4600858f32a8c704c0120c6e32bc11a5f6d87eaa.
Downloaded artifact digests, same-main input commits, the AArch64 executable,
service bytes and original actual-Uke/container/chroot maintainer guards verify.
This supersedes its pending native status above; no service or tablet ran.

The libssc adapter now retains Fedora's exact 0.4.4 source archive below the
owning sensor component and applies the same seven shared patches during host
source preparation. The preparation helper checks the archive digest, rejects
traversal and requires zero-fuzz patches. Generated native metadata retains the
runtime/dev ABI relationships and excludes Python mocks through the original
Meson options and payload audit. Explicit patch tooling is a build dependency.
Source tree comparison and native qualification are pending. SensorProxy must
wait for the real qualified libssc-dev package; no ABI provider is fabricated.

Superseding libssc qualification: the clean native Tenacious ARM64 build at
`d0d453a31772b821b08a3b50b5ff3d29520e6f18` passed for all four binary packages.
Downloaded artifact and per-file hashes, AArch64 payloads, absence of Python
payloads/direct runtime dependencies and equal recipe input revisions verify.
Every unpacked upstream file matches the older qualified quilt result. Source
regression fixtures now reject corrupted archives, missing or broken patches
and partial preparation; successful patching leaves no offset backup files.
The main snapshot retains real library/dev ABI relationships and upgrade order.
This is server package qualification, not Launchpad enrollment or sensor success.

The native libssc layout's copyright header now links its exact upstream
archive directly and names the nested source/src MIT file. The conventional
release overlay continues to reference its generated origin records. This
corrects metadata paths that do not exist in the native snapshot layout;
generated-overlay fixtures verify both paths. Compiled source, patch identity,
build rules and runtime dependencies are unchanged from native qualification.

## Verified publication and main-only CI transition

1 November 2026, 22:08 UTC. Launchpad published replacement source 18914465 at
22:05:34. Verification passed the PPA key, signed InRelease, SHA512 package and
source indexes, both source files and exact ARM64 DEB. The authenticated binary
matches build33899943 byte for byte. The isolated APT solver upgrades the prior
published version to the exact new native snapshot without removals or a
downgrade; the separately tested conffile lifecycle remains valid. This
supersedes the Pending archive reports above. The stable archive remains gated.

For the main-only repository, both general source-validation workflows now
filter pushes to main while retaining pull requests and manual dispatch.
GitHub's documented push event also includes branch deletion; limiting retired
head events avoids redundant jobs during archive-backed cleanup. All main
safety checks remain in place. This changes trigger scope, not validation.
Reference: https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#push

## Remaining canonical input reconciliation

1 November 2026. All eight remaining main adapters now have draft native
metadata. They retain the previously approved source archives and patch counts,
not newer upstream branch tips. Exact archive hashes match the historical
native source set. Touch retains the three pinned Apache inputs and the shared
geometry/assignment preparation script. Recovery delivery retains existing
v003 bytes, notices and source offer; it neither compiles nor installs recovery.
Image tools remain at the approved builder source pin and build six native
utilities, without composing a guest image.

The common battery component now matches every file in the reviewed 1.4.1
release manifest, including the mandatory integrated-protection evidence field.
Both Fedora and Debian now consume that common code. SSC's cache and fixture
match the two recorded release26 override hashes. The existing ownership,
permissions, packaged fallback, read-only persist and missing firmware/ABI
admission boundaries are retained. This restores already reviewed source
behavior; it imports no new hardware implementation or operating limits.

The shared donor policy, SM7675 survey/acquisition inventory, MiCode source
survey and Uke battery protection review were consulted. They preserve OEM
kernel5712080a/DT94c84aeb, upstream4056b3c9 and the distinct-board compatibility
limits. No donor checkout/script was used as a new hardware fix. Native source
and package qualification of these drafts is pending; live PPA enrollment stays
separate. Existing Fedora/GUI changes and main-only architecture are preserved.

SensorProxy's build dependencies now additionally require the real dual-light
ABI provider, as its runtime dependencies already do. A later stock libssc-dev
version alone must not satisfy this patched build contract. The native trial
built/installed the matching real SDK; installed package versions are retained
for independent readback. This metadata tightening changes no compiled source.
Live SensorProxy enrollment still requires the qualified libssc packages to
exist in the PPA first. It is not resolved by an invented virtual provider.

## Remaining eight native routes qualified

1 November 2026. Clean Tenacious ARM64 run41358005301 passed all eight remaining
main routes at23e25a7f, whose tree matches source-qualified9a7928e. Thirteen
binary packages and the dependency-first libssc SDK were read back; ZIP/internal
hashes, identical nested commits, AArch64 executables and direct Python exclusion
pass. Battery profiles/protection inventory match the reviewed unqualified
release. Recovery data and all source-offer/license bytes match the original
v003 payload and have no maintainer/activation hooks. Image tools contain only
the six approved9e4d2153 utilities. Source fixtures reject corruption, incomplete
or broken patches and changed touch licensing; all release hashes pass.

The SSC legacy RPM revision advances to27 after reconciling the released cache
helper, avoiding a rollback/collision with archived26. Its build/runtime gates
remain unchanged. SensorProxy's stronger build ABI dependency is satisfied by
the exact real SDK present in the native trial; this is not a new compiled input.
No new source revision, physical behavior, automatic stable promotion, Ubuntu
image or tablet operation is claimed. Launchpad main refreshed to9d7c056 after
its prior import completed; archived mirror heads were pruned by that import.

Kernel inspection confirms a separate implementation gap: default Debian rules
still emit three first-pass candidate packages, while the complete two-pass
producer remains an explicit boot action requiring a root-prepared hwdb receipt.
Unprivileged source-build provenance, four-role package wiring, runtime OEM DT,
root layout/security and selector/rollback qualification must be resolved before
normal boot admission. The selected shared config lacks AppArmor; no security
profile or setting was changed to bypass that gate.

## Reuse verified native output before rebuilding the kernel

1 November 2026. The existing source-specific native1006 output is sufficient
for a four-role Debian packaging trial. A fixed receipt binds fourteen inputs
to source fcbdae4b and native producer41331215146, including the recovered
IKCONFIG digest and unchanged original ZIP digest. An explicit Debian build
profile snapshots only those files, repeats receipt/crypto/cohort checks and
uses Debhelper to emit inert core, DT, modules and boot roles. It refuses the
source producer, missing inputs, activation hooks and runtime ABI providers.
No new upstream compilation, native helper execution, key generation or image
composition is needed. Default source builds still produce first-pass candidates.

The actual binary packaging trial passed in a network-disabled official
Ubuntu24 x86_64 tools container using the verified ARM64 bytes. The first
attempt exposed an invalid cross-build dependency qualification: dpkg rejects
`:native` on Multi-Arch: foreign packages. Correcting that metadata allowed
the full build without bypassing dependency checks. Final DEB readback and
negative fixtures verify bytes/modes/control members, four active roles,
exact dependencies, receipt corruption, linked inputs, hooks and false providers.
Source export tests compare the included common tools to their canonical files.

The real31-DEB APT fixture still rejects the same eleven binary chains;
twenty resolve, with Python confined to two development SDK plans. Signed
module metadata confirms the existing MSM separate_gpu_kms parameter and
guarded battmgr symbols. Touch and FS19xx modules match the kernel release.
These are software inventory results, not full runtime ABI or hardware proof.
The explicit gap plan distinguishes further userspace porting, boot/runtime
qualification, firmware rights/local input design and root security decisions.
The original trial also lacks the complete embedded-userspace license
collection, so binary redistribution stays blocked. No Ubuntu image, security
change, stable promotion, new credential or tablet action occurred.

## Explicit Ubuntu AppArmor selection

2 November 2026. User approval selects AppArmor for Ubuntu server builds while
requiring Fedora to retain its existing security configuration. Inspecting the
actual pinned Kconfig shows SECURITY/NET dependencies and selected AUDIT,
SECURITY_PATH/FS/NETWORK facilities. AppArmor's LSM is exclusive; Ubuntu uses
lockdown,yama,apparmor,bpf, with the shared signature/lockdown/Yama and memory
protections retained. Fedora's original config and contract remain unchanged.
An explicit host profile tool separates output directories and rejects missing
selection, contaminated baselines, output reuse and weakened protections.
The new Ubuntu release1006u2 distinguishes the changed module cohort from the
historical1006u1 artifact. Source/refusal tests pass; real Kconfig/native build
and running policy results must be recorded separately. Existing artifacts
cannot prove the new configuration and are not relabeled or promoted.

Launchpad SensorProxy source4108935 separately failed because its amd64 source
builder tried to install the ARM64-only patched SSC SDK/ABI. Build dependency
architecture qualifiers now omit only those two gates on amd64. ARM64 retains
the exact version and real ABI gate; runtime dependencies remain unchanged and
configuration explicitly rejects an amd64 binary build. Dpkg reduction/refusal
fixtures pass. This fixes source construction without inventing an SDK provider
or changing the approved compiled source.
The first public CI caught a stale generated canonical-main adapter. It was
regenerated and the new architecture fixture compares both control files and
requires the compile guard in the actual Launchpad adapter as well.

## Ubuntu AppArmor native cohort verified

2 November 2026. The exact Ubuntu security adapter at 0d979e3 passed a clean
Tenacious ARM64 kernel build with release 7.3.1-1006u2. Both effective distro
configs were retained: Fedora keeps AppArmor off and its SELinux LSM list;
Ubuntu selects lockdown,yama,apparmor,bpf and the required AppArmor facilities.
All other preflight settings compare equal outside the approved profile fields.
The original shared baseline is unchanged. No Fedora kernel or image was rebuilt.

Independent host readback of the downloaded native output matches Image
IKCONFIG to the recorded final Ubuntu config, verifies all 823 module signatures
against the embedded certificate, both boot wrappers, 90 early drivers, 318
base-file hashes and 116 links. The compile-stage DTB matches the preceding
exact-source cohort. These are build/artifact results; actual boot and AppArmor
policy loading/enforcement are untested. Normal complete Debian boot packaging,
root policy/layout/activation/rollback, firmware inputs, license/source-offer
audit and physical qualification remain separate. No runtime ABI provider,
Ubuntu system image, public image release or stable promotion is admitted.

The first successful native compile lost its export because a root-owned hash
manifest could not be overwritten and multiarch license paths contain colons
that the artifact uploader rejects. Tested atomic manifest replacement and a
portable outer tar preserve original notice names/data, including three bounded
common-license aliases. The corrected same-source build/export and independent
ZIP/tar/inner hash checks passed. No kernel protection or source identity was
changed to repair transport.

2 November 2026, complete Debian cohort integration. Normal source packaging
now invokes the existing full two-pass producer and stages four exact-version
core/DT/module/boot roles without boot activation or ABI providers. A separate
verified-cohort profile binds existing run41390762590 AppArmor outputs and
notice inventories, avoiding another full kernel compile for private image
preparation. Actual dpkg-buildpackage artifact import and complete DEB payload,
mode, control and dependency readback pass on official Ubuntu24 amd64 tools;
no target binary was executed. Historical u1 receipts remain separate. The new
normal source-package integration has not itself been natively rebuilt. Root
policy, filesystem/image and hardware qualification remain distinct work.
