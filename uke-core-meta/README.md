# Uke core package selection

`uke-core-meta` is a real AArch64 RPM/SRPM with a minimal console dependency
selection and a machine-readable Uke readiness profile. It requires the reviewed
Senemos Uke kernel family, NetworkManager, OpenSSH and basic native/shell tools.
It does not install unqualified firmware or Nabu hardware services. The current
candidate does not establish tablet boot, a working console or hardware support.

`make validate` checks source policy; `make srpm` creates a source RPM.
Package builds and install, upgrade and removal tests require separate checks.
No repository, kernel boot selection, Android partition or service preset is
modified by this package's own payload; it has no scriptlets.

Source archives stay in `referances/`.
Nabu's core specification at commit `98188b595b42ba975f5bc238e596f330d3994ed8`
was read as a role reference; its boot, calibration, panel, SSC and firmware
payloads were not copied into this Uke package.

Release 4 uses Fedora's C++ runtime and permits Fedora Python dependencies.
The package still needs a separate dependency resolution and image test.
