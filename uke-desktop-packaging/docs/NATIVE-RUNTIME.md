# Retired Python-free Fedora variants

The former Rawhide workflow rebuilt `at-spi2-core`, `gstreamer1`,
`libaccounts-glib`, `libwacom` and `libstdc++` to remove optional Python
files or helpers. These rebuilds are no longer selected by `make srpm`.
Use the original Fedora packages and their normal dependencies instead.

The pinned Rawhide image uses Fedora's `libstdc++` from its base image. The old
rebuilds have not been tested under the new policy. The
[Mesa source variant](../packaging/mesa/README.md) remains available because it
contains device graphics fixes unrelated to Python.
