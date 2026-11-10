SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.DEFAULT_GOAL := validate
fedora ?= $(shell rpm --eval '%{fedora}')
SPEAKER_PRESET ?= $(CURDIR)/referances/speaker-preset-3.0.304.0/fs19xx.fsm
SPEAKER_NOTICE ?= $(CURDIR)/referances/speaker-preset-3.0.304.0/odm-NOTICE.xml.gz
.PHONY: validate srpm
.PHONY: speaker-srpm
speaker-srpm:
	input='$(SPEAKER_PRESET)'; notice='$(SPEAKER_NOTICE)'
	[[ -f $$input && ! -L $$input && -f $$notice && ! -L $$notice ]]
	data=packaging/speaker-firmware
	bash src/audit-speaker-source.sh "$$input" "$$notice" "$$data"
	top="$$(realpath -m '$(outdir)/speaker-rpmbuild')"
	stage="$$top/stage/uke-speaker-firmware-3.0.304.0"
	[[ ! -e $$top ]]
	mkdir -p "$$stage" "$$top"/{SOURCES,SPECS,SRPMS}
	install -m644 "$$input" "$$stage/fs19xx.fsm"
	install -m644 "$$notice" "$$stage/odm-NOTICE.xml.gz"
	install -m644 "$$data/README.md" "$$data/provenance.json" "$$data/LICENSE.Apache-2.0" "$$stage/"
	tar --sort=name --mtime=@1793059200 --owner=0 --group=0 --numeric-owner -cJf "$$top/SOURCES/uke-speaker-firmware-3.0.304.0.tar.xz" -C "$$top/stage" uke-speaker-firmware-3.0.304.0
	install -m644 "$$data/uke-speaker-firmware.spec" "$$top/SPECS/"
	rpmbuild -bs --define "_topdir $$top" --define '_buildhost uke-copr-source' "$$top/SPECS/uke-speaker-firmware.spec"
validate:
	jq -e '.schema_version==1 and .device=="uke" and .hardware_tested==false and .redistribution_review_complete==false and .firmware_blob_package_ready==false and .payload_files==218 and .installed_hashes_verified==218 and .exact_notice_matches==7 and (.notice_mapped_files|length)==.exact_notice_matches and ([.notice_mapped_files[].notice_path]|unique|length)==.exact_notice_matches and (.exact_notice_matches+.missing_original_notice_paths+.unmatched_named_notice_paths)==.payload_files and all(.notice_mapped_files[]; (.sha256|test("^[a-f0-9]{64}$$")) and .content_ids==["3b83ef96387f14655fc854ddc3c6bd57"])' manifests/firmware-notice-review-20261030.json >/dev/null
	test "$$(sha256sum packaging/speaker-firmware/LICENSE.Apache-2.0 | cut -d' ' -f1)" = "$$(jq -er .license_evidence.license_sha256 manifests/firmware-notice-review-20261030.json)"
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | length > 0)' manifests/component.json >/dev/null

	jq -e '.schema_version==1 and .device=="uke" and (.admitted_files|length)==0 and .firmware_blob_package_ready==false' manifests/firmware-lock.json >/dev/null
	bash -n src/audit-admission.sh
	bash -n src/stage-local.sh
	jq -e '.schema_version==1 and .device=="uke" and .soc=="SM7675" and .public_upload_allowed==false and (.files|length)==75 and all(.files[]; .redistribution_review_complete==false)' manifests/local-firmware-candidates.json >/dev/null
	bash -n src/verify-oem-304.sh
	jq -e '.schema_version==1 and .device=="uke" and .soc=="SM7675" and .source.release=="OS3.0.304.0.WOZMIXM" and .public_upload_allowed==false and .redistribution_review_complete==false and .hardware_tested==false and (.missing_sensor_configs|sort)==["ak7323.json","sm7675_ak7323.json"] and (.files|length)==143 and all(.files[]; .source_group=="oem" and .redistribution_review_complete==false)' manifests/oem-runtime-source.json >/dev/null
srpm:
	@echo 'No firmware payload is admitted for public source packaging; complete file-level redistribution gates.' >&2
	@exit 1
# A lock with no admitted bytes can validate structurally, but never build a
# firmware RPM. There is no metadata-only substitute for a firmware payload.
audit-admission:
	bash src/audit-admission.sh "$(LOCK)" "$(ARCHIVE)" "$(REQUESTS)"
.PHONY: audit-admission test-admission
test-admission: validate
	bash tests/admission.sh
	bash tests/stage-local.sh
	bash tests/copr-source.sh
.PHONY: stage-local
stage-local: validate
	bash src/stage-local.sh manifests/local-firmware-candidates.json "$(MODEM_FILES)" "$(GPU_FILES)" "$(OUTPUT)"
.PHONY: stage-oem-local
stage-oem-local: validate
	bash src/stage-local.sh manifests/oem-runtime-source.json "$(OEM_FILES)" "$(OUTPUT)"
.PHONY: local-rpms
outdir ?= $(CURDIR)/build/local-rpms
local-rpms: validate
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	[[ ! -e $$top ]]
	mkdir -p "$$top"/{SOURCES,SPECS,RPMS,BUILD,BUILDROOT}
	bash src/stage-local.sh manifests/oem-runtime-source.json "$(OEM_FILES)" "$$top/input"
	bash src/stage-local.sh manifests/local-firmware-candidates.json "$(MODEM_FILES)" "$(GPU_FILES)" "$$top/legacy"
	mv "$$top/input/local-firmware-candidates.json" "$$top/input/oem-runtime-source.json"
	cp "$$top/legacy/local-firmware-candidates.json" "$$top/input/"
	# Reject overlapping payloads before combining the two verified source groups.
	while IFS= read -r -d '' path; do [[ ! -e "$$top/input/$${path#./}" ]]; done < <(cd "$$top/legacy" && find usr -type f -print0)
	cp -a "$$top/legacy/usr/." "$$top/input/usr/"
	(cd "$$top/input" && find . -type f ! -name MANIFEST.sha256 -print0 | LC_ALL=C sort -z | xargs -0 sha256sum > MANIFEST.sha256)
	tar --sort=name --mtime=@1792454400 --owner=0 --group=0 --numeric-owner -cJf "$$top/SOURCES/xiaomi-uke-firmware-3.0.304.0.tar.xz" -C "$$top/input" .
	cp packaging/xiaomi-uke-firmware.spec "$$top/SPECS/"
	rpmbuild -bb --define "_topdir $$top" --define '_buildhost uke-local-builder' "$$top/SPECS/xiaomi-uke-firmware.spec"
	install -m644 "$$top/RPMS/noarch/"*.rpm '$(outdir)/'
	(cd '$(outdir)' && sha256sum *.noarch.rpm > SHA256SUMS)
.PHONY: copr-srpm
# Explicit publisher-requested path; never changes the historical license review.
copr-srpm: local-rpms
	[[ '$(PUBLICATION_REQUESTED)' == yes ]]
	top="$$(realpath -e '$(outdir)/rpmbuild')"
	for pair in 'vendor ce8281d6fed48c89db41fc6ba79d6a2502e1c4406e14cbf0e4e9f2c739a52c05' 'odm 71aef2c9cb076992714858bd3861590e7329c82098501c19a7075b00c9540727'; do
		read -r partition expected <<< "$$pair"
		file='$(NOTICE_FILES)'/$$partition-NOTICE.xml.gz
		[[ -f $$file && ! -L $$file && $$(sha256sum "$$file" | cut -d' ' -f1) == "$$expected" ]]
		install -m644 "$$file" "$$top/input/"
	done
	rm -- "$$top/input/LOCAL-ONLY.txt"
	printf '%s\n' '20 October 2026. Publisher-requested COPR firmware submission. Historical source manifests retain their original local-review state. File-level redistribution review remains incomplete; attached OEM notices and a proprietary label do not establish additional grants. No private calibration, factory address or persist contents are included. Source/package checks do not establish hardware acceptance.' > "$$top/input/PUBLICATION.txt"
	(cd "$$top/input" && find . -type f ! -name MANIFEST.sha256 -print0 | LC_ALL=C sort -z | xargs -0 sha256sum > MANIFEST.sha256)
	tar --sort=name --mtime=@1792454400 --owner=0 --group=0 --numeric-owner -cJf "$$top/SOURCES/uke-firmware-3.0.304.0.tar.xz" -C "$$top/input" .
	cp packaging/uke-firmware.spec "$$top/SPECS/"
	rpmbuild -bs --define "_topdir $$top" --define '_buildhost uke-copr-source' "$$top/SPECS/uke-firmware.spec"
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'
verify-oem-304:
	bash src/verify-oem-304.sh
image-data-srpm: verify-oem-304
	root=payload/oem-os3.0.304.0
	top="$$(realpath -m build/fedora$(fedora)-image-data/rpmbuild)"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	tar -C "$$root" --sort=name --mtime='@0' --owner=0 --group=0 --numeric-owner \
	    --transform='s#^#uke-fedora$(fedora)-oem-data-3.0.304.0/#' \
	    -czf "$$top/SOURCES/uke-fedora$(fedora)-oem-data-3.0.304.0.tar.gz" ssc bt firmware
	cp packaging/uke-oem-data.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target noarch --define "_topdir $$top" \
	    "$$top/SPECS/uke-oem-data.spec"
