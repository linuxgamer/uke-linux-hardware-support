# Display controls and raw front-color data

## USB-C DisplayPort profiles, 24 October 2026

`sudo uke-control display lane-limit 1|2|4` sets the matching kernel's
disconnected-only training ceiling. It resets to four at driver initialization
and never changes the Type-C pin assignment. The resulting link remains bounded
by the firmware route, source, sink and repeaters. In particular, selecting four
cannot turn JH925's observed two-lane route into four lanes. `display profiles`
reports this ceiling separately from the observed link and standard pin control.
Disconnect the USB-C display before changing it; an attached or starting stream
returns `EBUSY`. A successful readback confirms policy, not physical link training.

`uke-control usb status` includes negotiated device speed and runtime power,
autosuspend, wakeup and persistence attributes. To prevent runtime autosuspend
for one external USB device under Uke's controller, use
`sudo uke-control usb power 2-1 performance`; restore kernel-managed autosuspend
with `sudo uke-control usb power 2-1 auto`. Discover the current device name in
the status output. These one-shot settings neither force USB generations nor
change charger/PD policy; they reset on device removal. System sleep remains
separate from runtime autosuspend.

`uke-control ethernet status IFACE` queries the original Fedora `ethtool` for
the RTL8153's link/advertisement, offloads, EEE, pause, rings, coalescing and
channels. Unsupported operations report their error, without invented values.
The cached section includes RX/TX errors and drops. Use
`sudo uke-control ethernet profile IFACE auto|1000-full|100-full|10-full` to
restrict auto-negotiation to supported modes. The ethtool value/mask changes
only copper speed bits; pause and other advertised
features are preserved. First disconnect that interface
through the original NetworkManager controls and bring it administratively
down; an up interface is refused. Reconnect afterwards. Driver readback checks
advertised modes, while cable/peer negotiation and throughput need a separate
physical test. Keep Wi-Fi available. These settings are not saved as a
NetworkManager connection profile. Advanced EEE/offload controls remain with
the original `ethtool`; this release inventories them and preserves their values.

Use `uke-control display profiles` to list profiles, the standard Type-C
selection interface and the active DP driver's cached lane count, rate and
wire bits per pixel. A missing cache, unsupported version, inactive stream or
invalid data remains `null`. These are sequential software observations;
they do not establish a visible image or working USB devices.

| Profile | Requested DP lanes | USB and display contract |
| --- | --- | --- |
| `usb3-display` | 2, pins D/F | Reserve lanes for USB3 alongside DP. |
| `display4` | 4, pins C/E | Prioritize DP bandwidth, including direct monitor cables. USB2 uses separate pins; dock USB/Ethernet fallback needs physical testing. |
| `dual-display` | 4 plus MST | Unavailable until the Uke driver implements MST stream/connector management. |

Request an available standard profile with
`sudo uke-control display profile display4` or
`sudo uke-control display profile usb3-display`. This is a one-shot request;
the command stores no boot preference. It accepts only an active, uniquely
associated DP partner's advertised pin assignments. A kernel write acceptance
and a matching pin cache are reported separately from link training and
physical validation. Attached storage, active USB networking, a bound gadget,
ambiguous ports or a changed attachment refuse the transition. Changing lanes
can interrupt video and SuperSpeed devices; retain another management path.

On POCO Pad X1 running 7.2.12-31 with Juo JH925, the firmware reports no UCSI
alternate-mode details or override feature, and no standard DP alternate-mode
control appears. Requests therefore return `DP_ALTMODE_CONTROL_UNAVAILABLE`.
The existing firmware-negotiated display/USB path is retained. This failure
does not establish that automatic four-lane negotiation with a direct DP cable
is impossible. No forced PHY/mux setting or raw firmware command is used.

Resolution, refresh rate, HDR, color depth and Adaptive Sync remain under the
distribution's original KDE controls and actual DRM/monitor capabilities.
A lane profile cannot manufacture those capabilities. Four HBR2 lanes provide
17.28 Gbit/s of payload: a 594 MHz 4K60 RGB8 stream fits (14.256 Gbit/s), while
RGB10 at the same timing needs 17.82 Gbit/s. Two such RGB8 4K60 streams also
exceed that link. These calculations are limits, not Uke hardware acceptance.

Current physical evidence covers one 2560x1440/75 Hz monitor, mouse, DP audio
and a Gigabit Ethernet link on the two-lane path. Actual HDR, external VRR,
four-lane USB/Ethernet fallback, two external monitors and DDC/CI brightness
remain unqualified. The cached stream ABI requires a matching newer kernel;
older kernels report `observed_link: null`.

## VRR minimum, 24 October 2026

On Uke / SM7675, use `sudo uke-control display vrr-min status` to read the
loaded minimum, saved minimum and whether a reboot is required. Save a minimum
with `sudo uke-control display vrr-min set 30` (or another integer from 30 to
143 Hz, below the currently selected mode). Select the 144 Hz mode in the
distribution's original KDE display settings for the full 30–144 Hz range.
KDE continues to control whether Adaptive Sync is enabled.

The command requires the kernel's O82 VRR floor API version 1 and an active,
VRR-capable internal display. Unsupported kernels refuse the write. The saved
policy takes effect when MSM loads on the next boot; the command does not
restart the desktop. The packaged loader appends the validated value after
firmware defaults. Without a valid saved policy or the driver API it preserves
normal module loading. The managed file is
`/var/lib/uke-control/vrr-min`. It contains only the numeric policy; no module
options leak into older rollback kernels that lack the required API.

The setting exposes the driver's range; it does not certify touch stability at
every minimum. Each minimum requires separate physical validation. A 1 Hz
minimum is not supported.

## Cached external-display diagnosis, 20 October 2026

`uke-control usb status` includes standard Type-C alternate modes and cached
UCSI, QMP PHY and WCD AUX routing attributes. The same snapshot is retained in
private device reports. The UCSI diagnostic requires the kernel's optional
`uke_transport_state` attribute; older kernels report null for that field.
It exposes transport service/request generations, quarantine, request/reply/
completion/notification boottime timestamps and errors. A timestamp describes
a driver event, not firmware sample time or port-state freshness. A missing
firmware error code remains explicitly unknown in the versioned raw record.

Only named bound drivers and fixed cached attributes are read. No GLINK command,
I2C access, EDID collection, role request, lane switch or modeset is performed.
Route completion, alternate-mode activity and connector status remain separate
observations. Link-training and visible-image verification remain null; monitor
hotplug/reconnect and actual output require their own physical test.

## Original front-color source record

Date: 2026-10-18. Environment: isolated source candidate on app5 source
`2c37e245accc1da836e003044a96f3df74c2abe5`. No test, compiler, package build,
installation or device operation was performed by this source workstream.

`uke-control display sensor` reads the optional native front-color snapshot
at `/run/uke-sensors/front-color.json`. It never starts a service or SSC request.
The separately packaged `uke-front-color` publisher and its activation remain
the sensor component's responsibility. Source pin
`b8f5f32711272b918056cbbc2204071b333fb50a` defines the exact publisher schema
`org.uke.sensors.front-color.v1`. See the sensor component's `FRONT-COLOR.md`.

The reader opens trusted root-owned 0755 runtime directories without symlink
traversal, then a root-owned regular 0644 single-link file. Reads are bounded
to 4096 bytes. It rejects duplicate/unexpected fields, wrong schema/version,
wrong types, malformed vectors, nonfinite/out-of-float-range numbers,
inconsistent availability and future timestamps. It reuses the existing
libsystemd JSON API; no new parser dependency, Python or SSC source copy is added.

CLI output uses `org.uke.control.front-color.v1` and retains `source_schema` on
a valid source record. It independently recalculates `fresh` and `available`
using CLOCK_BOOTTIME: publication must be at most five seconds old and a sample
at most 30 seconds old. It adds `publisher_alive`, `reader_boottime_ns` and
`reader_reason`. Stale raw values remain visible as unavailable. A stable
on-change value can age without representing a sensor fault; the reader sends
no request to refresh it. A missing/untrusted/malformed snapshot returns an
unavailable status with a fixed reason and no values.

DSP tick frequency, clock mapping, units and calibration remain unverified.
Raw numeric values are not relabelled Kelvin, and wire status 3 is not a
calibration certificate. This interface does not set the
desktop white point, replace SensorProxy's ALS/rotation interface or invent
an IIO/sysfs device.

The actual JSON-reader fixture is prepared in `tests/front-color.cpp` and
integrated into `make check`. It covers valid data, exact recency boundaries,
dead publication, stable stale samples, unreliable status, missing/duplicate
fields, false availability, future times, invalid vectors and fixed unverified
clock/unit rules. It has not run in this source workstream. The coordinator
must run host/native checks and separately qualify package/SELinux, explicit
publisher activation, ordinary-user reading, stop/crash invalidation and physical
color response. A source or fixture result does not establish those device gates.

The development display observer labels the configured mode as `VRR upper limit`
while VRR is enabled and shows `Measured DPU` separately. For example, a 144 Hz
configured mode can coexist with a 30 Hz measured vblank cadence. VRR changes frame
timing inside that mode; the selected-mode label need not follow each interval.
This measurement keeps interrupts referenced and is not optical panel metrology.
