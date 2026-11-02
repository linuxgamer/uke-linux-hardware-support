SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
outdir ?= $(CURDIR)/build/srpm
.PHONY: srpm
srpm:
	$(MAKE) -C ../control-gui srpm PHASE=metas outdir='$(outdir)'
