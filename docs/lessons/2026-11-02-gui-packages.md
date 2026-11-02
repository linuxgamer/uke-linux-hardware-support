# UKE-GUI-PACKAGES-20261102: publish matched SDK, GUI and desktop dependencies

Date: 2 November 2026. Environment: isolated Fedora 46 software container;
source inspection and AArch64 SRPM preparation, not tablet acceptance.

The current signed production source is COPR build 11240837,
`uke-control-2.8.1-5.fc46`, SHA-256
`44755d1c58f919bcd76c2a13c6dabefe2cf574464542bfa58fb29309fca5881a`.
Its outer spec contains the Control, desktop metadata and power packages. Its
nested source digests preserve the newer DNF fixes. The older COPR package source
configuration points at an earlier Git recipe and does not identify this newer
build's Git commit. Do not replace it with main's older standalone Control recipe.

The published binary has the client header/library but lacks `frontend.hpp` and
the ordinary-user `frontend-request` companion required by both native GUIs.
The published client header exactly matches hardware main. The isolated
`packaging/families/control-gui` recipe imports the signed production source,
compares its digest and existing project-key fingerprint, and preserves its
source archive and all existing maintainer scripts byte for byte. It adds only
the existing typed frontend contract, aligned internal package dependencies and
the required desktop selections. Backend source algorithms are unchanged.

The candidate advances Control to `2.8.1-6.fc46` and desktop selections to
`1.0.0-36.fc46`. Public client library and devel subpackages carry the shared SDK;
this lets GUI buildroots install the SDK without device/kernel-maintenance
runtime dependencies. Applications still require the secured backend and its
`frontend-v1` capability. Explicit header/helper provides avoid dependency on
optional filelists metadata. No compatibility capability is advertised without
its actual payload.

KDE requires `uke-control-qt >= 1.0.0-2`; the new canonical GNOME selection
requires `uke-control-gtk >= 1.0.0-2`. Both requirements are mandatory, and each
selection avoids the other desktop/frontend. The existing optional Plasma
applet remains separate. The GNOME selection adds no activation scriptlet.

Independent GUI source recipes produce `uke-control-qt-1.0.0-2.fc46` and
`uke-control-gtk-1.0.0-2.fc46` SRPMs. Qt's simulation fixture is now included in
its source archive so RPM tests do not rely on an adjacent backend checkout.
Source preparation, typed contract tests, signed-input checks, unchanged
archive/script comparisons, dependency separation and four Qt suites pass in
the laboratory. This does not qualify a native AArch64 binary build or a normal
DNF transaction; those require the published COPR results.

Build the backend/SDK cohort first, then both GUI packages using the same
immutable source commit in unstable Rawhide AArch64. Do not claim installation
availability until signed GUI RPMs and repository metadata satisfy fresh and
installed-meta solver checks. Existing legacy channel-transition failures and
the backend's kernel-update compatibility requirement remain separate; neither
is bypassed by this packaging work. No stable/testing promotion or image rebuild
is part of GUI publication.
