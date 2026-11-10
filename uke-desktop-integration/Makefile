SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate srpm test-native test-plugin
outdir ?= $(CURDIR)/build/srpm
BUILD_DIR ?= build
.PHONY: pipewire-monitor-srpm
pipewire-monitor-srpm:
	bash scripts/build-pipewire-monitor-srpm.sh "$(FEDORA_PIPEWIRE_SRPM)" "$(abspath $(BUILD_DIR))/pipewire-monitor/rpmbuild"
.PHONY: speaker-controls-srpm
.PHONY: displayport-audio-srpm
displayport-audio-srpm:
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	bash tests/audio-policy.sh
	tar --sort=name --mtime=@1793059200 --owner=0 --group=0 --numeric-owner --transform='s,^,uke-displayport-audio-0.1.0/,' -cf - LICENSE src/ucm2 src/wireplumber tests/audio-policy.sh tests/audio-policy.cpp | gzip -n > "$$top/SOURCES/uke-displayport-audio-0.1.0.tar.gz"
	cp packaging/uke-displayport-audio.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --define "_topdir $$top" "$$top/SPECS/uke-displayport-audio.spec"
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'
speaker-controls-srpm:
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1793059200 --owner=0 --group=0 --numeric-owner --transform='s,^,uke-speaker-controls-0.1.0/,' -cf - LICENSE SPEAKER-CONTROLS.md src/uke-speaker-controls.desktop src/kmix | gzip -n > "$$top/SOURCES/uke-speaker-controls-0.1.0.tar.gz"
	cp packaging/uke-speaker-controls.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --define "_topdir $$top" "$$top/SPECS/uke-speaker-controls.spec"
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and (.expected_packages | index("uke-adaptive-display") != null)' manifests/component.json >/dev/null
	bash tests/adaptive-display.sh
srpm: validate
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1792281600 --owner=0 --group=0 --numeric-owner --transform='s,^,uke-adaptive-display-1.0.16/,' -cf - LICENSE docs/ADAPTIVE-DISPLAY.md docs/DISPLAY-CONTROLS.md docs/SUSPEND.md docs/SERVICE-POLICY.md docs/GPU-MONITORING.md src/adreno-stats packaging/adreno.page packaging/uke-adaptive-display.target packaging/80-uke-adaptive-display.preset packaging/71-uke-tablet-mode.rules packaging/61-uke-display-ddc.rules packaging/60-block-scheduler.rules packaging/powerdevilrc packaging/kwinoutputconfig.json packaging/60-uke-display-ddc.conf packaging/30-uke-s2idle.conf packaging/uke-suspend.conf src/uke-ddc-config tests/ddc-config.sh | gzip -n > "$$top/SOURCES/uke-adaptive-display-1.0.16.tar.gz"
	cp packaging/uke-adaptive-display.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" "$$top/SPECS/uke-adaptive-display.spec"
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'
test-native:
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/adreno-stats/tests.cpp -o build/adreno-counter-test
	build/adreno-counter-test

.PHONY: test-bluetooth
test-bluetooth:
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror -Wno-unused-function tests/bluetooth-address.cpp -o build/bluetooth-address-test
	build/bluetooth-address-test

test-plugin:
	cmake -S src/adreno-stats -B build/adreno-stats -DCMAKE_BUILD_TYPE=RelWithDebInfo
	cmake --build build/adreno-stats --parallel 4
	ctest --test-dir build/adreno-stats --output-on-failure

# Host-only OEM data conversion; this executable is not a tablet startup task.
.PHONY: test-oem-brightness
test-oem-brightness:
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/oem-brightness-curve.cpp -o build/oem-brightness-test
	./build/oem-brightness-test
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/oem-brightness-curve.cpp -o build/uke-oem-brightness-curve

.PHONY: speaker-orientation-srpm test-speaker-orientation
speaker-orientation-srpm:
	mkdir -p $(BUILD_DIR)/speaker-orientation/rpmbuild/{SOURCES,SRPMS,SPECS}
	bash tests/speaker-orientation.sh
	tar --sort=name --mtime=@1793059200 --owner=0 --group=0 --numeric-owner --transform='s,^,uke-speaker-orientation-0.1.0/,' -czf $(BUILD_DIR)/speaker-orientation/rpmbuild/SOURCES/uke-speaker-orientation-0.1.0.tar.gz LICENSE SPEAKER-ORIENTATION.md src/uke-speaker-orientation.cpp src/uke-speaker-orientation.desktop tests/speaker-orientation.cpp tests/speaker-orientation.sh
	cp packaging/uke-speaker-orientation.spec $(BUILD_DIR)/speaker-orientation/rpmbuild/SPECS/
	rpmbuild -bs --target aarch64 --define '_topdir $(abspath $(BUILD_DIR))/speaker-orientation/rpmbuild' --define '_buildhost copr-builder' $(BUILD_DIR)/speaker-orientation/rpmbuild/SPECS/uke-speaker-orientation.spec

test-speaker-orientation:
	bash tests/speaker-orientation.sh
test-native: test-speaker-orientation

# Explicit diagnostics; their SDK dependencies are outside default packaging.
.PHONY: vulkan-probe gles-probe opencl-probe mesa-libclc-srpm
vulkan-probe:
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++17 -Wall -Wextra -Werror src/vulkan-probe.cpp $(LDFLAGS) -lvulkan -o $(BUILD_DIR)/uke-vulkan-probe
gles-probe:
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++17 -Wall -Wextra -Werror src/gles-probe.cpp $(LDFLAGS) -lEGL -lGLESv2 -o $(BUILD_DIR)/uke-gles-probe
opencl-probe:
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++17 -Wall -Wextra -Werror src/opencl-probe.cpp $(LDFLAGS) -lOpenCL -o $(BUILD_DIR)/uke-opencl-probe
mesa-libclc-srpm:
	@test "$$(sha256sum referances/mesa-libclc-20261024/mesa-libclc-87f66007242ff10e9e84bf32be84f9057f6abc75.tar.gz | cut -d ' ' -f1)" = 85ed6be5a3be9ae8928bb6c19b67cd9646779fe7e6dc25e66794d43d9635280a
	mkdir -p "$(BUILD_DIR)/mesa-libclc-rpm/SOURCES" "$(BUILD_DIR)/mesa-libclc-rpm/SPECS"
	cp referances/mesa-libclc-20261024/mesa-libclc-87f66007242ff10e9e84bf32be84f9057f6abc75.tar.gz "$(BUILD_DIR)/mesa-libclc-rpm/SOURCES/"
	cp packaging/uke-mesa-libclc22-spirv.spec "$(BUILD_DIR)/mesa-libclc-rpm/SPECS/"
	rpmbuild -bs --target aarch64 --define '_buildhost uke-builder' --define "_topdir $$(realpath "$(BUILD_DIR)/mesa-libclc-rpm")" "$(BUILD_DIR)/mesa-libclc-rpm/SPECS/uke-mesa-libclc22-spirv.spec"
