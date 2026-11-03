# Native Uke NFC and infrared access

3 November 2026. This package accompanies the kernel capability
`uke-kernel(nfc-ir-v1)`. The kernel enables FM19511 only on the admitted POCO
SKU and retains the exact Uke SPI infrared DT wiring.

The NFC interface is `/dev/fm-ntag`, a bounded FM19511 tag transport. The active
local graphical session receives access through logind when the device has the
`fm19511-uke` I2C ancestor. It remains a tag interface; NCI reader/rfkill APIs
describe a different capability. The driver's RF-event polling and bounded
read/write interface retain their existing protocol and removal handling.

The infrared transmitter uses the standard `/dev/lircX` whose SPI ancestor is
`ir-spi`. Logind grants the active local session access. The distribution's
`ir-ctl` utility supplies standard LIRC transmission and capability queries.
Inspect its `--help` and select the transmitter's actual device node before
sending the intended remote-control code.

Both rules match the specific native providers. Package updates reload rules
and request change events for the corresponding misc/LIRC nodes. Installation
does not send infrared codes or modify NFC tag contents. Source and udev rule
checks remain separate from optical/RF hardware acceptance.
