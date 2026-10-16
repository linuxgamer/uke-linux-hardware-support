SHELL := /bin/bash
.PHONY: validate srpm
validate:
	jq -e '.schema_version==1 and .device=="uke" and (.admitted_files|length)==0 and .firmware_blob_package_ready==false' manifests/firmware-lock.json >/dev/null
	bash -n src/audit-admission.sh
srpm:
	@echo 'No validated Uke package payload yet; complete the manifest acceptance gates.' >&2
	@exit 1
# A lock with no admitted bytes can validate structurally, but never build a
# firmware RPM. There is no metadata-only substitute for a firmware payload.
audit-admission:
	bash src/audit-admission.sh "$(LOCK)" "$(ARCHIVE)" "$(REQUESTS)"
.PHONY: audit-admission test-admission
test-admission: validate
	bash tests/admission.sh
