# Uke desktop selections

`uke-desktop-metas` records the console-first policy without selecting a desktop
environment. Optional packages select original Fedora applications:

| Package | Selection | Status |
| --- | --- | --- |
| `kde-plasma-uke-meta` | Plasma Desktop and common applications | Package candidate |
| `gnome-uke-meta` | GNOME Wayland and common applications | Package candidate |
| `plasma-mobile-uke-meta` | Plasma Mobile and on-screen keyboard | Experimental |
| `phosh-uke-meta` | Phosh with Fedora's dependency closure | Experimental |

Each optional RPM owns a matching JSON profile in `src/`. Dependencies retain
Fedora's original package names and normal helpers, including Python. None of
these RPMs installs a login-manager preset, enables a session or changes device
drivers. The existing login manager must support the selected Wayland session.
Profiles still identify Rawhide; the Fedora 44 migration is a separate change.

The Plasma Desktop list was compared with the Nabu KDE manifest and image profile
at [`03a5bdf09ce206410e7f7f1f97871110e66ffa82`](https://github.com/MCC45TR/Nabu-Fedora-Rawhide-Builder/tree/03a5bdf09ce206410e7f7f1f97871110e66ffa82).
Nabu hardware packages and its login-manager migration are excluded. GNOME,
Plasma Mobile and Phosh names were checked against Fedora package listings:
[GNOME session](https://packages.fedoraproject.org/pkgs/gnome-session/gnome-session-wayland-session/),
[Plasma Mobile](https://packages.fedoraproject.org/pkgs/plasma-mobile/plasma-mobile/)
and [Phosh](https://packages.fedoraproject.org/pkgs/phosh/phosh/).

Run `make validate` and `make srpm` for source checks and an SRPM candidate.
The current rootfs recipe remains console-only and pins an older
`uke-desktop-metas` RPM; it does not include these new selections. Full
AArch64 dependency resolution, a graphical root build and tablet behavior each
need separate validation. Display, touch, rotation and on-screen keyboards are
not qualified by selecting a package.

On 16 October 2026, source checks, availability of all new direct dependencies
and builds of one SRPM and five AArch64 RPMs passed in a Fedora 44 container.
Package profiles, direct requirements and absence of activation scriptlets were
checked. Installation, graphical images and new tablet sessions remain untested.
