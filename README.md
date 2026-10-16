# Uke Linux hardware support

Platform packages and device integration for Xiaomi Pad 7 and POCO Pad X1
(`uke`, SM7675). All twelve components live in this repository as ordinary
folders, with their own sources and packaging rules.

## Status — 14 October 2026

Boot and USB debug packages are available through the development COPR. The
Fedora development installation reaches a software KDE desktop and Wi-Fi SSH.
Experimental native tools return motion and rear-light samples. Finger input
and ten simultaneous touch contacts worked in the 14 October tablet test.
Power and volume buttons and manual backlight control also worked in that test.
Backlight adjustment is linear and difficult to use comfortably; a logarithmic
curve is planned. Permanent desktop sensor support remains incomplete.
These are results from specific development tests, not a complete hardware
qualification. A folder in this repository does not imply a working package or
device feature.

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

KDE applications come from the distribution. They are not cloned, forked or
rebuilt as Uke applications. Fedora Python dependencies are allowed; desktop
package composition and tablet behavior still need separate validation.

## Checks and builds

Each component README lists its own available checks and package build commands.
These are source or host checks; passing them does not establish tablet support.
Run `make validate` here to run the offline checks of components with active
package or source recipes.

The kernel and Fedora image builder are pinned separately by the parent
`uke-fedora` repository. Recovery and UEFI development also have separate source
trees.
