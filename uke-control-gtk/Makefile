SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate srpm
package := uke-control-gtk
version := $(shell awk '/^Version:/ {print $$2; exit}' $(package).spec)
outdir ?= $(CURDIR)/build/srpm
TARGET_ARCH ?= aarch64
SOURCE_DATE_EPOCH ?= 1793577600
validate:
	test -s LICENSE -a -s CMakeLists.txt -a -s src/main.cpp
	test -s data/org.uke.Control.Gtk.desktop -a -s data/org.uke.Control.Gtk.svg
srpm: validate
	top="$$(realpath -m "$(outdir)/rpmbuild")"
	mkdir -p "$$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS}
	tar --sort=name --mtime=@$(SOURCE_DATE_EPOCH) --owner=0 --group=0 --numeric-owner -czf "$$top/SOURCES/$(package)-$(version).tar.gz" --transform 's,^,$(package)-$(version)/,' LICENSE README.md CMakeLists.txt Makefile src tests data $(package).spec
	cp $(package).spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target $(TARGET_ARCH) --define '_buildhost uke-builder' --define "_topdir $$top" "$$top/SPECS/$(package).spec"
	mkdir -p "$(outdir)"
	install -m644 "$$top/SRPMS/"*.src.rpm "$(outdir)/"
