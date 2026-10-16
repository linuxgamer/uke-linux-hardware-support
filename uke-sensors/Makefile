SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate srpm test-native test-cache test-hexagonrpc test-touch-normal test-touch-thp touchscreen-rpm
PACKAGE ?= libssc
outdir ?= $(CURDIR)/build/srpm/$(PACKAGE)
validate:
	for spec in packaging/{libssc,iio-sensor-proxy,uke-ssc-fileservice}/*.spec; do
		test -f "$$spec"
		if grep -E '^(Version|Release):.*[Uu][Kk][Ee]' "$$spec"; then exit 1; fi
	done
srpm: validate
	case '$(PACKAGE)' in libssc|iio-sensor-proxy|uke-ssc-fileservice) ;; *) echo 'Unreviewed sensor package' >&2; exit 1;; esac
	case '$(PACKAGE)' in
	libssc) archive=referances/libssc/libssc-v0.4.4.tar.gz; url=https://codeberg.org/DylanVanAssche/libssc/archive/v0.4.4.tar.gz; hash=716d6bd6b34d2d753060c6b54c9a87e34fae75b724c763bf9ef487efa3621587; name=libssc-v0.4.4.tar.gz;;
	iio-sensor-proxy) archive=referances/iio-sensor-proxy/source/iio-sensor-proxy-3.9.tar.bz2; url=https://gitlab.freedesktop.org/hadess/iio-sensor-proxy/-/archive/3.9/iio-sensor-proxy-3.9.tar.bz2; hash=19a4ae99b1b8af603fc789841025f9747bf445418d69ec7389e75adb6c8186bf; name=iio-sensor-proxy-3.9.tar.bz2;;
	uke-ssc-fileservice) archive=referances/hexagonrpc-write/source.tar.gz; url=https://api.github.com/repos/linux-msm/hexagonrpc/tarball/510c87e5c89026ee2bd2034776fc4d15994f353d; hash=ed7b0e704a5e8c20b77b013b4b34f3cddbe41d4cf546edc73f029501f7ebdec5; name=hexagonrpc-510c87e5.tar.gz;;
	esac
	mkdir -p "$$(dirname "$$archive")"
	if test ! -s "$$archive"; then curl -fL --retry 3 --connect-timeout 20 "$$url" -o "$$archive.part"; mv "$$archive.part" "$$archive"; fi
	test "$$(sha256sum "$$archive" | cut -d ' ' -f1)" = "$$hash"
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	cp "$$archive" "$$top/SOURCES/$$name"
	case '$(PACKAGE)' in
	libssc) cp patches/libssc/*.patch "$$top/SOURCES/";;
	iio-sensor-proxy) cp patches/iio-sensor-proxy/*.patch packaging/selinux/uke-ssc.cil "$$top/SOURCES/";;
	uke-ssc-fileservice) cp patches/hexagonrpc/*.patch packaging/systemd/*.service packaging/systemd/iio-ssc-order.conf packaging/uke-ssc.conf tests/hexagon-registry.cpp tests/hexagon-shell.c "$$top/SOURCES/";;
	esac
	cp packaging/$(PACKAGE)/$(PACKAGE).spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" "$$top/SPECS/$(PACKAGE).spec"
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'
touchscreen-rpm:
	test "$$(uname -m)" = aarch64
	test -s src/upstream/nt36532-thp/nvt_touch_core.cpp
	top="$$(realpath -m build/rpmbuild)"; mkdir -p "$$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS}
	tar --sort=name --mtime=@1791024250 --owner=0 --group=0 --numeric-owner \
	 --transform='s#^#uke-touchscreen-0.1.0/#' -cJf "$$top/SOURCES/uke-touchscreen-0.1.0.tar.xz" \
	 src/touch-* src/upstream/nt36532-thp src/units src/modules-load.d src/modprobe.d src/udev tests/touch-input.cpp tests/touch-normal-probe.cpp LICENSE.GPL-3.0
	cp packaging/uke-touchscreen.spec "$$top/SPECS/"
	rpmbuild -ba --define "_topdir $$(realpath -m build/rpmbuild)" "$$(realpath -m build/rpmbuild)/SPECS/uke-touchscreen.spec"
test-native:
	bash tests/source-contract.sh
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/ssc-discovery.cpp -o build/ssc-contract
	./build/ssc-contract
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/ssc-discovery.cpp -o build/uke-sensor-discovery
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/ssc-sample.cpp -o build/ssc-sample-contract
	./build/ssc-sample-contract
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/ssc-sample.cpp -o build/uke-sensor-sample
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-readonly.cpp -o build/touch-contract
	./build/touch-contract
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/touch-readonly.cpp -o build/uke-touch-readonly
test-cache:
	test "$$(id -u)" = 0
	bash tests/ssc-cache.sh

# Host qualification only; this target neither installs nor starts a tablet daemon.
test-hexagonrpc:
	printf '%s\n' 'ed7b0e704a5e8c20b77b013b4b34f3cddbe41d4cf546edc73f029501f7ebdec5  referances/hexagonrpc-write/source.tar.gz' | sha256sum -c -
	mkdir -p build/hexagonrpc-source build/hexagonrpc
	tar -xf referances/hexagonrpc-write/source.tar.gz --strip-components=1 -C build/hexagonrpc-source
	for patch in patches/hexagonrpc/*.patch; do patch --batch -p1 -d build/hexagonrpc-source < "$$patch" || exit; done
	(cd build/hexagonrpc-source && $(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -DHEXAGONRPC_VERBOSE -I include libhexagonrpc/*.c libhexagonrpc/interface/*.c hexagonrpcd/*.c hexagonrpcd/interface/*.c -o ../hexagonrpc/hexagonrpcd)
	(cd build/hexagonrpc-source && $(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -I include tests/test_hexagonrpc.c libhexagonrpc/hexagonrpc.c -o ../hexagonrpc/test-rpc)
	./build/hexagonrpc/test-rpc
	(cd build/hexagonrpc-source && $(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -I include tests/test_iobuffer.c hexagonrpcd/iobuffer.c -o ../hexagonrpc/test-buffer)
	./build/hexagonrpc/test-buffer
	(cd build/hexagonrpc-source && $(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -I include tests/test_hexagonfs.c hexagonrpcd/hexagonfs.c hexagonrpcd/hexagonfs_mapped.c -o ../hexagonrpc/test-files)
	./build/hexagonrpc/test-files build/hexagonrpc-source/tests/sample_file.txt
	(cd build/hexagonrpc-source && for name in hexagonfs hexagonfs_mapped hexagonfs_virt_dir rpcd_builder; do $(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -I include -c "hexagonrpcd/$$name.c" -o "../hexagonrpc/$$name.o" || exit; done)
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror -I build/hexagonrpc-source/hexagonrpcd tests/hexagon-registry.cpp build/hexagonrpc/{hexagonfs,hexagonfs_mapped,hexagonfs_virt_dir,rpcd_builder}.o -o build/hexagonrpc/test-registry
	./build/hexagonrpc/test-registry

# Host qualification needs the privately retained normal CSOT image; never an MP image.
test-touch-normal:
	test -n "$(TOUCH_NORMAL_FIRMWARE)"
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-normal-probe.cpp -lcrypto -o build/touch-normal-contract
	./build/touch-normal-contract "$(TOUCH_NORMAL_FIRMWARE)"
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/touch-normal-probe.cpp -lcrypto -o build/uke-touch-normal-probe

# Laboratory only: private OEM calibration/reference packets are not RPM payloads.
test-touch-thp:
	test -n "$(TOUCH_CORE_REFERENCE)" -a -n "$(TOUCH_FRAME_REFERENCE)" -a -n "$(TOUCH_CONFIG)"
	bash src/prepare-touch-core.sh "$(TOUCH_CORE_REFERENCE)"
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-thp.cpp src/upstream/nt36532-thp/nvt_touch_core.cpp -lcrypto -o build/touch-thp-contract
	./build/touch-thp-contract "$(TOUCH_FRAME_REFERENCE)" "$(TOUCH_CONFIG)"
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-input.cpp src/upstream/nt36532-thp/nvt_touch_core.cpp -lcrypto -o build/touch-input-contract
	./build/touch-input-contract
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-replay.cpp src/upstream/nt36532-thp/nvt_touch_core.cpp -lcrypto -o build/touch-replay
	bash tests/touch-replay.sh ./build/touch-replay "$(TOUCH_CONFIG)" "$(TOUCH_FRAME_REFERENCE)"
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/touch-thp.cpp src/upstream/nt36532-thp/nvt_touch_core.cpp -lcrypto -o build/uke-touch-thp
