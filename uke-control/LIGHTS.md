# Uke light controls

25 October 2026. Control 1.10.2 uses original kernel LED class interfaces on
Xiaomi Pad 7 / POCO Pad X1. Source and host checks do not establish physical
light output or measured current/voltage.

```sh
uke-control flashlight status
sudo uke-control flashlight on
sudo uke-control flashlight off
uke-control led status
sudo uke-control led set 255 0 0
sudo uke-control led set 0 255 0
sudo uke-control led set 0 0 255
sudo uke-control led off
```

`on` starts a supervised torch worker. Its independent local deadline is
55 seconds, counting suspended time, and systemd enforces a 60-second maximum.
The service conflicts with and stops before `sleep.target`. Normal stop, failure and
service cleanup request zero brightness. The service is never enabled at
boot. Repeating `on` while active does not extend the original deadline.

The exact reviewed kernel graph is required: original QCOM flash driver,
PMXR2230 rear channels 1 and 4, and a **20,000 microampere total** torch ceiling.
This is 10 mA per channel, below the retained OEM 250 mA per-channel limit.
No flash/strobe mode, regulator voltage control or arbitrary sysfs path is
exposed. The original driver's protection and current clamp remain active.

RGB accepts color components from 0 through 255 and follows the kernel's
reported channel order. Global PWM brightness stays at 8 out of 511; this is
a duty setting, not an ampere measurement. Color changes first turn the output
off, then set all intensities, then apply the fixed low brightness. Failures
attempt to leave it off. No notification or camera automation is installed.

Missing or mismatched board identity, provider, driver, current limit, channel
mapping, scale or trigger ownership prevents changes. Older kernels report an
unavailable provider. Installing a new kernel RPM alone does not activate its
DT graph; the matched kernel must be booted separately.
Provider identity follows the parent controller's DT link and the exact LED
label; it does not depend on an optional per-LED OF symlink.

Physical acceptance remains open: actual PMIC probe, visible torch/RGB output,
color mapping, measured electrical/thermal behavior, stop/timeout and suspend
lifecycle. Do not interpret a status response or package build as these results.
