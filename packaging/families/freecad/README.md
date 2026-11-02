# FreeCAD source family for Fedora Rawhide AArch64

This optional application is built from FreeCAD 1.1.4's upstream release source
for `mcc45tr/uke-linux-unstable`, `fedora-rawhide-aarch64` only. All FreeCAD spec,
source pins, checks and source-generation rules live in this separate `freecad`
family. Its single source package produces `freecad` and `freecad-data`, plus
RPM-generated debug packages. It is separate from the `uke-*` hardware families,
their automatic COPR hooks and release-image package selections. Testing and
stable promotion require separate test results and authorization.

## Source provenance

The spec is adapted from Elaine Gibson's
[Fedora package review](https://bugzilla.redhat.com/show_bug.cgi?id=2386504),
with the upstream stable release updated from 1.1.3 to 1.1.4. Fedora's previous
FreeCAD package was retired after its Python 3.12 installation failure
([retirement marker](https://src.fedoraproject.org/rpms/freecad/raw/rawhide/f/dead.package)).
That retirement does not establish a current AArch64-specific failure.

`source-lock.json` records the source archive, upstream commit, published
SHA-256, three upstream build/test fixes and the bundled JSON license text.
The source factory checks every downloaded file before creating the source RPM.
It does not consume an AppImage or a prebuilt application. The newer review's
external JSON/kdtree patches target main rather than the stable branch; this
package retains the stable branch's bundled headers and declares their licenses.
Bundled SMESH and zipios++ remain FreeCAD's maintained implementations.

FreeCAD embeds upstream Python and imports Python workbenches at runtime. The
user explicitly requested this optional application for the Fedora root. Its
dependencies are Fedora packages; no project-owned Python helper, hardware
service, recovery component, initramfs or installer payload is introduced.

## Source build

On a Fedora host with `rpm-build`, `make`, `curl`, `jq`, `cmake-rpm-macros` and
`python3-rpm-macros`:

```sh
make -C packaging/families/freecad validate
make -C packaging/families/freecad srpm
```

For a COPR SCM build, use:

| Field | Value |
| --- | --- |
| Project | `mcc45tr/uke-linux-unstable` |
| Type | Git |
| Clone URL | `https://github.com/MCC45TR/uke-linux-hardware-support.git` |
| Committish | Reviewed immutable commit containing this directory |
| Subdirectory | `packaging/families/freecad` |
| Spec file | `freecad.spec` |
| SRPM method | `make srpm` |
| Chroot | `fedora-rawhide-aarch64` |
| Build networking | Disabled |

The root `.copr/Makefile` dispatches this source directory separately. Binary
compilation uses Fedora libraries and does not fetch dependencies from the
network. Developer tests, desktop metadata and AppStream validation run during
the RPM build and must pass before publication is called successful.

## Validation boundary

On 2 November 2026, source RPM generation, reproducibility, clean patch
application and Fedora Rawhide AArch64 build-dependency resolution were checked
in an isolated Fedora 46 host container. The dependency transaction resolved 936
packages entirely from the official Rawhide repositories. An x86_64 CMake
configuration passed with GCC 16.2.1, Python 3.15.2 and Qt/PySide 6.12.0; SWIG/Pivy
runtime compatibility passed. This is a source/configuration result. A native
COPR build, published RPM dependency resolution and tablet testing remain separate.

The dedicated GitHub workflow checks only this family's source contracts and
does not submit COPR builds. The existing `uke-*` family registry is unchanged,
so its test/stable hooks do not pick up FreeCAD. See the family's
[lesson record](docs/lessons/2026-11-02-freecad-rawhide.md) for evidence and limits.

After a successful native build and repository verification, install with:

```sh
sudo dnf copr enable mcc45tr/uke-linux-unstable
sudo dnf install freecad
```

Review the transaction before accepting it. Launch `FreeCAD` or `freecad`.
