SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate test-native test-applet test-applet-load srpm
version := 1.0.0
package := $(notdir $(CURDIR))
outdir ?= $(CURDIR)/build/srpm
# shortcut: timestamps follow the monotonic Release, include Version before resetting Release.
archive_epoch := $(shell awk '/^Release:/ {printf "%.0f", 1791158400 + int($$2); exit}' packaging/uke-desktop-metas.spec)
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false' manifests/component.json >/dev/null
	jq -e '.device == "uke" and .hardware_tested == false and .boot_tested == false' src/profile.json >/dev/null
	if grep -Eq '^(Requires|Recommends):.*(nabu)' packaging/*.spec; then exit 1; fi
	bash tests/plasma-profile.sh
	bash tests/cutefish-profile.sh
test-native: validate
test-applet:
	node tests/applet.cjs
test-applet-load:
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Werror $(LDFLAGS) tests/applet-load.cpp $$(pkg-config --cflags --libs Qt6Gui Qt6Qml) -o build/applet-load
	runtime=$$(mktemp -d)
	trap 'rm -rf -- "$$runtime"' EXIT
	XDG_RUNTIME_DIR="$$runtime" QT_QPA_PLATFORM=offscreen QT_QPA_PLATFORMTHEME=generic QT_QUICK_BACKEND=software build/applet-load "$(CURDIR)/src/plasma-applet/contents/ui/main.qml"
	bash tests/applet-cache.sh
srpm: validate
	top="$$(realpath -m "$(outdir)/rpmbuild")"
	mkdir -p "$$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS}
	tar --sort=name --mtime=@$(archive_epoch) --owner=0 --group=0 --numeric-owner -cJf "$$top/SOURCES/$(package)-$(version).tar.xz" --transform 's,^,$(package)-$(version)/,' LICENSE README.md Makefile src tests packaging
	cp packaging/$(package).spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define '_buildhost uke-builder' --define "_topdir $$top" "$$top/SPECS/$(package).spec"
	mkdir -p "$(outdir)"
	install -m644 "$$top/SRPMS/"*.src.rpm "$(outdir)/"
