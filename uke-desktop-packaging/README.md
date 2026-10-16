# Uke desktop package sources

This component contains decoration, theme and native runtime sources originally
prepared for Fedora Rawhide AArch64:

- `material-decoration`: optional upstream C++20/Qt6 KWin decoration and KCM.
  Source is pinned to the published upstream release in
  `manifests/material-decoration.json`, with archive SHA-256 verification both
  before SRPM generation and during RPM preparation. Its GPL/LGPL source license
  is retained. The small KPlugin metadata correction is attributed to the Nabu
  reference specification at `152980fa9a4fd2d71a2825efe10411c5a8a74da7`.
- `plymouth-uke`: an original MIT-licensed optional text theme using the current
  renderer dimensions. It does not assume a Uke panel DPI, enable itself,
  regenerate an initramfs, select a boot entry or copy Nabu geometry/artwork.

`make validate` checks the source contracts. `make srpm PACKAGE=NAME` creates
that family's source RPM. A source RPM or binary build does not establish
package acceptance. New findings are recorded separately from build and device
results.

Fedora's shared `powerdevil` and `plymouth` engines are dependencies. No measured
Uke issue currently justifies replacing them with Nabu-patched generic RPMs.
The source catalog retains those reference families for later Uke-specific work.
Python from ordinary Fedora packages is allowed in the target runtime.
Neither decoration compilation nor theme installation proves Uke graphics or boot.

The former [Python-free runtime variants](docs/NATIVE-RUNTIME.md) are retired.
Use the original Fedora `at-spi2-core`, `gstreamer1`, `libaccounts-glib`,
`libwacom` and `libstdc++` packages.

A [Mesa source candidate](packaging/mesa/README.md) corrects an observed
AArch64 llvmpipe SIGILL by passing Linux-reported CPU features to Gallivm.
It uses the complete pinned Fedora recipe with one source conditional changed;
KDE applications are untouched. Its package and physical Wayland checks remain
separate.

The [package testing procedure](docs/PACKAGE-TESTING.md) describes historical
Rawhide checks. Earlier Python-free transaction results do not qualify the new
package selection or Fedora 44. Use fresh package and device checks for either.

Project policy prohibits cloning, forking or rebuilding KDE desktop applications
as Uke variants. Plasma and Dolphin source targets are withdrawn, their automatic
COPR jobs are disabled, and the source allowlist rejects them before download.
Use original distribution KDE applications. Earlier source, helper and package
test records are historical evidence, not current runtime recipes.
