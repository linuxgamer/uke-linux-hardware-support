# Diagnostic logs

24 October 2026. Version 1.6.2 adds loaded MSM, O82 panel and THP module build-ID
notes and taint flags to `display` and `all` reports. Notes are recorded as hex
through the existing bounded collector. Compare these loaded-image identities
with installed package artifacts; `uname`, `modinfo` and RPM verification alone
do not establish which module image is running. Missing notes remain explicit.

22 October 2026. Uke Control 1.3.0 provides a single local log for support and
hardware-variant comparisons. Run it from the directory where the log belongs:

```sh
sudo uke-control diagnostic all
sudo uke-control diagnostic wifi
sudo uke-control diagnostic bluetooth
sudo uke-control diagnostic sensors
sudo uke-control diagnostic sleep
uke-control diagnostic --help
```

The first command writes `uke-diagnostic-all.log`; focused commands write
`uke-diagnostic-wifi.log`, `uke-diagnostic-bluetooth.log`, and so on. Existing
files, directories and symbolic links are refused. Move an earlier log before
repeating collection. The file has mode 0600 and belongs to the invoking
effective user; a log created through sudo normally requires sudo to read.
An ordinary user can collect a partial report with explicit permission failures.

| Topic | Collected evidence |
| --- | --- |
| `all` | Every topic below, including current and previous boot logs |
| `system` | Distribution, CPU topology, memory, pressure, processes and resource sample |
| `hardware` | Udev database, all sysfs device directories, bus/driver links, device nodes, PCI and cached Uke hardware inventory |
| `kernel` | Kernel build/configuration, loaded modules, interrupts, memory map, deferred probes, current/previous kernel logs and module settings |
| `boot` | Current/previous journals, boot history/timing/order, boot artifact metadata, boot identity and existing update-state records |
| `packages` | RPM versions/vendor/source/signature/install time, database/file checks, DNF history, dependency problems, duplicates, retained kernels and cached repositories/updates |
| `services` | Failed/all units, enablement, overrides, warnings/errors and crash inventory |
| `wifi` | Interface paths, PHY capabilities, link/address/routing/DNS state, NetworkManager, radio blocks and related service/kernel logs |
| `bluetooth` | Controller paths, existing BlueZ tree, radio blocks and related service/kernel logs |
| `sensors` | IIO/hwmon/thermal paths and exported readings/scales/offsets, existing desktop/color interfaces and SSC/sensor logs |
| `sleep` | Supported sleep states, wake permissions/counters, inhibitors, suspend statistics/configuration and related logs |
| `power` | Battery/charger metadata, cached protection, power profiles, thermal/cooling/CPU policy exports and related logs |
| `usb` | Cached USB/Type-C topology, USB IDs/drivers/speed, port/data/power roles and related logs |
| `display` | Disk module metadata, loaded display/touch module build IDs and taint, connector/mode/backlight/devfreq exports, and related logs |
| `audio` | ALSA inventory, audio/DSP paths, existing PipeWire session and related logs |
| `input` | Input nodes/aliases, advertised capabilities/properties and related logs |
| `camera` | Video/media node metadata and related logs |
| `storage` | Block/partition/filesystem labels and paths, mount/fstab/swap/capacity/I/O state and related logs |
| `firmware` | Cached SCMI/remote-processor state, installed firmware file metadata and related logs |
| `security` | SELinux status/denials, lockdown, kernel access restrictions and related logs |

Each focused report includes only its topic plus the distribution, running
kernel and collector RPM version. Commands run sequentially with their exact
arguments, exit status and elapsed time recorded. Exit 127 indicates an absent
executable; exit 124 indicates a time/output limit. Other nonzero statuses can
mean an unavailable service, no matching journal entries, insufficient access,
or a genuine failure; inspect the adjacent output. A missing file pattern is
recorded explicitly. The final `COLLECTION COMPLETE` marker confirms that the
writer reached its footer. It does not certify every collector or device.
An interrupted log lacks that footer and is retained for inspection.

Collection is bounded to 20 seconds and 8 MiB per command, 4096 matches per file
pattern and the first 65536 bytes per file. Large regular files receive a
truncation notice. Sysfs sizes do not reliably describe content length. The
overall budget is 15 minutes/256 MiB, checked between collectors; the final
collector and text escaping can extend the size/time boundary. The footer
records whether remaining collectors were skipped. Journal entry limits are
visible in each command. Old journals may not exist, and DNF results depend on
existing metadata because network refresh is disabled.

The collector uses distribution executables through absolute paths, without a
shell. Base tools come from coreutils, findutils, util-linux, procps-ng, RPM and
systemd. Optional evidence uses DNF5, pciutils, usbutils, iw, NetworkManager,
audit/SELinux tools and WirePlumber. Missing tools are recorded; collection does
not install them. No new Python payload or Python runtime is added.

The collector requests no Wi-Fi scan, Bluetooth pairing, audio/camera stream,
suspend, reboot, partition write, package transaction or firmware control RPC.
It does not mount debugfs, start sensor acquisition or read persist/calibration
payloads. BlueZ discovery disables service activation. Standard exported sensor
attributes can perform normal driver reads. DNF queries may update ordinary
local bookkeeping; they do not refresh repositories. Desktop/audio commands use
the existing process session, so sudo/SSH may have no user-session connection.
Those failures remain visible rather than selecting a guessed user or bus.

The log is private and is **not anonymized or uploaded**. Journals and metadata
can contain usernames, hostnames, addresses, device identifiers, partition UUIDs,
repository URLs and other private context. Connection secrets and firmware/core
dump/calibration payloads are not requested. Review/redact a copy before sharing
it publicly. The existing `report github` anonymous-summary flow is unchanged;
it does not accept or upload these diagnostic logs.

Host fixtures cover topic isolation/coverage, private file modes, existing-file
and symlink refusal, shell-like invalid topics, control-character escaping and
unavailable/timeout receipts. Hardware acceptance and calibration accuracy
require separate device evidence; a complete diagnostic log establishes neither.
