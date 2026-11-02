SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate srpm
outdir ?= $(CURDIR)/build
validate:
	bash tests/source-contracts.sh
srpm: validate
	root="$(CURDIR)"
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$root/referances" "$$top"/{SOURCES,SPECS,SRPMS}
	jq -r '.inputs[]|[.filename,.url,.sha256]|@tsv' source-lock.json >"$$top/inputs.tsv"
	while IFS=$$'\t' read -r filename url sha; do
		cached="$$root/referances/$$filename"
		if [[ ! -s $$cached ]]; then
			curl --fail --location --silent --show-error --retry 3 --max-time 300 --proto '=https' --proto-redir '=https' "$$url" -o "$$cached.part"
			mv "$$cached.part" "$$cached"
		fi
		printf '%s  %s\n' "$$sha" "$$cached" | sha256sum --check -
		install -m644 "$$cached" "$$top/SOURCES/$$filename"
	done <"$$top/inputs.tsv"
	install -m644 freecad.spec "$$top/SPECS/"
	install -m644 source-lock.json "$$top/SOURCES/freecad-source-lock.json"
	epoch=$$(jq -er .source_date_epoch source-lock.json)
	touch --date="@$$epoch" "$$top/SOURCES/"* "$$top/SPECS/"*
	SOURCE_DATE_EPOCH=$$epoch rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" --define '_buildhost freecad-source' --define 'use_source_date_epoch_as_buildtime 1' "$$top/SPECS/freecad.spec"
	mkdir -p '$(outdir)'
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'
