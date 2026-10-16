# Fedora package checks

The older Rawhide package trials aimed for a Python-free target. That policy has
been retired in `uke-hardware`. Their build and transaction results are historical;
they do not validate the current package selection or the planned Fedora 44 base.

For a new package selection, use the original signed Fedora packages unless a
specific device fix requires a variant. Record exact package versions, source
identities and the dependency closure. Test installation, upgrade and removal in
a disposable AArch64 userspace. Check the extracted payload for private files
and unexpected device configuration. Python files and dependencies are allowed.

A successful package transaction or VM check does not establish a tablet boot,
graphical session, charging behavior or peripheral support. Record tablet tests
separately with the tested image, kernel and device variant.
