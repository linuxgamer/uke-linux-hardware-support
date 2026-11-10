# TOUCH-2026-10-23-TM-PANEL

- Date: 23 October 2026.
- Environment: Xiaomi Pad 7 running OS3.0.304.0 TWRP; Fedora 44 KDE boot journal from the same tablet; host C++ checks.
- Evidence: the stock touch driver reports `lcd_id_value = 0, this panel is TM` and requests `novatek_nt36532_o82_fw_tm.bin`. TWRP reads GPIO 26 as input low. The Fedora image installed CSOT firmware and its service repeatedly stopped at `Touch panel-ID must be high`. The pinned TM normal image and configuration pass the host protocol and geometry checks.
- At the time, the Fedora 44 image recipe selected TM at build time. The 25 October candidate packages both pinned variants and selects one from GPIO26 at startup. The daemon checks both the firmware and its configuration against that choice before loading firmware. CSOT still lacks a physical test.
- Correction: the TM image booted and reached Plasma setup on 24 October, but touch still failed because the daemon required a native panel reset provider. See [TOUCH-2026-10-24-FRAMEBUFFER-HANDOFF](TOUCH-2026-10-24-FRAMEBUFFER-HANDOFF.md).
- TM touch input worked through KDE setup and login with the single-variant image. Stylus behavior and the new automatic selection remain untested on the tablet.
- Next validation: boot an image with both variants and confirm TM selection; test CSOT on a tablet with that touch panel.
