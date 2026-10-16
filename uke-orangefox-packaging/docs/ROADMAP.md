# Initial Uke roadmap

1. Inventory the named Nabu reference families and pin relevant upstream sources.
2. Establish Uke-specific interfaces, licenses and firmware profile boundaries.
3. Implement the intended packages and complete these gates:

- Verify exact GitHub release asset hashes and source provenance.
- Keep alpha and stable release channels distinct.
- RPM scriptlets must never flash, unlock or change boot selection.

4. Build and audit AArch64 RPM/SRPM payloads and their complete dependency closure.
5. Test installation, upgrade and removal in an isolated target environment.
6. Publish accepted development candidates to COPR, then collect distinct physical
   results using a preserved stock/recovery route.

The initial alpha delivery now has real RPM/SRPM, extracted payload, AArch64
DNF install/upgrade/removal and signed COPR repository records. The SCM source
build and stable polling workflow also passed at the time. That automation is
no longer present in this repository. A fresh complete OrangeFox source build,
the first positive stable advancement and physical boot/rollback remain
separate next gates.
