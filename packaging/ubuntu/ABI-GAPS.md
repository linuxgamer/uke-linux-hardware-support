# Remaining binary dependency chains

Recorded 1 November 2026. The actual 27-DEB dependency fixture had eleven
unresolved binary package chains. Several share one missing provider; this is
not a count of distinct kernel ABIs. Adding the four inert native-artifact
packages must leave those eleven gates unresolved. A local APT simulation is
dependency evidence, not an authenticated archive or installation result.
The machine-readable [gap plan](kernel-abi-gap-plan.json) lists every chain;
`tests/abi-gap-plan.sh` compares its package set with supplied solver output.

| Binary package | Reusable existing evidence | Remaining work before runtime closure |
| --- | --- | --- |
| `uke-charge-control` |1006 thresholds/current config and signed guarded battmgr interfaces |Exact live cohort/API inventory; firmware/readback, autonomous protections and per-boot lease qualification |
| `uke-core-meta` |Matched full1006 cohort and built-in platform-profile support |APT transactions/finalizer/update and remaining userspace ports; child gates; factory firmware; runtime OEM DT, root security/layout, selector and rollback |
| `uke-displayport-audio` |Native policy package |Shared PipeWire port at the required version/ABI; physical DP lifecycle |
| `uke-front-color` |Native service and SSC userspace builds |SSC firmware/config rights or admitted local inputs; service lifecycle and real sensor qualification |
| `uke-gpu-defaults` |Signed MSM's actual `separate_gpu_kms` parameter matches the policy |Live exact cohort/render-only behavior; GPU firmware rights or admitted local inputs |
| `uke-offline-charge` |Native helper inventoried in the verified embedded initramfs |Charger marker, selector, root/security handoff, failure/recovery and charging qualification |
| `uke-proximity-capture` |Native capture and SSC userspace builds |SSC firmware/config rights or admitted local inputs; lifecycle and sensor qualification |
| `uke-speaker-controls` |Signed FS19xx module and shared amplifier/rotation patches |Live cohort, exact mixer API/topology and physical routing/lifecycle |
| `uke-speaker-orientation` |Native policy package |Transitive shared PipeWire port/ABI and physical orientation/audio lifecycle |
| `uke-ssc-fileservice` |Native service/cache builds with retained admission guards |Exact firmware/config rights or admitted local stock inputs; ownership, fallback and persist lifecycle |
| `uke-touchscreen` |Signed NT36532 module and native pinned userspace processing build |Exact live device/ioctl/geometry contract and physical qualification; private calibration remains private |

Existing artifacts close the four-role **packaging** gap and establish the
listed kernel/helper software inputs. They close **zero complete runtime
chains** by themselves. Module/config presence does not establish the full
hardware ABI. In particular, the charge development interface deliberately
retains its protection/admission requirements. No provider is added to make
the dependency solver pass. The actual 31-DEB follow-up resolves twenty chains
(eighteen without Python; two development SDK chains have host Python) and
retains exactly the same eleven unresolved packages. No package is installed.

PipeWire and Control/update/finalizer work are implementation tasks that can
progress independently of credentials. A kernel rebuild cannot supply those
userspace ABIs or missing firmware bytes. Firmware redistribution is assessed
per file; existing notice mapping for seven files does not clear the remaining
210 of 217 inventoried firmware files. A local-stock input route needs its own
provenance/admission design; it is not implicit redistribution permission.

The historical 1006u1 kernel lacks AppArmor and selects SELinux. On 2 November
the user selected AppArmor for Ubuntu while requiring Fedora to stay unchanged.
The [explicit distro profile](DISTRO-SECURITY.md) defines the new u2 cohort;
its native results are separate from root policy loading/enforcement and
runtime qualification. Existing u1 artifacts cannot establish the changed configuration.
The compile-time DT is not a replacement
for runtime OEM DT evidence. Firmware, layout, selector and rollback gates stay
in force before any image or tablet action.
