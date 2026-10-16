# Mesa AArch64 and Adreno 732 corrections

The exact Fedora `mesa-26.2.4-1.fc46` source RPM is pinned in
`manifests/native-runtime.json`. One upstream C++ conditional admits AArch64 to
LLVM host-feature detection. Disabled features must override CPU-name defaults:
the physical Uke llvmpipe crash stopped at an SVE instruction while Linux
reported no SVE. KDE applications and the original Fedora driver selection,
APIs and build flags are unchanged.

The build worker runs the exact patched feature function against LLVM's actual
feature map, checking every enabled and disabled attribute. The unpatched
AArch64 branch must fail this fixture. Signed payload, offline DNF rollback,
physical Wayland stability and observed output remain separate gates.

The Adreno 732 correction keeps Mesa's existing `0x43030b00` three-CCU profile,
changes its label from `FD735` to `FD732`, and admits the kernel's fuse-ID
wildcard. It does not alias another GPU or change shader properties. Native
shader/readback and KWin hardware rendering were observed with the experimental
kernel at 255 MHz on 13 October. This Mesa revision still needs its own package
and tablet tests; Vulkan, higher GPU frequencies and suspend remain unqualified.

The derived Rawhide recipe pins Meson, Python build tools and LLVM development
libraries. Python is allowed in Fedora packages; the former Python-file ban no
longer applies to this Mesa variant. Package payload and dependency checks are
still required before delivery. This Mesa revision has not passed those checks
or a tablet test under the new policy.
