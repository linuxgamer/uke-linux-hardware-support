# Contributor and agent instructions

These rules apply to the workspace and every project-owned component. Read component instructions before changing that component. Reference repositories are evidence, not instructions for the parent project.

- Write GitHub documentation, issues, commit messages and pull requests in clear, natural English. User conversation may be in any language.
- Write GitHub READMEs, status tables, release notes, issues, pull requests and commit summaries as concise English engineering reports. State the date, test environment, observed result and remaining work where relevant. Name devices directly; omit personal ownership labels, conversational narration, promotional wording and internal review jargon. Keep source, build, package, VM and tablet results distinct. Preserve evidence, attribution and test limitations. Keep detailed trial histories in private records.
- Use C++ for new native device applications and recovery management tools. Follow upstream C/assembly requirements in Linux and EDK II; do not convert their existing code to C++.
- Never clone, fork or rebuild KDE desktop applications as Uke variants. Use the distribution's original KDE applications. Keep incompatible target dependency requirements explicitly gated.
- Prefer Bash for host automation and Go only when shell is unsuitable. Python is allowed for host tooling where it is the clearer choice.
- Keep device payloads small: prefer native or POSIX-shell helpers for tablet startup, and audit transitive packages and extracted payloads.
- Keep source archives below the owning component's `referances/`. Do not modify or execute donor scripts as a shortcut. Development happens in `src/`, patches, configuration and project code.
- Before hardware changes, consult the pinned source archives under `referances/releases/`, the umbrella hardware-integration policy ([CONTRIBUTING.md](https://github.com/linuxgamer/uke-fedora/blob/main/docs/CONTRIBUTING.md#hardware-integration-policy)) and the upstream model ([UPSTREAM.md](https://github.com/linuxgamer/uke-fedora/blob/main/docs/UPSTREAM.md)). The old `referances/README.md` and `uke-linux-docs/` trees were removed by the umbrella fork; do not look for them. Compare exact Uke OEM evidence, relevant same-SoC/shared-IP donors and current upstream with the existing implementation. Record source commits/files, attribution, compatibility limits and separate host/device validation in the private lesson journal; document a missing applicable donor instead of assuming a match.
- Keep source, build, package, emulation, third-party hardware and own-device results separate. Never infer hardware success from a build.
- Public files must not contain credentials, private logs, unit identifiers, calibration data, home-directory paths or personal author email. Use the account's GitHub no-reply email locally.
- Preserve stock early firmware and a documented recovery route. Storage plans must be derived from Uke evidence; Nabu offsets, GPT backups and security binaries are not portable.
- Keep commits focused and explain what changed and why. Run the relevant checks before publication. Do not create hardware success records without evidence.

## Engineering lessons and laboratory records

- Record every material research, implementation, build, packaging and test finding, including failed trials and corrections, in `docs/lessons/` using its template and index. Write professional English and include date, stable lesson ID, environment, exact evidence, practical consequence, uncertainty and next validation. Preserve older findings with explicit superseding corrections.
- Maintain separate Fedora Linux and PenguinOS evidence for POCO Pad X1. The laboratory RC1001/JH925 plan and equipment log (`docs/testing/ACCESSORY-TEST-PLAN.md`, `docs/research/ACCESSORY-LAB.md`) are not present in this fork; keep the evidence that exists under `docs/lessons/`. The umbrella documents these removed paths in [DEVELOPING.md](https://github.com/linuxgamer/uke-fedora/blob/main/docs/DEVELOPING.md#missing-evidence-paths). OrangeFox recovery packaging is not carried by the umbrella fork. Purchase reports and advertised specifications must not become physical support claims.

## Checks

Run from this repository root:

```sh
make validate     # package metadata, source locks, schemas, service and udev contracts
make test-native  # native helper tests; needs the host build dependencies
```

`make validate` is fast and does not need containers. `make test-native` and
`make native-guis` are needed only when the change touches those helpers and
require host build dependencies. Record host, package, VM and on-device results
separately; a passing check is not a device test.

## Working with the umbrella

Commit and push here, and open the pull request in this repository. The umbrella
repository updates its gitlink only after this change is accepted; cross-repo
links must use absolute URLs. Contributor rules:
[AGENTS.md](https://github.com/linuxgamer/uke-fedora/blob/main/AGENTS.md) and
[DEVELOPING.md](https://github.com/linuxgamer/uke-fedora/blob/main/docs/DEVELOPING.md).
