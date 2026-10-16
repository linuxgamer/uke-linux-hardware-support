SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate srpm test-native
outdir ?= $(CURDIR)/build/srpm
validate:
	bash tests/adaptive-display.sh
srpm: validate
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1791936000 --owner=0 --group=0 --numeric-owner --transform='s,^,uke-adaptive-display-0.1.0/,' -cf - LICENSE docs/ADAPTIVE-DISPLAY.md packaging/uke-adaptive-display.target packaging/80-uke-adaptive-display.preset | gzip -n > "$$top/SOURCES/uke-adaptive-display-0.1.0.tar.gz"
	cp packaging/uke-adaptive-display.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --define "_topdir $$top" "$$top/SPECS/uke-adaptive-display.spec"
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'
