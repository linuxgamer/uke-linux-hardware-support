SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
outdir ?= $(CURDIR)/build/srpm
.PHONY: srpm
srpm:
	test ! -e /etc/senemos/kernel-update.conf
	top="$$(realpath -m "$(outdir)/rpmbuild")"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1793664000 --owner=0 --group=0 --numeric-owner -czf "$$top/SOURCES/uke-control-gui-check-1.0.0.tar.gz" --transform 's,^,uke-control-gui-check-1.0.0/,' solver.cpp verify.sh
	cp ../control-gui/copr-public-key.asc "$$top/SOURCES/"
	cp ../../../uke-control-qt/LICENSE "$$top/SOURCES/"
	cp uke-control-gui-check.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" "$$top/SPECS/uke-control-gui-check.spec"
	identity=$$(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$$top/SPECS/uke-control-gui-check.spec")
	install -Dm644 "$$top/SRPMS/$$identity.src.rpm" "$(outdir)/$$identity.src.rpm"
