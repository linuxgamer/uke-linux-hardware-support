# Native Uke NFC and infrared access

4 November 2026. Version 1.1.0 adds a native read-only tag client and accompanies
the kernel capability `uke-kernel(nfc-ir-v2)`. The kernel enables FM19511 only on the admitted POCO
SKU and retains the exact Uke SPI infrared DT wiring.

The NFC interface is `/dev/fm-ntag`, a bounded FM19511 tag transport. The active
local graphical session receives access through logind when the device has the
`fm19511-uke` I2C ancestor. It remains a tag interface; NCI reader/rfkill APIs
describe a different capability. The fixed kernel ABI provides bounded memory
reads, cached raw IRQ status and RF-notification polling. It exposes no EEPROM
writer. The running kernel must support that ABI; installing a newer package
alone does not change the running kernel.

The native client provides these commands:

```sh
uke-nfc-tag info
uke-nfc-tag events
uke-nfc-tag wait 10
uke-nfc-tag read ADDRESS LENGTH
```

`info` reports the Type 2 role and available kernel operations. `events` reads
and acknowledges cached notifications; the three IRQ bytes remain undecoded.
`wait` returns a pending notification or waits once for 1..60 seconds (default 10).
It exits 3 on timeout. A notification does not establish a decoded NFC message.
`wake_capable` describes support, not whether wakeup is currently enabled.

Memory reads require an explicit unsigned decimal or hexadecimal address and
1..256 bytes without 16-bit address wrap. The returned maximum transfer may
further restrict a request. The client prints only that requested range as
JSON hexadecimal; it does not scan memory, infer capacity, save payloads or
write chip contents. Treat requested contents as private. Invalid arguments
exit 2 before opening the device. Provider/ABI/I/O faults exit 1 without an
automatic retry; an older running kernel reports the required update/boot.
An exclusive-open conflict is an error rather than a second tag connection.

The exact kernel header and its source identity are included in
`uapi-source.json`; its syscall exception is preserved separately. Application
code is C++17/MIT. No Python runtime, new hardware bus protocol, NCI reader,
card emulation or IR learning path is added.

The infrared transmitter uses the standard `/dev/lircX` whose SPI ancestor is
`ir-spi`. Logind grants the active local session access. The distribution's
`ir-ctl` utility supplies standard LIRC transmission and capability queries.
Inspect its `--help` and select the transmitter's actual device node before
sending the intended remote-control code.

Both rules match the specific native providers. Package updates reload rules
and request change events for the corresponding misc/LIRC nodes. Installation
does not send infrared codes or modify NFC tag contents. Source and udev rule
checks remain separate from optical/RF hardware acceptance.

Release 1.1.0-2 explicitly sets mode 0660 for the exact NFC provider before
session ACL handling. A pre-existing named-user grant can remain ineffective
when its ACL mask is zero; an unchanged grant does not make systemd recalculate
that mask. The explicit mode restores the mask. Other users receive no access,
and logind still controls the named-user grant and its revocation. The kernel's
restrictive default and read-only API are unchanged. The package's COPR check
reproduces this failure and repair on temporary-file ACLs without opening NFC.

The 1.1.0 source requires remote client ABI/bounds/fault/poll checks and an
AArch64 package build. It is not installed or physically qualified by those
checks. Use an external compatible reader for the tablet's tag role and a
separate optical/receiver test for the IR transmitter.
