SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate test-native
components := uke-core-meta uke-desktop-metas uke-desktop-packaging uke-orangefox-packaging uke-boot uke-desktop-integration uke-sensors xiaomi-uke-firmware
validate:
	for component in $(components); do $(MAKE) -C "$$component" validate; done
test-native:
	bash uke-orangefox-packaging/tests/release-contract.sh
	$(MAKE) -C uke-sensors test-native
	$(MAKE) -C uke-boot test-native
	$(MAKE) -C xiaomi-uke-firmware test-admission
