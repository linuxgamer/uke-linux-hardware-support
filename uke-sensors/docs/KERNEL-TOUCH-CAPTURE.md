# Kernel THP contact consumer

23 October 2026 follow-up: the client reports systemd readiness only after its
first valid captured frame. Registering an input device alone cannot make a
failed firmware stream healthy for boot acceptance. The source exporter runs
the production notification fixture before constructing the kernel payload.
Physical contact and cold-start acceptance remain separate tests.

23 October 2026. Source/host validation on x86_64 and AArch64; no tablet
deployment of this consumer is claimed.

`src/touch-capture.cpp` consumes the candidate kernel driver's complete 5432-byte
MET8 packets. It uses the existing calibrated C++ contact core, packet decoder,
type-B publisher and O82 readiness parser. Shared legacy implementations were
moved without changing their contents. The private `uke-local-touch-data`
package remains required; configuration and reference packets are not bundled.

The client admits only the matching Uke SPI provider, driver and metadata
geometry. Two bounded MMAP buffers carry monotonic timestamps. Error, short,
future and older-than-100-ms frames are rejected before contact processing.
Each dequeued buffer is returned. Panel generation changes release contacts,
stop capture and discard the old queue before opening the next stream. The
client never opens SPI or GPIO, downloads firmware or changes charger policy.

The [standard MET8 format](https://docs.kernel.org/userspace-api/media/v4l/metafmt-generic.html)
preserves the packet bytes, including signed raw nodes, counter and scan type.
It does not label those raw nodes as signed delta measurements. The kernel
driver owns firmware admission, SPI, IRQ, watchdog, power and panel lifecycle.

```sh
make -C uke-sensors test-touch-capture TOUCH_CORE_REFERENCE=/path/to/verified/reference
```

The target runs format, freshness and provider-refusal fixtures, then exercises
the actual queue owner with synthetic ioctls and real memfd mappings. Eight
constructor failures, dequeue/requeue, stale/error buffers and cleanup pass.
The complete client compiles on x86_64 and AArch64 with warnings as errors.
Clang ASan/UBSan passes the new fixtures and shared read-only admission tests.
The existing publisher's ten-contact, tracking, lift and release fixtures pass.
These checks do not establish physical touch latency, suspend or VRR behavior.

Release recipes do not install or select this client yet. Paired kernel/DT
selection, firmware-path provisioning, service migration and a recoverable
tablet trial must precede removal of the existing touchscreen package. Preserve
its working SPI path for accepted-kernel rollback until that migration passes.
