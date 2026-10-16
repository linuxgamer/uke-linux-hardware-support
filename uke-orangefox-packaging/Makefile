SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate srpm track-stable audit
version := $(shell jq -r .rpm_version manifests/release-lock.json)
outdir ?= $(CURDIR)/build/srpm
releases_file ?=
validate:
	jq -e '.device == "uke" and .rpm_version == "$(version)" and (.rpm_version|test("uke";"i")|not) and .physical_device == false and (.assets|length == 6) and (.source_snapshots|length == 2) and all(.assets[],.source_snapshots[]; (.sha256|test("^[a-f0-9]{64}$$")) and (.name|test("^[A-Za-z0-9][A-Za-z0-9._-]*$$")) and (.url|startswith("https://github.com/MCC45TR/orangefox_device_xiaomi_uke/releases/download/")))' manifests/release-lock.json >/dev/null
	test "$$(awk '/^Version:/ {print $$2}' packaging/uke-orangefox-recovery.spec)" = "$(version)"
srpm: validate
	tag=$$(jq -er .tag manifests/release-lock.json)
	cache="$(CURDIR)/private/releases/$$tag"
	top="$$(realpath -m "$(outdir)/rpmbuild")"
	mkdir -p "$$cache" "$$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS}
	payload="$$top/payload/uke-orangefox-recovery-$(version)"
	rm -rf "$$top/payload"; mkdir -p "$$payload"
	while IFS=$$'\t' read -r name url expected; do
		if test ! -s "$$cache/$$name"; then
			curl -fL --retry 3 --connect-timeout 20 "$$url" -o "$$cache/$$name.part"
			mv "$$cache/$$name.part" "$$cache/$$name"
		fi
		test "$$(sha256sum "$$cache/$$name" | cut -d ' ' -f1)" = "$$expected"
		cp "$$cache/$$name" "$$payload/"
	done < <(jq -r '.assets[]|[.name,.url,.sha256]|@tsv' manifests/release-lock.json)
	jq -e --arg profile "$$(jq -er .firmware_profile manifests/release-lock.json)" '.device == "uke" and .firmware_profile == $$profile and .validation.payload_privacy == true and .validation.compile == true and .validation.physical_device == false' "$$payload/ARTIFACT-MANIFEST.json" >/dev/null
	cp manifests/release-lock.json "$$payload/"
	cp docs/{DELIVERY,SOURCE-OFFER}.md LICENSE "$$payload/"
	cp -r licenses "$$payload/"
	(cd "$$payload"; find . -type f -print0 | sort -z | xargs -0 sha256sum > ../PACKAGE-SHA256SUMS; mv ../PACKAGE-SHA256SUMS .)
	epoch=$$(date -u -d "$$(jq -er .published_at manifests/release-lock.json)" +%s)
	tar --sort=name --mtime="@$$epoch" --owner=0 --group=0 --numeric-owner -czf "$$top/SOURCES/uke-orangefox-recovery-$(version).tar.gz" -C "$$top/payload" "uke-orangefox-recovery-$(version)"
	cp packaging/uke-orangefox-recovery.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" "$$top/SPECS/uke-orangefox-recovery.spec"
	cp "$$top/SRPMS/"*.src.rpm "$(outdir)/"

# Track only stable releases published by the project that pass its public
# contract. This host target never executes an image, ZIP or donor script.
track-stable:
	mkdir -p build/tracking
	if test -n "$(releases_file)"; then
		cp "$(releases_file)" build/tracking/releases.json
	else
		curl -fsSL --retry 3 'https://api.github.com/repos/MCC45TR/orangefox_device_xiaomi_uke/releases?per_page=100' > build/tracking/releases.json
	fi
	jq '[.[]|select(.draft == false and .prerelease == false)]|sort_by(.published_at)|last' build/tracking/releases.json > build/tracking/candidate.json
	if test "$$(jq -r 'type' build/tracking/candidate.json)" = null; then
		printf '%s\n' 'No stable Uke recovery release is published; the explicit preview pin is retained.'
		exit 0
	fi
	tag=$$(jq -er .tag_name build/tracking/candidate.json)
	if test "$$tag" = "$$(jq -er .tag manifests/release-lock.json)"; then exit 0; fi
	[[ "$$tag" =~ ^r([0-9]+\.[0-9]+)-uke\.([0-9]{8})$$ ]] || { echo 'Stable tag must use rMAJOR.MINOR-uke.YYYYMMDD' >&2; exit 1; }
	new_version="$${BASH_REMATCH[1]}.$${BASH_REMATCH[2]}"
	jq --arg v "$$new_version" --slurpfile old manifests/release-lock.json '
	  . as $$r | {schema_version:1, device:"uke", repository:$$old[0].repository,
	    tag:.tag_name, channel:"stable", prerelease:false, rpm_version:$$v,
	    published_at:.published_at, firmware_profile:$$old[0].firmware_profile,
	    physical_device:false,
	    assets:[$$r.assets[]|select(.name == "ARTIFACT-MANIFEST.json" or .name == "INSTALL.md" or .name == "ORANGEFOX-SOURCE-PINS.xml" or .name == "STOCK-KERNEL-SOURCE.json" or (.name|test("^[A-Za-z0-9][A-Za-z0-9._-]*[.](img|zip)$$")))|
	      {name, url:.browser_download_url, sha256:(.digest|ltrimstr("sha256:")), bytes:.size}],
	    source_snapshots:[$$old[0].source_snapshots[].name as $$n | $$r.assets[]|select(.name == $$n)|
	      {name, url:.browser_download_url, sha256:(.digest|ltrimstr("sha256:")), bytes:.size}]}' build/tracking/candidate.json > build/tracking/release-lock.json
	jq -e '(.assets|length == 6) and (.source_snapshots|length == 2) and all(.assets[], .source_snapshots[]; (.sha256|test("^[a-f0-9]{64}$$")) and (.url|startswith("https://github.com/MCC45TR/orangefox_device_xiaomi_uke/releases/download/")))' build/tracking/release-lock.json >/dev/null
	url=$$(jq -er '.assets[]|select(.name == "ARTIFACT-MANIFEST.json")|.url' build/tracking/release-lock.json)
	curl -fL --retry 3 "$$url" -o build/tracking/artifact-manifest.json
	test "$$(sha256sum build/tracking/artifact-manifest.json|cut -d ' ' -f1)" = "$$(jq -er '.assets[]|select(.name == "ARTIFACT-MANIFEST.json")|.sha256' build/tracking/release-lock.json)"
	jq -e --arg profile "$$(jq -er .firmware_profile manifests/release-lock.json)" '.schema_version == 1 and .device == "uke" and .firmware_profile == $$profile and (.classification == "stable" or .classification == "reviewed-stable") and .validation.compile == true and .validation.header_sections == true and .validation.zip_integrity == true and .validation.static_installer == true and .validation.payload_privacy == true and .validation.source_identification == true' build/tracking/artifact-manifest.json >/dev/null
	cp build/tracking/release-lock.json manifests/release-lock.json
	sed -i "s/^Version:.*/Version: $$new_version/; s/^Release:.*/Release: 1%{?dist}/" packaging/uke-orangefox-recovery.spec
	$(MAKE) validate

audit: srpm
	tag=$$(jq -er .tag manifests/release-lock.json)
	cache="$(CURDIR)/private/releases/$$tag"
	mkdir -p build/audit
	# Android boot header v4 has a 4096-byte page and a kernel-size field at 8.
	while IFS= read -r image; do
		test "$$(head -c8 "$$cache/$$image")" = 'ANDROID!'
		test "$$(od -An -tu4 -j40 -N4 "$$cache/$$image"|tr -d ' ')" = 4
		kernel=$$(od -An -tu4 -j8 -N4 "$$cache/$$image"|tr -d ' ')
		ramdisk=$$(od -An -tu4 -j12 -N4 "$$cache/$$image"|tr -d ' ')
		expected_size=$$(jq -er .recovery_ramdisk.bytes "$$cache/ARTIFACT-MANIFEST.json")
		test "$$ramdisk" = "$$expected_size"; test "$$ramdisk" -gt 0
		offset=$$((4096 + ((kernel + 4095) / 4096) * 4096))
		dd if="$$cache/$$image" of=build/audit/ramdisk.lz4 iflag=skip_bytes,count_bytes skip="$$offset" count="$$ramdisk" status=none
		test "$$(sha256sum build/audit/ramdisk.lz4|cut -d ' ' -f1)" = "$$(jq -er .recovery_ramdisk.sha256 "$$cache/ARTIFACT-MANIFEST.json")"
	done < <(jq -r '.assets[].name|select(endswith(".img"))' manifests/release-lock.json)
	rm -rf build/audit/extracted; mkdir -p build/audit/extracted
	lz4 -dc build/audit/ramdisk.lz4 | cpio -it --quiet > build/audit/files.txt
	if rg '(^/|(^|/)\.\.(/|$$))' build/audit/files.txt; then exit 1; fi
	(cd build/audit/extracted; lz4 -dc ../ramdisk.lz4 | cpio -idm --quiet)
	test -s build/audit/extracted/system/bin/recovery
	if rg -a -l --hidden --no-ignore '/home/[^/[:space:]]+/|/Users/[^/[:space:]]+/' build/audit/extracted; then exit 1; fi
	while IFS= read -r archive; do unzip -t "$$cache/$$archive"; done < <(jq -r '.assets[].name|select(endswith(".zip"))' manifests/release-lock.json)
	printf '%s\n' 'Pinned image ramdisk, nonempty extracted payload, privacy and ZIP integrity passed.'
