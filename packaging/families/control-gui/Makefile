SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
outdir ?= $(CURDIR)/build/$(PHASE)/srpm
TARGET_ARCH ?= aarch64
PHASE ?= backend
.PHONY: srpm
srpm:
	bash prepare.sh '$(outdir)' '$(TARGET_ARCH)' '$(PHASE)'
