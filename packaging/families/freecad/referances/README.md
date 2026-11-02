# Source cache

`make srpm` caches the upstream release archive and commit patches here, checks
every input against `../source-lock.json`, and includes them in the source RPM.
Archives and downloaded patches are ignored by Git. No third-party binary
application or AppImage is repackaged.
