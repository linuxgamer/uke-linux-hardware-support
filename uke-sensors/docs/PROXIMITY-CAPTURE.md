# Bounded proximity transport comparison

23 October 2026. Uke / SM7675, Fedora AArch64. This optional native diagnostic
does not change SensorProxy's decoder or claim calibrated proximity support.
It requires root and one physical, available, on-change default proximity
endpoint. Each capture has a ten-second, 64-datagram and 32-event limit after
bounded metadata discovery. Closing its socket removes that client's request.

| Mode | Report | Delivery | Request body |
|---|---|---|---|
| `passive` (default) | Small | Non-wakeup | Empty payload, passive flag |
| `active-small` | Small | Wakeup | Payload absent |
| `active-large` | Large | Wakeup | Payload absent, matching libssc |
| `active-empty` | Large | Wakeup | Empty payload explicitly present |

Active modes temporarily enable the default sensor with request 514. They set
no sample rate and write no calibration, registry or persist data. Passive
mode follows Qualcomm's non-wakeup contract and does not activate a sensor.
The selected SUID, client and peer must match before any event is recorded.
Output contains bounded raw payloads and must remain private. It excludes
envelope identifiers and does not reinterpret unknown floats as near/far or distance.

Capture one mode at a time, keeping the physical conditions unchanged:

```sh
umask 077
sudo uke-proximity-capture active-small > proximity-small.log
sudo uke-proximity-capture active-large > proximity-large.log
sudo uke-proximity-capture active-empty > proximity-empty.log
```

The tool reuses the project's MIT native framing/capture source. The wire
contract is Qualcomm sensinghub `329551963bcc063f6a89f6cbca259c5b5c9c4e0b`,
`sns_client.proto`, `sns_std.proto` and `sns_suid.proto`. Libssc 0.4.4 supplies
the comparison request, not a portable decoder for an unknown vendor payload.
Build and framing fixtures remain separate from physical near/far validation.
