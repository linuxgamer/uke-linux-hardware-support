# Uke Linux hardware support

Platform packages and device integration for `uke`. All twelve components live in this repository as ordinary folders, with their own sources and packaging rules.

## Components

| Folder | Purpose |
|---|---|
| [uke-core-meta](uke-core-meta) | Console package selection |
| [uke-desktop-metas](uke-desktop-metas) | Desktop package policy |
| [uke-desktop-packaging](uke-desktop-packaging) | Native libraries, theme and decoration plugin |
| [uke-orangefox-packaging](uke-orangefox-packaging) | Versioned recovery images managed by DNF |
| [uke-boot](uke-boot) | Boot and USB debug integration |
| [uke-platform-runtime](uke-platform-runtime) | Native platform runtime |
| [uke-sensors](uke-sensors) | Sensor discovery and native tools |
| [uke-camera](uke-camera) | Camera integration work |
| [xiaomi-uke-firmware](xiaomi-uke-firmware) | Firmware sources, licenses and file identities |
| [uke-desktop-integration](uke-desktop-integration) | Desktop integration |
| [plasma-uke-kcm](plasma-uke-kcm) | Planned device settings panel |
| [uke-hardware-provenance](uke-hardware-provenance) | Hardware source records |

Unlike upstream, KDE applications come from the distribution and Fedora Python dependencies are allowed.

## Checks and builds

Each component README lists its own available checks and package build commands.
These are source or host checks; passing them does not establish tablet support.
Run `make validate` here to run the offline checks of components with active
package or source recipes.

The kernel and Fedora image builder are pinned separately by the parent
`uke-fedora` repository. Recovery and UEFI development also have separate source
trees.
