# SSC sensor packages

14 October 2026. Fedora Rawhide AArch64 preview sources:

- `libssc`: native SSC client library and diagnostic client.
- `iio-sensor-proxy`: upstream desktop D-Bus API with SSC enabled and the
  previously checked startup-order correction. A narrow SELinux module permits
  its own QRTR sockets.
- `uke-ssc-fileservice`: native FastRPC file transport with private RAM registry
  storage. Symlink escapes and incomplete shell inputs are rejected.

`make srpm PACKAGE=NAME` downloads the exact upstream archive below
`referances/`, verifies SHA-256 and packages the attributed patches. COPR binary
workers build without network access. Upstream C code retains its native build;
new protocol fixtures use C++ where applicable.

Upstream Meson uses Python during the build. The current source recipe pins
Python `3.15.0-1.fc46` and Meson `1.12.1-2.fc46`. Build dependencies do not
automatically become target dependencies.

The file-service package requires separately admitted device-local firmware
and configuration. Factory persist remains read-only; copies and any registry
updates stay in `/run/uke-ssc`. Calibration and proprietary data are not COPR
inputs. No unit is independently enabled by an install preset.

The optional `uke-adaptive-display` integration target supplies an explicit
activation path. See its [device trial guide](../../uke-desktop-integration/docs/ADAPTIVE-DISPLAY.md).
Physical orientation, front-light discovery, lux response, cold boot and
suspend/resume remain unqualified by package builds.

`make test-native` runs the unprivileged protocol/source fixtures. The separate
`make test-cache` needs root inside a disposable user namespace or container to
check root-owned tmpfiles semantics; it never mounts or accesses a real persist
partition. An ordinary user cannot perform those ownership operations.
