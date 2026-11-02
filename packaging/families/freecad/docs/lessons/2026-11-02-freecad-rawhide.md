# UKE-FREECAD-20261102: isolate the requested application in its own source family

Date: 2 November 2026.
Environment: isolated Fedora 46 x86_64 host container, official Rawhide AArch64
repository metadata, FreeCAD 1.1.4 release source. No tablet was accessed.

## Evidence and consequence

Fedora's FreeCAD retirement marker cites Python 3.12 installation failure and
FESCo retirement. Package review
[2386504](https://bugzilla.redhat.com/show_bug.cgi?id=2386504) remains open and
contains the newer 1.1.x source spec. The July comments discuss retired SoQt and
SIMVoleon dependencies. Current Rawhide AArch64 metadata supplies Coin4, Pivy,
PySide6, OndselSolver and E57Format. The absence of the main package cannot be
reported as evidence of an AArch64 compiler failure.

Use Elaine Gibson's review spec as a packaging baseline, update to the current
stable FreeCAD 1.1.4 source and pin its full release hash. The release archive's
SHA-256 is
`674f2c42dc060b71c8b052c70b04a9a995dde6cb868fe568615d3ff3f2717748`,
which matches the upstream checksum file. The release tag resolves to
`4fd3bf320d9566a27e60069fc8387448aaa3a094`.

Three upstream review patches apply without fuzz. They locate external E57Format
and fix TechDraw preference initialization and parallel FileInfo tests. The
main-branch external JSON/kdtree patch does not apply to stable 1.1.4; retain the
stable headers instead of forcing unrelated changes. Declare bundled libraries
and preserve the Artistic 2.0 and MIT license texts. Workbench Python remains
upstream functionality explicitly requested for this optional application; no
project-owned Python or early-boot payload is introduced.

The review spec contains an obsolete `-b1` setup argument and omits some direct
build requirements. Use the single rootless release archive, explicit Coin4,
VTK, Orocos KDL, Qt tools and PySide tools requirements, and enable the upstream
developer tests with the offscreen Qt platform. Keep binary build networking
disabled. Source downloads occur only while generating the source RPM and are
checked against the source lock.

## Observed validation

Two source RPM builds from pinned inputs produced identical SHA-256 results.
Source preparation applies all three upstream patches cleanly. An empty
AArch64 install-root dependency transaction resolves all BuildRequires from
official Fedora Rawhide repositories: 936 packages, 871 MiB compressed. This
was a solver-only transaction; no AArch64 program was executed on the host.

CMake configuration succeeds on x86_64 with GCC 16.2.1, Python 3.15.2,
Qt/PySide 6.12.0, Pivy 0.6.10, SWIG 4.5.1 and Coin 4.0.10. Its Pivy/SWIG runtime
check passes. That host result does not establish an AArch64 binary build or
functional tablet support. The RPM's CTest suite and metadata checks are gates
for the later native build, not claimed results at this stage.

The host FreeCADBase target also compiles and links with Python 3.15. Continuing
the generated CMake build directly initially omitted RPM_ARCH, which Fedora's
linker package-note configuration requires. Restoring the normal RPM environment
resolved that host-only invocation error without changing upstream code. The
remaining application and test targets were not built on the host.

## Channel boundary and next validation

FreeCAD is a new optional package only for unstable. Its dedicated family is
`packaging/families/freecad`, with all source, spec, checks and source-generation
rules kept together. It is outside the old 25-source channel-parity snapshot
and is not registered in the automatic `uke-*` family build hooks. Complete
the old final reforks before adding it to unstable, otherwise a bulk fork could
promote the untested application. Submit only `fedora-rawhide-aarch64`, inspect
native build/test logs, then solve the published RPM transaction against the
official Rawhide and unstable repositories. Device installation and GUI/CAD
testing remain unperformed until actual tablet evidence is available.
