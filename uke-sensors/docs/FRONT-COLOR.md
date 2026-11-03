# Raw front-color endpoint

23 October 2026 packaging correction: families 1.0.7 and 1.0.8 selected the
retained local source RPM release 3 after producing a newer source RPM. Thus
front-color releases 10/11 did not contain the reviewed wakeup changes or phase
logs. Their warm samples do not validate that source candidate. Composition
now uses a fresh private output directory, accepts exactly one generated RPM
and compares all publisher/helper/test sources inside it with the working tree.
Sensors 1.0.9 delivers the current source as front-color 1.0.2-12. A subsequent
normal boot must qualify automatic readiness.

23 October 2026 startup correction: a recorded normal boot of `1.0.2-10` fails
with a transport timeout at 12.357 seconds and publishes no sample. Warm-session
reception did not establish automatic readiness. Attribute request 1 now also
uses the exact OEM APSS/wakeup contract; requests 513/514 remain non-wakeup.
Four bounded startup phase messages distinguish discovery from attribute
admission without raw identities or values. A new boot must test this candidate.
Static comparison uses the same exact HAL below: `sensor_factory::create_attr_request`
at `0x54c98` sets delivery 0 and processor 1, confirmed by the API serializer.

23 October 2026 correction: release 8 remains in `starting` for more than four
minutes after a normal Uke boot; one later publisher restart receives a sample.
Keeping the subscription alone therefore does not qualify automatic readiness.
The publisher now matches exact OEM SUID discovery delivery: request 512
uses APSS/wakeup, while measurement requests 513/514 remain non-wakeup. No
resend, startup delay or DSP restart is added. A new boot must test this candidate.

Static comparison uses OS3.0.304.0 `libqsh.so` SHA-256
`b7a6e9762119e16fe36b1cce391dc0461bb51058ac9aea839126763347cae663`,
`suid_lookup::request_suid` at `0xc860`. The exact `libsnsapi-full.so` serializer
at `0x17a438` confirms the delivery/processor fields. Original libssc 0.4.4
likewise selects wakeup delivery. Proprietary code is read only and is not
executed or included; the shared enum contract is the pinned Qualcomm API below.

23 October 2026: the publisher retains its existing SUID-update subscription
while the endpoint initializes. An empty initial lookup no longer becomes a
permanent failure after 30 seconds. Unavailable `starting` snapshots continue
at most once per second; no sample is invented and no request is resent.
Cancellation, the 64-event bound, transport limits and metadata admission remain.
Late readiness still requires a separate cold-boot tablet result.

18 October 2026. Release 1 is installed on Fedora Rawhide AArch64 / Uke and
publishes raw `cct_front` samples through event 1025 after on-change request 514.
The kernel-25 boot instead reports metadata unavailable; an explicit later
restart receives a sample. This does not establish automatic data readiness,
units, spectral calibration or persistent operation.

Release 2 waits for SUID readiness updates before admitting metadata and
collection. It uses the pinned Qualcomm sensinghub
`329551963bcc063f6a89f6cbca259c5b5c9c4e0b` `sns_suid.proto` subscription,
retains all matching endpoints and refuses ambiguity. That release's 30-second
discovery limit is superseded by the readiness change above; the 64-event bound
remains. It does not resend requests or restart a failed DSP.
The metadata reply must contain event 128 and the existing type, physical,
availability and on-change checks still apply. A failure logs one reason
without raw identifiers. Host framing/state/file and empty-to-ready endpoint
fixtures pass; native packaging and a normal tablet boot remain separate gates.

The optional `uke-front-color` package publishes one local, read-only snapshot:

```text
/run/uke-sensors/front-color.json
```

It does not replace original SensorProxy, which remains the ALS/rotation
interface. It creates no IIO/sysfs device, lux conversion, kelvin value, color
management policy or desktop application variant. No Python, firmware,
calibration bytes, SUIDs or other device identifiers are included.

## Activation and lifecycle

The service is separately packaged and disabled by its install preset. Package
installation and metadata/status readers do not activate continuous collection.
After a separately authorized package/firmware and SELinux check, activation is
explicit:

```sh
sudo systemctl enable --now uke-front-color.service
```

`uke-ssc-sensors.service` readiness precedes the publisher. The publisher binds
to that service's lifetime; its opt-in dependency is restored on a resource
restart. It also checks the Uke / SM7675 compatible strings itself. Exactly one
`cct_front` SUID must advertise the expected datatype, available and physical
flags, and on-change stream type. Only then does this explicitly started
publisher send one empty request 514 on its own persistent non-wakeup QRTR
connection. It does not change calibration, registry, sensor rate or persist.

Normal stop closes the connection and removes the snapshot. Transport, SSC
server removal/replacement, malformed data or metadata failure invalidate the
sample. The publisher removes a previously live snapshot immediately and writes
an empty error snapshot within the one-update-per-second limit. It then waits
without retrying or restarting a failed DSP. A resource/service restart is
required to retry; there is no elapsed-time startup delay or restart loop.
Power, restart and suspend/resume behavior require separate tablet qualification.

Disable collection with:

```sh
sudo systemctl disable --now uke-front-color.service
```

## JSON version 1

The schema string is `org.uke.sensors.front-color.v1`. The snapshot is bounded to
4096 bytes and replaced atomically no more than once per second. The runtime
directory is owned by root and is mode 0755; the regular snapshot is mode 0644.
The publisher checks directory traversal permissions as well as ownership.
Readers can inspect raw color values without privileges. This public directory
is independent of the private mode-0700 `/run/senemos-uke` USB gadget state;
enabling CDC must not make the sensor endpoint inaccessible.

| Field | Type and meaning |
|---|---|
| `schema`, `version`, `data_type` | Fixed schema string, integer 1, and `cct_front` |
| `connected` | The current metadata-qualified connection acknowledged request 514 |
| `has_sample` | A validated raw vector is retained from this connection |
| `fresh` | Connected sample received within `freshness_window_ns` of publication |
| `available` | Connected, fresh sample with wire status 1–3 and no error |
| `error` | Empty string, `starting`, `awaiting_sample`, `metadata_unavailable`, `transport_unavailable` or `invalid_sample` |
| `values` | Raw finite float array, 1–16 entries when a sample exists; otherwise empty |
| `status` | Actual SSC wire status 0–3; null without a sample |
| `timestamp_ticks` | Actual nonzero SSC timestamp; null without a sample |
| `dsp_clock_hz` | Null: this endpoint's tick frequency is unverified |
| `dsp_clock_mapping` | `unverified`; no conversion or host-clock synchronization is implied |
| `received_boottime_ns` | Host `CLOCK_BOOTTIME` receipt time in nanoseconds; null without a sample |
| `published_boottime_ns` | Host `CLOCK_BOOTTIME` snapshot publication time in nanoseconds |
| `freshness_window_ns` | 30000000000: a 30-second receipt-recency window |
| `units`, `calibration` | Both `unverified` |

Status 0 is retained as an unreliable raw observation with `available=false`.
Status 3 is not a calibration certificate. An on-change value can remain stable
without a new event; after 30 seconds it is marked stale for consumers, without
inventing a sensor fault or requesting a new sample. Stale values remain visible
with `has_sample=true`, `fresh=false` and `available=false`. A transport/reset
discontinuity clears values, status and timestamps entirely.

A reader must use a bounded regular-file read, verify the root-owned directory
and file, reject symlinks and unexpected schema/types, and recompute freshness
using its own `CLOCK_BOOTTIME`. Reject future receipt/publication times, a
publication older than five seconds, or a receipt older than the advertised
30-second window. This also invalidates an abrupt publisher death; a cached
Boolean alone must never establish liveness. Host timestamps include suspend
time and must not be compared with wall time or the unverified DSP clock.

## Source and validation boundaries

The implementation reuses the existing native SSC client/parser from support
commit `2676cdd47dc33da661315091f599587396e3b288`. The active Uke HAL uses 1025;
the generic `sns_cct.proto` event 882 is not substituted. Qualcomm's pinned
[client API](https://github.com/qualcomm/sensinghub/blob/329551963bcc063f6a89f6cbca259c5b5c9c4e0b/apis/proto/sns_client.proto)
leaves sensor event timestamps sensor-specific. A generic libssc 19.2 MHz
comment does not independently establish the CCT endpoint's clock frequency.

Each datagram is limited to 16 KiB, each client event envelope to 32 entries,
each sample vector to 16 finite floats, and receipt rate to 64 datagrams per
second. Validate QMI peer/client identity, selected SUID, message ID, status and
strictly increasing nonzero timestamps. Discovery and acknowledgement have
five-second/64-packet bounds. No raw payload or sensor identifier is logged.

`make test-front-color` compiles the publisher and runs isolated host fixtures
for metadata/type refusal, sample framing, malformed values, status, stale and
reset state, atomic replacement and symlink/hardlink/permission refusal.
`make front-color-srpm` creates the separate source RPM. Host checks, COPR,
package payload/dependency checks, enforcing SELinux, explicit activation,
continuous reception, stop/crash invalidation and physical color response remain
coordinator acceptance gates. None is inferred from source preparation.
