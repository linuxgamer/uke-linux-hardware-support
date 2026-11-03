# Uke Control GTK

Native C++20 GTK 4 / libadwaita frontend for the shared Uke Control client.
The `uke-control-gui-gtk` RPM retains the `uke-control-gtk` executable and
provides the previous package name for upgrades.
The application constructs native widgets directly. It has no Python, webview,
QML or embedded desktop application dependency.

The daily navigation contains Status, Power, Display & lights, Connections,
Audio & sensors and Updates. Advanced provides searchable access to the complete
shared action catalog. Everyday tasks show a short explanation; technical IDs,
access requirements and full limitations remain available in an expander.

## Build and run

Requirements: C++20 compiler, CMake 3.20, GTK 4.12+, libadwaita 1.5+,
JsonCpp and the Uke Control 2.8.0 client headers/library/pkg-config metadata.
The distribution's `gnome-terminal` or `konsole` is needed for terminal handoff.

```sh
cmake -S uke-control-gtk -B uke-control-gtk/build
cmake --build uke-control-gtk/build -j4
uke-control-gtk/build/uke-control-gtk
```

For an uninstalled sibling client build:

```sh
cmake -S uke-control-gtk -B uke-control-gtk/build \
  -DUKE_CONTROL_SOURCE="$PWD/uke-control"
cmake --build uke-control-gtk/build -j4
```

Run the application as a normal session user. A root GUI is refused. Optional
arguments are `--read-only`, `--start ACTION` and `--fixture FILE`.
`--start` opens a task without executing it. No observation starts just by
opening a category or task. The sidebar button opens navigation in a narrow
window. Escape returns; Ctrl+Q closes the application.

## Observations and changes

Captured observations use the shared single-owner asynchronous `Query` API.
Stop, Back, category navigation and window closure cancel the owned read.
Refresh starts another read only. Nothing automatically refreshes or retries.
Results preserve provider errors separately from standard output and distinguish
completion, cancellation, timeout, missing data and truncated output. A provider
that reports `available: false` is explicitly unavailable even with exit status
zero. Native result rows retain null/unknown, false, negative numbers and zero.
The raw expander retains captured evidence; the native view displays at most
48 values. Session results stay in memory and are not exported automatically.

Choice, bounded numeric and text controls come from public client parameters.
Form defaults describe requested settings. Inspect and Preview open the related
observation while preserving the draft and transferring matching parameters.
Every change requires a separate native request review and Continue action.
Package changes require `APPLY`, maintenance requires its exact action ID and
publication requires the exact repository. Destructive storage warnings remain
visible in review. The public `frontend.hpp` helper revalidates the action ID and
typed values before handing the existing command to a distribution terminal;
root backend commands use `sudo --` there. The GUI never runs as root.
A launched terminal is reported only as a handoff. Backend questions,
authorization, freshness checks and original storage guards still apply.
Inspect current state afterward; a launch does not establish operation success.

Read-only and fixture modes permit typed plan review but disable terminal
continuation, including after typing the correct confirmation. The callback
independently checks these modes before any launch.

## Simulation and checks

```sh
uke-control-gtk/build/uke-control-gtk \
  --fixture uke-control/tests/fixtures/tui.json
ctest --test-dir uke-control-gtk/build --output-on-failure
uke-control-gtk/tests/gui-smoke.sh
```

The fixture adapter accepts `org.uke.control.tui-fixture.v1` from a bounded regular
file and rejects symlinks, duplicate keys, unknown action IDs, invalid result
types, excessive delays and oversized output. The visible SIMULATION banner
stays present. Fixture mode never constructs a Query, executes a command,
opens a terminal, saves a receipt or contacts a device. Missing fixture actions
produce an explicit unavailable result. No executable fixture callbacks exist.

`tests/gui-smoke.sh` requires a normal user, Xvfb, xauth and dbus-run-session.
It exercises real widgets, asynchronous fixture results, typed choices,
inspection/preview navigation, cancellation, missing data, separate errors,
native request review and the simulation confirmation gate.
`tests/visual-check.sh` runs inside an existing nonroot Xvfb/DBus session and
also requires xdotool and ImageMagick. It saves screenshots below `build/` and
performs one real captured charging-status read. This check is intended for an
isolated host laboratory where the charging provider is unavailable. It never
runs a mutation or terminal handoff.

## Verification record

2026-11-01, isolated Fedora Rawhide container, x86_64, GCC 16.2.1,
GTK 4.24.1, libadwaita 1.10.0, JsonCpp 1.9.6, shared client 2.8.0:

- Development and installed pkg-config builds passed.
- Fixture/parser and confirmation safety CTest passed.
- Native Xvfb fixture smoke passed as UID 1000.
- Native live Query reported charging provider unavailable (`available: false`),
  independently of simulation; no mutation was executed.
- Wide and 700-pixel layouts were rendered and inspected. Screenshots remain
  ignored build evidence, not a claim of physical touch or tablet acceptance.
- The standalone x86_64 RPM built with `rpmbuild -bb --nodeps`; its embedded
  CTest and desktop validation passed. `--nodeps` was necessary because the
  matching client SDK was manually installed in the laboratory rather than
  registered in its RPM database. Payload inspection found the native binary,
  desktop entry, SVG icon, documentation and license, with no Python payload.

The RPM spec is a separate optional frontend package. No tablet, root backend
mutation, terminal authentication flow, package transaction, storage operation,
physical touch interaction or hardware function was tested. Host portal/FUSE,
PipeWire and software-rendering warnings reflect the isolated display laboratory.
