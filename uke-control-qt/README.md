# Uke Control for Qt

A native Qt 6 Kirigami application for the existing Uke Control client. The
application, typed models, validation and device adapter are C++20; the presentation
uses Qt Quick/QML and original Kirigami/FormCard components. It respects KDE
appearance preferences and uses Breeze with KDE's desktop Quick Controls style.
There is no Python, WebView, replacement desktop service or privileged GUI.

The daily pages are Status, Power, Display & lights, Connections, Audio & sensors,
and Updates. Advanced provides searchable access to the complete installed action
catalog. Kirigami's page stack, responsive drawer and page actions provide
navigation; FormCard pages group related controls. The drawer becomes modal in a
narrow window, and choice controls use native selection dialogs. **Back** or
Alt+Left returns to the current area. Daily pages keep recovery and diagnostics
in Advanced. See the [Kirigami FormCard documentation](https://api.kde.org/org-kde-kirigamiaddons-formcard-qmlmodule.html).

Opening a page or guide does not query devices. Select **Read information**
or an inspection button to read. Requested defaults are drafts rather than current
measurements. **Details** reveals the full catalog explanation, parameter limits,
action identifier, access requirements and, during review, exact validated arguments.
Maintenance and external-publication warnings remain visible before confirmation.

## Build and run

Install a C++ compiler, CMake, Qt 6.6 or newer (Widgets, Qml, Quick and
QuickControls2), ECM, KDE Frameworks (IconThemes, ConfigWidgets, I18n 6.8 or newer),
Kirigami, Kirigami Addons FormCard 1.1 or newer, and the existing Uke Control package
providing `pkgconfig(uke-control-client)`, `client.hpp` and `frontend.hpp`.
On Fedora, development packages include `qt6-qtbase-devel`, `qt6-qtdeclarative-devel`,
`extra-cmake-modules`, `kf6-kwidgetsaddons-devel`, `kf6-kiconthemes-devel`,
`kf6-kconfigwidgets-devel`, `kf6-ki18n-devel` and `kf6-kirigami-addons-devel`.
Runtime integration uses `plasma-breeze-qt6`, `plasma-integration`,
`breeze-icon-theme`, `kf6-kirigami`, `kf6-kirigami-addons`,
`kf6-qqc2-desktop-style` and the original `konsole` for authenticated foreground
operations. The source-tree build also
finds the sibling `uke-control/build/libuke-control-client.so.1` when pkg-config
is unavailable.
Repository integration can explicitly select the current source SDK with
`-DUKE_CONTROL_SOURCE=/path/to/uke-control`; this takes precedence over the
installed headers and library. Standalone package builds use pkg-config.

```sh
cmake -S uke-control-qt -B uke-control-qt/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build uke-control-qt/build -j4
uke-control-qt/build/uke-control-qt
uke-control-qt/build/uke-control-qt --read-only --start battery.status
```

Run as the normal graphical session user. A root GUI is refused. `--help`,
`--version`, `--appearance-info`, and strictly isolated fixture sessions are
exceptions. `--appearance-info` prints the actual style class, resolved icons,
font, palette and Quick Controls style without constructing the frontend or querying any device. If Breeze
is absent, a native warning identifies the installed fallback style and the
missing package; the application does not imitate Breeze with a custom stylesheet.
`KIconTheme::initTheme()` and `KStyleManager::initStyle()` provide KDE's supported
startup behavior. `org.kde.desktop` is the default Quick Controls style unless
`QT_QUICK_CONTROLS_STYLE` is explicitly set. User appearance preferences remain effective.

## Reads and reviewed changes

Captured observations use the shared read-only `Query` on the Qt event loop.
Stop, Back, changing pages and closing the window cancel the owned read. Each
query retains its backend timeout and bounded stdout/stderr. Refresh only repeats
the last validated read; it cannot replay a mutation. Structured values, original
bounded output and errors have separate sections. Null, false, zero, negative values,
unavailable providers, cancellation, timeout and truncation remain distinct.
Control and directional override characters are rendered visibly. Large structured
views are bounded to 2,000 values and 16 levels; the raw bounded result remains
available.

Changes pass shared typed validation and open a separate native review dialog.
The requested values are visible before continuing. Package operations require
`APPLY`, maintenance requires its action ID, and publication requires its exact
repository, in addition to separate consent. Read-only sessions block changes.
The shared `frontend.hpp` adapter revalidates the ID and values, then opens
an original distribution terminal. Its small, unprivileged native
`frontend-request` wrapper revalidates the request and uses the existing
`run_terminal` path, with `sudo --` only when required. It uses no shell.
Authentication and all existing backend admission and interactive confirmations
remain in that terminal. The wrapper displays the exit status and waits for
Enter, preserving the result for inspection. Opening a
terminal establishes neither command completion nor hardware success. The GUI
never retries writes, refreshes them, or interrupts an independently handed-off
terminal operation when it closes.

## Simulation and tests

```sh
uke-control-qt/build/uke-control-qt \
  --fixture uke-control/tests/fixtures/tui.json --start charge.mode
ctest --test-dir uke-control-qt/build --output-on-failure
```

Fixture mode shows a persistent **SIMULATION** indicator and blocks every real
query, command, terminal handoff, mutation and receipt save. It accepts the existing
`org.uke.control.tui-fixture.v1` schema from a non-symlink regular file of at most
1 MiB. Results use known action IDs with `data` or `stdout`, optional `stderr`,
exit status 0–255 and delay 0–3,000 ms. Unknown fields and incompatible fields
are rejected. Missing results stay unavailable and never fall through to a real
query. No executable callbacks are supported.

The C++ Qt tests load the actual Kirigami pages and exercise typed values, separate errors, read refresh, unavailable
fixtures, cancellation, page/search/form navigation, native approval requirements,
read-only and simulation blocking, close cleanup, all action guides at 480 pixels,
and page cleanup after repeated navigation. They check scene placement, accessible
control names, native review refusal and QML runtime errors. The previous widget
view tests remain as migration regressions; that view is not linked into the
shipped executable. A separate native appearance suite
verifies the actual Breeze class and icons, explicit style overrides and KDE
font/palette settings; install the runtime integration packages before running it.
The shell test checks CLI
help/version, invalid fixtures and root refusal. Tests use the sibling TUI fixture;
source archives built against an installed client may disable tests with
`-DBUILD_TESTING=OFF`.

## Packaging and observed validation

`packaging/uke-control-qt.spec` builds the independent `uke-control-gui-qt` RPM against
the existing monolithic `uke-control` package. CMake installs the executable,
desktop entry and scalable icon using standard GNU installation directories.
It does not change the existing family package metadata.
The RPM additionally requires the package-owned
`/usr/libexec/uke-control/frontend-request` helper: an older package with the same
2.8.0 version may lack that addition. Building requires the package-owned
`/usr/include/uke-control/frontend.hpp` header and `pkgconfig(uke-control-client)`
2.8.0 or newer. Manually copied SDK files do not satisfy RPM dependency checks;
a `--nodeps` assembly build alone does not establish installability.

On 1 November 2026, the Fedora 46/rawhide software container built the application
with GCC 16.2.1 and Qt 6.12.0. The installed native Breeze plugin was
`plasma-breeze-qt6-6.8.2-1.fc46`. Native Xvfb screenshots were inspected at
1120×800 and 780×720, including charging preferences, simulated observations,
review blocking and an actual nonroot charging read. The actual read returned
`available: false`; this was displayed as provider unavailable. A staged CMake
installation was executed against the installed shared client.

Dependency verification on the same date built the current core RPM with normal
RPM dependency checks and `--nocheck`. A default-repository `dnf --assumeno install`
preview of the two GUI RPMs could not find the core package. Including the actual
local core RPM supplied the helper and shared-library providers, but resolution
still stopped on external `uke-kernel-update >= 1.8.7` and
`uke-kernel-update(transaction-v1) >= 1` requirements. The preview ran no
transaction or scriptlets; complete installation remains unverified.

Screenshots and logs stay under ignored `build/`. These are source, build and
synthetic desktop checks. No tablet, physical touch acceptance, charging cutoff,
display receiver, storage mutation or package transaction was tested.

On 2 November 2026, the local KDE redesign built with Qt 6.12, KDE Frameworks 6.31
and Breeze 6.8.2 in the disposable Fedora 46 container. The three Qt CTest suites
passed. The original distribution System Settings and KInfoCenter were run as
visual references; their code was not copied or modified. Real Xcb/Xvfb runs
used the KDE platform plugin as a normal user, with inspected wide and 480-pixel
views. Evidence remains under ignored `build-redesign/`. These desktop checks do
not qualify the interface for inclusion in a release image. Full package transaction and
physical-device acceptance remain subject to the limitations above.

The subsequent Kirigami migration supersedes that widget presentation, following
explicit approval for a QML presentation layer with C++ logic. Four CTest suites
cover the new bridge and actual QML pages as well as the retained migration
regressions. Source/runtime dependency checks find the real Kirigami, FormCard and
desktop Controls modules; the library version and QML import version are checked
separately because they are different version namespaces. The previous widget
sources and a complete local recovery patch remain available for comparison.
