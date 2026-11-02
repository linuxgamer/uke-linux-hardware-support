SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
outdir ?= $(CURDIR)/build/srpm
TARGET_ARCH ?= aarch64
.PHONY: srpm
srpm:
	bash prepare.sh '$(outdir)' '$(TARGET_ARCH)'
