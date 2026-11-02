# UKE-QT-KDE-20261102: use native KDE appearance and responsive settings

Date: 2 November 2026.
Environment: isolated Fedora 46 x86_64 software container; GCC 16.2.1, Qt 6.12.0,
KDE Frameworks 6.31.0, Breeze and plasma-integration 6.8.2, Xcb/Xvfb.
Scope: local Qt frontend changes and tests. GTK, TUI, backend, authentication,
tablet, images and publication inputs are unchanged.

## Evidence and implementation

The earlier application could load the real Breeze widget style, but its large
custom headings, framed navigation and oversized repeated cards did not follow
the desktop's settings layout. The container also lacked KDE's platform-theme
integration. Merely assigning a style name did not establish KDE integration.

The distribution's original System Settings and KInfoCenter were installed and
run as visual references, without cloning, modifying or rebuilding either app.
Their frameless leading navigation, modest headings, native icons and inherited
window palette informed the Qt layout. The official
[KDE layout guidance](https://develop.kde.org/hig/layout_and_nav/) supports a
sidebar on wide windows and navigation on demand in narrow windows; it also
identifies QDockWidget for Qt Widgets sidebars.

The application now uses a native toolbar, frameless settings sidebar, grouped
daily actions and a bottom primary action. Expert tools remain searchable in
Advanced. Navigation becomes a native menu below 800 pixels and forms wrap below
660 pixels. Enlarged controls and pixel scrolling support tablet-sized layouts;
this is not physical touch acceptance. Empty search results, cancelled reads and
unavailable providers have explicit states. Back, page changes and closure still
cancel only the owned read. Review, exact confirmation tokens and existing
typed terminal handoff remain intact; the frontend adds no privileged method.

[KIconTheme::initTheme](https://api.kde.org/kicontheme.html) runs before
QApplication and [KStyleManager::initStyle](https://api.kde.org/kstylemanager.html)
after it. KDE/user preferences remain authoritative. CMake and RPM declarations
now include WidgetsAddons, IconThemes and ConfigWidgets; the runtime requires
the original Breeze style, plasma-integration and Breeze icons. No QML, custom
stylesheet, hard-coded font/palette or replacement desktop service was added.
The metadata-only `--appearance-info` path reports actual loaded appearance
without constructing a frontend, reading a fixture or querying a device.

## Validation and correction

The native build and staged CMake installation complete in the disposable
container. Three CTest suites pass: fixture/navigation/forms and safety gates,
CLI policy, and native appearance. All 126 action guides open within a 480-pixel
window without executing their actions. Tests cover the narrow navigation menu,
Escape, Back, repeated reads, cancellation, review refusal and inherited guards.
Appearance tests confirm the real Breeze class and ten resolved theme icons;
explicit Fusion, configured icon preferences, a custom KDE font and palette
remain effective. The metadata path also succeeds with a deliberately nonexistent
fixture and invalid action, proving it does not enter those client paths.

Xcb metadata and plugin traces show `Breeze::Style`, KDEPlasmaPlatformTheme6,
Breeze icons, Noto Sans 10, Window `#eff0f1` and Highlight `#3daee9`. The staged
executable is run as the normal laboratory user. Wide and narrow rendered
screenshots are inspected, including a simulated review and actual provider
unavailability. Fixture screenshots explicitly say Simulation and block all
device commands. Real reads in this container establish missing providers only.

Visual inspection found KMessageWidget text clipped after narrowing. Its native
[implementation](https://raw.githubusercontent.com/KDE/kwidgetsaddons/master/src/kmessagewidget.cpp)
sets a fixed height on show/layout requests, but normal width changes do not
always recompute it. A native probe measured 460 by 42 pixels while its own
height-for-width requested 60. A small local subclass queues that native height
calculation after a width change; the corrected label occupies both lines.
The regression test checks its height after resizing. No message text, palette
or style is replaced to conceal clipping.

Build logs, plugin traces, original KDE references and final rendered evidence
remain in ignored `uke-control-qt/build/` and `build-redesign/`. The widget
presentation is superseded by the Kirigami implementation recorded in the next
lesson. This work makes no COPR request or release-image update.

## Practical limits and next validation

The Xvfb session is not a complete Plasma desktop and has no document-portal FUSE
mount or physical devices. Successful build, layout, simulation and missing-provider
reads do not establish actual charging, display, touch, authentication or storage
behavior. Verify those separately in an authorized KDE tablet session.

The earlier full GUI RPM transaction blocker remains the core package's external
`uke-kernel-update` and `transaction-v1` requirements, as recorded in
[the earlier UI lesson](2026-11-01-control-interfaces.md). Dependency declarations
are parsed and their new KDE providers are verified locally, but no dependency
is stubbed or bypassed and no full RPM transaction is claimed here. Source
publication does not qualify a package transaction or release image.
