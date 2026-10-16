# xiaomi-uke-firmware

File-level firmware provenance, acquisition and redistribution policy for Xiaomi Pad 7 / POCO Pad X1 (`uke`, SM7675).

This component owns the file-level admission rules for the shared hardware
support repository. The current lock admits no firmware bytes, so `make srpm`
fails explicitly. A metadata-only RPM cannot substitute for device firmware.

## Package scope

- `xiaomi-uke-firmware`

Nabu reference families: `xiaomi-nabu-firmware`.
These are process references; Uke wiring, firmware and runtime behavior require
independent implementation and validation.

## Required validation

- Record exact source/license/hash for every file.
- Do not publish OEM blobs without file-level redistribution terms.
- Never publish unit calibration, persist data or security firmware.

The first packaging target is Fedora Rawhide AArch64 in
[uke-linux-test COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/).
DEB and Alpine APK targets require their own native rules later.
Unimplemented targets fail explicitly rather than producing placeholder RPMs.

Use `make validate` for available source and package checks. Native device
applications use C++; upstream C/assembly interfaces retain their original
languages. Host automation may use Python where appropriate. Source archives
belong in `private/`, development in `src/`.
Public records exclude personal paths, unit identifiers and calibration data.

The [firmware lock](manifests/firmware-lock.json) pins the inspected upstream
inventory and keeps redistribution and hardware acceptance false. Before
admission, every source file and license must match its SHA-256, and its target
path must match an enabled Uke driver request from the selected kernel profile.
Security firmware, device calibration, persist files and foreign-board guesses
are excluded.

The [OEM Wi-Fi source inventory](manifests/oem-wifi-source.json) records verified
OS3.0.304.0 modem extraction, WPSS split-file hashes and O82 board candidates.
It grants no firmware admission or redistribution permission. The ELF32 WPSS
image fits the OEM reserved region, but exact board selection, Linux power/DMA
bindings and physical firmware startup still require validation. Candidate
bytes remain below this component's ignored `private/` directory.

On the build host, run `make test-admission` for synthetic acceptance/rejection
fixtures. For real candidate bytes, run:

```bash
make audit-admission LOCK=reviewed-lock.json ARCHIVE=private/source-tree REQUESTS=kernel-inputs.json
```

Generate `kernel-inputs.json` using the host-only audit in `uke-boot/src/`.
An empty request list from an incomplete device tree does not prove that the
physical tablet needs no firmware. Keep raw evidence in the parent repository's
ignored `private/` directory.
