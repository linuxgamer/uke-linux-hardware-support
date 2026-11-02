# Explicit kernel security profiles

2 November 2026. Ubuntu AppArmor support is authorized for server compilation.
The shared source remains the pinned experimental1006 composition. The new
Ubuntu release is `7.3.1-1006u2`, separating its module/output identity from
the older SELinux-based `u1` artifact. Source/profile validation is separate
from native compilation, runtime enforcement and tablet boot acceptance.

The host configuration tool requires `--distro ubuntu|fedora`, an unchanged
baseline and a new output root. It writes only `OUTPUT_ROOT/DISTRO/.config`.
There is no environment/OS autodetection or default distro, and output reuse
or a previously modified baseline is refused. The Fedora selection copies
the complete original baseline byte for byte. Fedora RPM specifications,
source archives, default config, SELinux settings and boot producer remain
unchanged and do not acquire AppArmor.

Ubuntu applies only `packaging/kernel-security/ubuntu.config`, then validates
the resolved Kconfig. It enables AppArmor and its SECURITY/NET/AUDIT,
securityfs, path/network and policy-verification dependencies. The default
LSM list is `lockdown,yama,apparmor,bpf`: AppArmor is its sole major LSM.
Compiled SELinux support and the remaining shared hardening settings are
retained; SELinux is excluded from Ubuntu's default active LSM list. The
original Fedora list remains `lockdown,yama,selinux,bpf` with AppArmor off.

The Debian first-pass and complete Ubuntu boot producers require
`UKE_KERNEL_DISTRO=ubuntu`. Debian rules select that flag explicitly; direct
producer invocations must supply it. Their output directories include `ubuntu`, and
both complete Image passes repeat the security checks after `olddefconfig`.
The Ubuntu host adapter changes only its copied hardening contract's selected
major LSM; all existing signature, lockdown, Yama and memory protections remain
required. It does not edit the shared Fedora contract or configuration. Raw
source assembly never executes Kconfig or selects a security profile.

The original `u1` artifact and its fourteen-input receipt remain historical
evidence at commit208492c. The artifact-only profile is version-bound and
cannot turn those bytes into an AppArmor `u2` result. Reproduce that original
trial from its recorded source commit; a different config requires a complete
new Image/module/initramfs cohort and module-signing identity.

Source/refusal tests cover absent/unknown/duplicate flags, separate outputs,
unchanged Fedora bytes, cross-profile contamination, changed baseline
protections, duplicate settings, missing AppArmor dependencies and conflicting
LSM/cmdline choices. Native server validation must additionally retain both
resolved profile configs, their hashes, the Ubuntu Image/modules/certificate
and boot readback. Host checks do not establish running AppArmor policy.
Future authorized runtime tests must read `/sys/kernel/security/lsm`, check
AppArmor enabled/features and policy loading/enforcement on the actual root.
Firmware, runtime OEM DT, selectors/rollback and complete licensing remain
independent gates. No root/image, tablet, stable or publication action is added.

References checked 2 November 2026: the pinned Linux7.3.1
`security/apparmor/Kconfig`, `security/Kconfig` and AppArmor LSM source;
[kernel AppArmor documentation](https://docs.kernel.org/admin-guide/LSM/apparmor.html),
[LSM ordering](https://docs.kernel.org/admin-guide/LSM/index.html) and
[Ubuntu AppArmor documentation](https://ubuntu.com/server/docs/how-to/security/apparmor/).
Kernel support alone does not enforce application policy: userspace must load
the appropriate profiles. No claim of Ubuntu-specific downstream AppArmor
extensions or completed root-policy compatibility is made.
