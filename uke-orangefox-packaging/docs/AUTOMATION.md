# Release delivery and rebuild history

The automatic COPR and daily stable workflow described below was part of the
14 October 2026 development setup. Its `.copr/Makefile` and GitHub workflow
have since been removed. The commands below describe the former release gate;
they are not active release automation.

The former COPR setup used the `main` branch, `.copr/Makefile`, `make_srpm` and
the normal RPM spec.
Source generation downloaded the locked GitHub assets, checked every SHA-256 and
verified the public Uke validation manifest. Binary packaging ran offline.
Source snapshots were offered alongside the original release and identified in
each package's lock file; the SRPM rebuilt image delivery rather than compiling
OrangeFox from scratch.

The former GitHub push webhook rebuilt accepted packaging changes. The daily
workflow called `make track-stable`, selecting the newest published non-draft,
non-prerelease release. It admitted only the explicit stable tag format
`rMAJOR.MINOR-uke.YYYYMMDD`, all six delivery assets and both corresponding
source snapshots with GitHub SHA-256 digests. The artifact manifest had to retain
the reviewed Uke firmware profile and pass compile, header, ZIP, installer,
source and privacy gates. Unknown profiles, missing assets and disguised
alphas failed before changing the source pin.

When eligible stable inputs changed, the workflow audited the image ramdisk,
the nonempty extracted target, ELF dependencies and the ZIP on its host runner,
then committed the new lock/spec and explicitly requested a COPR build. This
did not execute any image or installer. `COPR_RECOVERY_BUILD_HOOK` was a GitHub
secret and was excluded from public source and reports. `releases_file=PATH` was
an optional local metadata fixture input; scheduled runs used the project GitHub
API.

DNF makes accepted images available through ordinary package upgrades. It
does not flash them, change Android firmware or schedule device writes. The
versioned image directory and manifest allow inspection before any separately
authorized device test. Physical acceptance never follows automatically from
a release label or COPR job.

Same-version payload changes need an increasing RPM release. A new stable
recovery version resets that release counter. Unimplemented Nabu counterparts
remain initial repositories until a real Uke payload and its package tests exist.

Reference: [COPR source methods and webhook protocol](https://docs.pagure.org/copr.copr/user_documentation.html).
