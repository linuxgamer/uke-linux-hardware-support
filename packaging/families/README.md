# COPR source families

1 November 2026: `mcc45tr/uke-linux-unstable` uses 23 active source producers,
with only `fedora-rawhide-aarch64` enabled. The existing
[`uke-control` source family](consolidation/control-family-results.md) owns
12 independently installable RPMs. The visible list also retains 20 historical
definitions, for 43 rows in total. The nine umbrella recipes and earlier
observations below remain the legacy stable/test path.

27 October 2026: the hardware update cohort (the revision was removed with
 the top-level docs tree; recover it from git history)
joins Core `1.15.1-3`, hardware `1.8.5-1`, power `1.3.3-4`, sensors `1.0.9-5`,
extras `1.0.4-2`, additional `1.0.0-13`, metas `1.4.0-4`, camera `1.0.0-3`
and sound `1.0.5-1`. Non-kernel children now all use the `uke-` prefix.
Sound owns the retained PipeWire source recipe, speaker orientation and generic
preset; extras owns the renamed material decoration. Hardware requires the
existing BlueZ/libinput/libwacom infrastructure. Core selection requires the
matched release-33 kernel and sound family, including when weak dependencies
are disabled. Focus input hardware acceptance remains open.

27 October 2026: core `1.15.1-2` carries the read-only lid/screensaver and
active-depth/DSC diagnostic correction. Its exact Control requirement matches
both rebuilt children. A standalone `1.15.1-1` test cannot replace Control below
the installed Core pin; use the complete matching family without relaxing that
dependency or removing Core. No hardware or kernel family is rebuilt here.

24 October 2026: core `1.9.1` adds disconnected DP training ceilings,
per-device USB runtime power and RTL8153 link capability/profile controls.
It preserves core `1.8.1`'s bounded native KScreen session controls.

27 October 2026: additional `1.0.0-12` incorporates the unchanged, pinned Mesa
SPIR-V source recipe as the `uke-mesa-libclc22-spirv 0.2.0-2` child. Its original
source RPM is retained in the permanent source archive with SHA-256
e648ef77ffb84523a675fb4a879a18e3cad0d64c1af3d8248e8fd91f03de0643.
Every rebuilt additional child receives a higher release. This removes the
need for a separate libclc COPR source definition after replacement package
and dependency validation. Source composition, native SPIR-V validation and
tablet OpenCL/render acceptance remain separate results. Chromium stays
withdrawn; no new family, project or custom directory is introduced.

26 October 2026: additional `1.0.0-10` integrates the experimental Iris VA-API
backend as the separately installable `uke-vaapi-v4l2` child. Its intermediate
component source RPM is not an independently registered COPR package. Existing
additional children receive fresh revisions for the new cohort. Backend source
and fixtures match revision0.6; native-family packaging and device validation
remain separate gates. All new project package names start with `uke-`; no
separate Chromium build or additional source family is introduced.

24 October 2026: core `1.8.1` provides bounded native KScreen session controls.
Metas `1.1.0-2` contains the separate `uke-plasma-controls` RPM, recommended only
by `uke-kde-plasma-meta`. Removing the widget retains the KDE meta package.
The applet delegates all settings to `uke-control`, uses standard Polkit for
USB administration and advertises no unavailable hardware control. GNOME has
no widget dependency or integration. Source and package checks are separate
from tablet UI acceptance.

24 October 2026: metas `1.0.6-2` requires original Fedora `firewalld` for
the policy XSD used by the mandatory native check. `firewalld-filesystem` owns
the RPM macro and directories but not that schema. Build `11202425` stopped
at this missing build dependency and produced no installable correction.
Desktop children advance to `1.0.0-10`, Core metadata to `1.0.5-4`.

24 October 2026: metas `1.0.6-1` delivers Plasma selection `1.0.0-9` with
original Fedora KDE Connect autostart and a vendor firewalld policy limited to
private IPv4 and local IPv6 sources. Pairing and administrator overrides remain
authoritative. The unchanged Core selection advances to `1.0.5-3` for the
rebuilt cohort. Policy schema/source checks, native COPR packaging and actual
tablet discovery remain separate validation steps.

23 October 2026: sensors `1.0.9-3` advances every rebuilt child revision before
integrating the published hardware sources into main. The shared sensor
Makefile gained touch-capture host targets and is included in the Front Color
and proximity source archives. Their compiled inputs and runtime behavior are
unchanged. A source revision still requires new RPM identities; existing
same-NEVRA and duplicate-build guards remain mandatory.

23 October 2026: hardware `1.8.2-1` carries the DSI connector health correction
in its matched native children. Unchanged firmware advances to release 9 for
a coherent, monotonic rebuild. Host checks and tablet validation are distinct.

23 October 2026: hardware `1.8.1-2` advances boot children to `1.8.1-2` and
the unchanged firmware payload to `3.0.304.0-8`. Build `11199340` was cancelled
before publication after detecting its unchanged firmware RPM revision.

23 October 2026: hardware `1.8.1` corrects repeated DNF hooks before an A/B
trial reboot and narrowly restores a stale backup array CRC during acceptance
or fallback. Both GPT arrays, valid primary data, header identity and header
CRCs remain mandatory. Host fixtures pass; package and tablet validation are
separate. The observed B/7.2.12-14 to A/7.2.12-22 boot remains unaccepted.

23 October 2026: hardware `1.8.0` adds pending-trial fallback after bounded
health retries and admits one exact firmware cohort after external-fastboot
Linux entry in both slots. Unknown cohorts remain refused. The matching kernel
contact client waits for valid frames before reporting readiness. Native/DNF
A/B, hard-hang recovery, power-loss and independent recovery tests remain open.

23 October 2026: the active Uke COPR projects are `uke-linux` and
`uke-linux-test`. The four frozen SRPM inputs use the
[permanent source archive](https://github.com/MCC45TR/uke-linux-hardware-support/releases/tag/copr-migration-archive-20261020)
with their original SHA-256 values. Historical COPR build identifiers remain
provenance records. A byte-identical URL relocation requires no binary rebuild;
changed source hashes still require a family revision. Source contract and
trigger fixtures cover this distinction; tablet state is unaffected.

23 October 2026: sensors `1.0.6` adds the separately installable, root-only
`uke-proximity-capture` diagnostic. It is not a default dependency or service.
Fixed bounded modes compare the captured Uke format with libssc's request;
they do not replace the decoder, set calibration or establish near/far support.

23 October 2026: hardware `1.7.0` is a source migration candidate. Boot
integration owns the existing calibrated touch service and its legacy rollback
client, with a native launcher for a client supplied by the running kernel's
module package. The separate `uke-touchscreen` child is no longer built.
Private data remains separate. Native source generation, host boot/update/USB
fixtures and source-family checks pass; COPR and tablet migration are pending.
See [touch runtime migration](../../uke-boot/docs/TOUCH-RUNTIME.md).

23 October 2026: sensors `1.0.5` builds the front-color publisher from its
current owning source instead of the archived release-2 source RPM. It retains
the SUID readiness subscription while publishing unavailable startup state,
with no timed retry or DSP reset. Host fixtures cover late readiness,
cancellation and event limits; cold-boot tablet acceptance remains separate.

23 October 2026: extras `1.0.4` delivers `uke-plymonth 1.0.2-4` with a required
original Fedora label plugin. The first tablet boot exposed a script-plugin
crash when text rendering was absent. The package build now exercises the
engine's real space-glyph rendering check as well as the theme parser. This
dependency correction does not establish physical theme visibility.

22 October 2026: extras `1.0.3` delivers `uke-plymonth 1.0.2-3`, adapting the
attributed Xiaomi/Fedora artwork from Nabu. Metas `1.0.5` adds it as a Core
recommendation. Normal DNF upgrades install the new weak dependency; removing
the theme preserves Core and subsequent upgrades respect that removal under
the default DNF policy. Original Fedora Plymouth units show the graphical theme
after root mount and during shutdown. No Nabu repository, boot image update or
slot operation is introduced. RPM scriptlet, parser and isolated DNF fixtures
are separate from physical Uke display and shutdown acceptance.

22 October 2026: hardware `1.6.0` adds the native updater's guarded restoration
of a booted, unaccepted current-slot trial to its recorded accepted B image.
The failed trial is not marked healthy. Package/module verification, physical
storage checks, battery admission, durable backup and write/readback remain
mandatory. Other hardware children retain their inputs with fresh revisions.
Host tests do not establish tablet restoration or interrupted-write recovery.

22 October 2026: skipped-release resolver checks cover the earliest retained
`7.2.9-1` component baseline, the pre-family `7.2.11-9` package set and the
reported mixed `7.2.12` kernel list. Each resolves directly against current
test COPR and Fedora Rawhide metadata in one transaction. No intermediate
repository, sequential upgrade or dependency-ignore option is required.
Name-based replacements cover the first retired power backend and standalone
Bluetooth packages, with the existing service-state migration. These checks
use isolated RPM header databases; they do not execute target scriptlets or
establish tablet installation. Default kernel retention may replace an old
kernel set, so the proposed transaction remains relevant to rollback planning.

22 October 2026: power `1.3.3` and additional `1.0.0-6` restrict upstream file
globs to their owning source. UPower no longer includes sibling Uke service,
D-Bus and udev files; libaccounts-glib-devel no longer includes AccountsService
interfaces. Shared-root fixtures retain each source's own files and reject
sibling capture. Rebuilt children receive fresh revisions. Bluetooth source
changes are watched by the hardware family only.

22 October 2026: hardware `1.5.3` grants the existing Bluetooth factory helper
read/search access through Android-owned directory permissions. The service
retains protected read-only persist and its exact identity and input checks.
A host capability fixture verifies read admission and write refusal. No service
is newly enabled and no factory permissions are modified. Tablet controller
startup and pairing remain separate validation.

22 October 2026: power 1.3.2-1 replaces the retired
`uke-power-profile-backend` package and requires the original Fedora
`power-profiles-daemon`, the native kernel profile provider and boot readiness
integration. The migration preserves enabled, active and masked service state;
it supplies no legacy capability alias. This allows the existing kernel boot
dependency to remain strict while removing the obsolete backend. All rebuilt
power children receive fresh revisions. Resolver checks and COPR results are
recorded separately from tablet power-profile and Bluetooth validation.

22 October 2026: hardware `1.5.2` applies the reviewed panel-start patch to
the pinned touchscreen source. Touchscreen `1.0.14` can wait for the kernel's
panel notification while DPMS is off. It reports standby explicitly, creates no
input device and performs no firmware transfer until the panel is ready. This
allows the graphical target to start without a failed-service retry loop.
Missing/malformed providers still fail admission; cancellation closes the
transport. Existing firmware, CRC, geometry, watchdog and input tests remain.
This source correction is separate from the pending touchscreen kernel port
and requires signed-package and tablet lifecycle validation.

21 October 2026: power 1.1.1-1 supplies adaptive display 1.0.16 with original
KWin's `Always` VRR policy for `DSI-1`. Existing user settings remain
authoritative. The O82 kernel opt-in and physical variable-cadence acceptance
are separate. Other power children retain their sources and receive fresh
RPM revisions.

21 October 2026: sensors 1.0.4-1 builds libssc from its owning component recipe
and rejects invalid proximity values without aborting the client or changing
its last valid state. The actual callback passes host fault fixtures, including
fractional and non-finite vendor values. All sensor children receive fresh RPM
revisions. Native package and physical sensor acceptance remain separate gates.

21 October 2026: additional 1.0.0-5 keeps AccountsService's compiled release
version equal to its signed Rawhide archive. RPM-generated and ancestor Git
history no longer determines that metadata. All rebuilt child revisions advance;
the installed Release-4 functional baseline and separate native/target gates remain.

21 October 2026: core/control 1.1.1-1 excludes ordinary sysfs attributes from the
device and policy inventory after the 1.1.0 tablet readback exposed an extra record.

Core/control 1.1.0-1 adds cached chipset/cache/accelerator metadata
to the existing hardware status and private reports without hardware control writes.

Core/control 1.0.24-1 scopes vendor exceptions to exact planned
package/repository/vendor jobs while retaining global vendor restrictions.
Signed repository fixtures include combined update/rename and an unrelated
foreign-vendor candidate. Native packaging and target transactions are separate.

21 October 2026: power source 1.1.0-4 recognizes the standard Wireless supply
type in original Rawhide UPower 1.91.5, retaining kernel online values and
unknown-type diagnostics. Every rebuilt child advances; UPower is 1.91.5-8.
The private-bus fixture includes offline, online, malformed and missing values.
This classification establishes no physical wireless-charging capability.

21 October 2026: additional source 1.0.0-4 includes the original Rawhide
AccountsService base, an optional-directory correction and bounded Turnip
display-fd handling. The source watch now covers ten Fedora-derived bases.
Mesa's feature fixture links its real CPU producer and runs before the complete
driver build. Native source 1.0.0-3 stopped at that fixture after compilation;
no successful package or device result is inferred from it. Release-4 native
build and tablet acceptance are separate gates.

21 October 2026: [Rawhide tracking](RAWHIDE-TRACKING.md) replaces historical
frozen inputs for the modified Fedora source bases, including UPower,
SensorProxy and the native PPD backend. The six-hour watcher verifies signed
source payloads, advances complete child cohorts and uses validated test-build
admission. Initial refreshed source revisions are additional 1.0.0-3,
power 1.1.0-3 and sensors 1.0.3-2. Native package and tablet results remain separate.

21 October 2026 source follow-up: core 1.0.23 retains the exact device-private
touchscreen-data package during public synchronization and excludes public
candidates for that identity. Generic legacy firmware replacement remains
explicit. No private data enter a family source or binary RPM.

21 October 2026 source follow-up: hardware family 1.1.3-1 includes the bounded
journal-filesystem policy in the existing installation classifier. Its base
data package is an exact classifier dependency. Other rebuilt children retain
their source inputs and receive fresh RPM revisions; firmware content is
unchanged. Core 1.0.22 corrects inference of an unset channel from the one enabled
normal Uke COPR. No automatic channel transition or boot activation is added.

21 October 2026 sensor source follow-up: family 1.0.3-1 builds the file-service
recipe from its owning component, including the cache-generation correction.
The upstream archive remains pinned in that recipe; the historical frozen SRPM
input is no longer active for this component. Other sensor children retain their
pinned source inputs and receive fresh package revisions. Installed firmware and
active RAM/DSP content remain separate evidence.

21 October 2026 source follow-up: hardware family 1.1.2-1 carries updater 1.1.2.
The native automatic entry point requires both strict global and userdata
policies before any acceptance or staging operation. Private VRR derivation
requires a signed manifest capability and a retained structured Image note.
All rebuilt children receive fresh package revisions. Hardware Release 4 was
published but was not installed on the tablet during this maintenance pass.

21 October 2026 correction: hardware source Release 4 keeps the current boot
implementation and the reviewed Fastfetch 1.0.1 physical-storage profile. Its
exact boot, classifier and updater requirements match the generated Release 3
children. The source composer refuses inconsistent exact child dependencies,
including ISA-qualified dependencies. Each rebuilt child has a fresh RPM
revision. Required firmware file paths remain explicit primary-metadata
providers. Host source generation and dependency fixtures do not establish
tablet installation or hardware acceptance.

20 October 2026. Fedora Rawhide AArch64. This directory owns the Uke source
package map. COPR lists source packages in a flat list; it has no nested package
folders. Each family is one source RPM producing separately installable child
RPMs. Installing a metadata parent starts no process by itself.

The runtime responsibility map records which functions
belong to kernel/DT, original distribution services and bounded Uke helpers.

| Source family | Child responsibilities |
|---|---|
| uke-core | uke-control and optional development diagnostics |
| uke-hw | boot/USB/Wi-Fi integration, installation classifier, firmware, Bluetooth factory address, touch runtime with matched-kernel dispatch and original Fedora Fastfetch |
| uke-power | original UPower with reviewed corrections, battery observation/control gates, PPD backend, adaptive display |
| uke-sensors | SSC library, standard SensorProxy with SSC, bounded file service and opt-in front color |
| uke-additional | AT-SPI, GStreamer, libaccounts-glib, libwacom, C++ runtime and Mesa; original upstream versions retained |
| uke-extras | optional uke-plymonth theme, original material-decoration and recovery images delivered as data |
| uke-metas | separate strict console and original Fedora Plasma selections |
| uke-camera | optional original Fedora camera application selection; internal camera graph unqualified |
| uke-sound | original Fedora ALSA/PipeWire/WirePlumber selection; internal playback/capture unqualified |

The four kernel source names are `senemos-uke-linux-kernel-mainline`,
`senemos-uke-linux-kernel-mainline-singleboot`,
`senemos-uke-linux-kernel-mainline-kvm` and
`senemos-uke-linux-kernel-mainline-kvm-singleboot`. The resulting source list contains
13 families. The `material-decoration` binary keeps its original identity under
`uke-extras`; it no longer needs a separate source row. `uke-hw` also produces `uke-firmware`, an
explicitly requested proprietary data RPM with OEM notices. Its historical
file-level redistribution review remains incomplete; publication does not
establish additional grants. Private calibration is excluded.

Project application versions follow `MAJOR.MINOR.PATCH`: increment MAJOR for a
critical architectural or compatibility change, MINOR for a feature and PATCH
for a correction. Reset lower fields after a higher-field increment. A
packaging-only rebuild increments RPM Release. Upstream libraries, kernels and
third-party applications retain their actual upstream versions. Requested
application identities are `uke-control 1.0.19-1`,
`uke-adaptive-display 1.0.14-1` and `uke-plymonth 1.0.2-1`; the latter is the
requested package spelling, while the original Plymouth theme file paths stay
compatible. Do not add Epoch to outrank another repository.

The subsequent coordinated corrections use `uke-control 1.0.20-1` for F2FS
swap allocation and `uke-adaptive-display 1.0.15-1` for original KWin defaults.
The latter ships a generic monotone brightness curve, tablet-mode rotation and
automatic VRR policy only for DSI-1, without forced layout/resolution or factory
calibration. The configuration is installed with noreplace semantics; existing
user settings remain authoritative. These changes require their own native
build and device checks rather than replacing the earlier package identities.

The family composer retains complete exact-pinned source RPM inputs, original
specifications, archives, patches and licensing. It expands the reviewed RPM
recipes for the native target and combines their build/check phases in isolated
source directories. Binary RPMs are built from source, never copied from an
old binary package. Source input hashes and build IDs are in
`source-inputs.json`; component source changes require an explicit input refresh
and package revision review. Local Control, adaptive display, battery and meta
sources come from their current owning component recipes. The existing input
boot-22 keeps the frozen userdata image ABI intact.

Hardware source 1.0.0-2 retains boot22 and separately builds the corrected
`uke-installation-profile 1.0.31` from the coordinator's canonical C++ source.
The original classifier30 service unit is retained. Reconciliation runs only on
a real Uke outside a container/chroot, may repair only the exact legacy unknown
record, and preserves masks. It performs no block writes or slot changes. The
new A/B backend remains a separate future source freeze.

Hardware source 1.1.1-2 retains the subsequently packaged 1.1.1 boot/update
implementation and extends the canonical firmware replacement bounds to
`.owner.fc46` package variants. Unsuffixed compatibility Provides remain
available. Firmware Release 2 contains the same 217 generic files; private touch
data remain separate. Every rebuilt child has a new packaging revision.
Re-querying the captured tablet RPM database confirmed that its four owner
packages actually use unsuffixed `1.owner` releases. The suffix extension is a
compatibility correction, not an observed migration failure on this tablet.
The updater's existing `automatic=0` setting and administrator masks remain
authoritative; package installation does not qualify a boot transition.

Firmware source inputs retain their exact upstream Version and Release.
Foreign DSP ELF data is held outside the buildroot while RPM runs native
post-install processing, then restored before the firmware hash checks. Native
programs retain normal post-processing. The final firmware RPM must match all
217 source file hashes and modes; a source check alone is insufficient.

The source family Release defaults to 1; set its `release` field when packaging
changes require a rebuild. Control's family version and Release must match its
owning spec; CI refuses a mismatch. Native child recipes keep their Release
once their source Version matches the declared native Version. Refresh exact
dependency pins deliberately: the Mesa LLVM build pin is 23.1.3-1.fc46 because
23.1.2-1.fc46 is absent from Rawhide AArch64. The original recipe is retained.

Renamed child RPMs carry versioned Provides (including ISA where appropriate)
and bounded Obsoletes for their previous names. New project names begin `uke-`;
standard executable paths, library SONAMEs, D-Bus names, units and pkg-config
interfaces remain upstream-compatible. Development packages retain exact
runtime dependencies. Meta selections do not force optional diagnostics,
recovery images, physical camera/audio controls or private calibration packages.
The Plasma selection stays independent of the console selection.


Service renaming preserves admitted system-unit state through RPM scriptlets;
a masked or disabled path is not newly enabled. SensorProxy's policy is restored
after the old package's removal scriptlet. Failed restoration retains its
private state for inspection. This is packaging logic, not physical lifecycle
qualification. The Bluetooth helper performs a device-specific factory address
handoff via the standard management API and exits; removing its package would
not remove a persistent extra Bluetooth daemon. UART, pinctrl, regulators and
protocols continue to belong to the kernel/DT and original BlueZ.

The Fastfetch fork is withdrawn. The normal Fedora `fastfetch` dependency is
retained. Control's signed native solver returns that one package explicitly
to `rawhide`, including a necessary downgrade; other project packages still
come from the selected Uke channel. Missing original replacements refuse the
transaction. Hardware identity fixes belong in the real kernel/DT and existing
system integration. Do not manufacture SMBIOS data or probe legacy PC memory
ranges on ARM to satisfy a display field. Full root all-module captures remain
unqualified until the original distribution program safely handles that case.

On 21 October 2026, `uke-hw 1.1.4` retires `uke-fastfetch-config` and requires
the original Fedora `fastfetch` directly. No project Fastfetch configuration is
built or installed. RPM preserves any locally modified configuration as an
`.rpmsave`; an unmodified vendor profile is removed. Model, panel geometry, CPU
and storage information must come from their actual operating-system interfaces.
Native identity and physical-device validation remain separate from this change.

On Rawhide AArch64, build a source family with
`make -C packaging/families srpm FAMILY=uke-core`. The source-generation host
must match the target: RPM's `--target` does not retarget every build/host
triplet in an expanded recipe. COPR source workers can use another architecture;
generate and upload the SRPM from a native target worker in that case. The
source host also needs each recipe's RPM macro providers, including
`cargo-rpm-macros` for current Mesa. Unexpanded build commands are rejected
before composition instead of being escaped into a broken family recipe.
Use `TARGET_ARCH=x86_64` only for isolated x86_64 host fixtures.
Run `make -C packaging/families check` and the original component/native solver
checks. Before changing COPR, validate every generated package name, dependency,
scriptlet, source hash, signature and rollback artifact. A COPR package deletion
also deletes its builds: never use it merely to hide a dashboard row. Preserve
referenced historical builds and test the new dependency closure before a
source-definition cutover. Stable promotion remains distinct from test builds.

Source-family automation watches each family's inputs, requires a higher RPM
revision, runs validation and suppresses duplicate active or published builds.
Shared composer changes rebuild only families with an explicit revision bump;
unchanged families retain their published RPMs. Changes to a family's own
sources, policy or pinned inputs without a revision bump still fail admission.
The COPR source definitions track the published branch with auto-rebuild enabled.
The direct GitHub integration accepts tag creation events; validated CI requests
changed families through the existing custom hook. This avoids two builds for
one branch push. Remaining upstream-only frozen inputs require an explicit hash
and revision refresh. Fedora-derived inputs use the signed Rawhide watcher.

Primary contracts: [RPM spec format](https://rpm.org/docs/6.0.x/manual/spec.html),
[DNF5 native transaction API](https://dnf5.readthedocs.io/en/latest/tutorial/transaction.html)
and [COPR source methods](https://docs.copr.fedorainfracloud.org/user_documentation.html#build-source-types).

22 October 2026: power family `1.3.1` withdraws the compatibility PPD overlay.
It requires the native kernel profile capability, boot integration `1.3.4` or
newer and the original distribution daemon. Images must select a matching
native-profile kernel. Existing installations need a qualified running provider
before removing the old backend through DNF; package presence alone does not
establish the running kernel. Retain signed backend RPMs for explicit rollback
to an older kernel. Historical COPR removal requires a complete replacement
cohort and separately retained signed archives.
The replacement increases every rebuilt child's RPM release so DNF can replace
the complete cohort. The initial `1.3.0` candidate reused child NEVRAs and is
superseded; do not treat a parent-only upgrade as child synchronization.

29 October 2026: run source-family composition in a worker matching the Fedora
target. Set `FEDORA_TARGET=44`, `45` or `rawhide`; the composer checks its worker
release and derives RPM macros there. Child release overrides and cohort checks
use that target. Generated family SRPMs contain expanded upstream recipes and
must be generated separately per release; a Rawhide-generated family SRPM is
not a Fedora 44/45 source profile. Existing Rawhide lock archives are source
evidence, not proof of compatibility with earlier Fedora libraries. Native
builds, dependency resolution and signed provenance remain required per target.
