# Control frontend package contract

This source recipe preserves the signed published `uke-control-2.8.1-5.fc46`
cohort, including its DNF fixes, and advances the native backend to
`2.8.1-7.fc46`. `input.json` pins COPR build 11240837, its source digest and the
existing project signing key. The package-source configuration for an older COPR
build does not identify this newer artifact's Git commit; the signed SRPM and its
nested source digests are the authoritative inputs here.

The download URL selects the signed SRPM from published repository metadata.
COPR's separate `srpm-builds` artifact is unsigned and has a different outer
digest, even though the extracted spec and source archive are identical. It is
recorded for provenance, but is not an accepted source for this signed-input
recipe. Keep both the published digest and the existing-key signature checks.

The original source archive and maintainer scripts remain unchanged. A small
additional archive contains the existing C++ `frontend.hpp` and ordinary-user
`frontend-request` companion from hardware main. It compiles against the exact
client library built by the preserved source. A byte comparison of `client.hpp`
guards the contract before compilation. No privileged method, raw shell entry,
authentication bypass or charging/storage/network policy change is added.

The backend owns the terminal companion and advertises
`uke-control(frontend-v1) = 1`. The public library and headers move to matched
`uke-control-client` and `uke-control-client-devel` packages. The SDK advertises
`uke-control(frontend-sdk-v1) = 1`, with an explicit header provide for reliable
DNF BuildRequires resolution. Building a frontend no longer installs the device
backend and its kernel-maintenance dependencies in the buildroot; the applications
still require that secured backend at runtime. Existing strict internal dependencies are aligned with
the new cohort's exact package versions. The backend source produces only
Control, TUI, tests, client library and devel SDK packages. It emits no desktop
meta or power packages.

`uke-kde-plasma-meta-1.0.0-37` requires `uke-control-gui-qt >= 1.0.0-3`.
The new canonical `uke-gnome-meta-1.0.0-37` requires `uke-control-gui-gtk >= 1.0.0-3`
and original Fedora GNOME session packages. Neither selection requires the other
desktop or the other frontend. GNOME has no activation scriptlet.

Run `bash tests.sh` in an isolated Fedora source worker to verify the signed
input, unchanged source/archive/scripts, dependency separation and SRPM assembly.
The `--nodeps` flag applies only to source RPM assembly; it does not qualify a
binary build or installation transaction.

## COPR build sequence

Use only `mcc45tr/uke-linux-unstable`, chroot `fedora-rawhide-aarch64`.
All Git source definitions must use the same reviewed immutable commit:

1. `uke-control`: source subdirectory `packaging/families/control-gui`.
2. After the backend/SDK is published, `uke-control-gui-qt`: `uke-control-qt`, and
   `uke-control-gui-gtk`: `uke-control-gtk`.
3. After both signed GUI RPMs and repository metadata are available, build
   `uke-desktop-metas` using source subdirectory
   `packaging/families/control-gui-metas`.
4. Verify normal fresh meta installation and an installed KDE meta upgrade
   against the final signed RPMs and updated repository metadata.

The last source produces only desktop selections/applet with mandatory GUI
dependencies. The first backend publication leaves installed/available meta
versions unchanged, avoiding an incomplete graph while GUI builds run. No
stable/testing promotion, release-image rebuild or physical device acceptance is
implied. Do not advertise `dnf update` availability before the complete graph is
published and solved against the real RPMs.

The published reference spec is retained as `uke-control-published-spec.txt`.
Its earlier `.spec` suffix caused COPR's dist-git source resolver to find two
package recipes and refuse build11246544 before compilation. Source-package
checks now require exactly one `.spec` member. Release7 also avoids reusing
the consumed release6 identity. The desktop selection retains the separately
published Turkish dictionary dependency and leaves the Core audit-tools update
outside this source phase. GUI packages provide and obsolete their earlier
`uke-control-qt` and `uke-control-gtk` names while retaining executable and
desktop-file identities.
