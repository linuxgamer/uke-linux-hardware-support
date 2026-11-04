# Device-local O82 preparation during kernel updates

5 November 2026. Source proposal for the unstable development channel.
Qualification is recorded in `manifests/o82-private-color-20261105.json`.

New memory/v5 kernels require `uke-kernel-update(o82-private-color-v5)` and
the native `uke-o82-color-profile` utility. Their RPM-owned boot manifest opts
into preparation with `o82_private_color=5`. DNF installs these dependencies
before its existing final automatic-staging trigger runs.

The updater admits the selected bundle and staging policy, then prepares the
fitted panel's private profile before the existing firmware, clock and boot
operations. A preparation failure stops activation. Read-only checks do not
open stock data; older bundles retain their existing behavior. Preparation
uses a bounded native child and the source partitions remain read-only.

No preset is selected and no HDR or DMA gate is enabled by this integration.
Automatic staging stays enabled, package rollback defaults remain enforced,
and an existing persistent rollback-off preference is retained. Full cumulative
kernel delivery and physical color/HDR acceptance remain separate gates owned
by the single integration coordinator.
