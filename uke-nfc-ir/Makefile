SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
outdir ?= $(CURDIR)/build/srpm
.PHONY: srpm
srpm:
	test ! -e /etc/senemos/kernel-update.conf
	top="$$(realpath -m "$(outdir)/rpmbuild")"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1793750400 --owner=0 --group=0 --numeric-owner -czf "$$top/SOURCES/uke-nfc-ir-1.1.0.tar.gz" --transform 's,^,uke-nfc-ir-1.1.0/,' README.md 70-uke-nfc-ir.rules src include tests LICENSES uapi-source.json
	cp ../uke-control-qt/LICENSE "$$top/SOURCES/"
	cp uke-nfc-ir.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" "$$top/SPECS/uke-nfc-ir.spec"
	install -Dm644 "$$top/SRPMS/uke-nfc-ir-1.1.0-1"*.src.rpm "$(outdir)/uke-nfc-ir-1.1.0-1.src.rpm"
