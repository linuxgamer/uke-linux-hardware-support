#!/usr/bin/env bash
# COPR only: actual prepared updater callbacks and bounded native child capture.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for variant in optimized sanitized; do
    compiler=g++; flags=(-O2)
    if [[ $variant == sanitized ]]; then
        compiler=clang++; flags=(-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    fi
    "$compiler" -std=c++20 -UNDEBUG -Wall -Wextra -Werror -Wno-misleading-indentation \
        "${flags[@]}" -Wl,--wrap=main "$owner/tests/o82-private-color.cpp" \
        $(pkg-config --cflags --libs openssl libsystemd) -o "$work/$variant"
    "$work/$variant"
done
