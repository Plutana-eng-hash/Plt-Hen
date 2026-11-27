#!/usr/bin/env bash
set -euo pipefail

: "${OO_PS4_TOOLCHAIN:?Set OO_PS4_TOOLCHAIN to the extracted OpenOrbis toolchain directory}"
TOOLCHAIN="$(cd "$OO_PS4_TOOLCHAIN" && pwd)"
BUILD_DIR="${1:-build}"
mkdir -p "$BUILD_DIR"

for required in link.x include lib/crt1.o; do
  if [[ ! -e "$TOOLCHAIN/$required" ]]; then
    echo "OpenOrbis toolchain is incomplete: missing $required" >&2
    exit 2
  fi
done

TARGET="x86_64-pc-freebsd12-elf"
COMMON_FLAGS=(--target="$TARGET" -fPIC -funwind-tables -O2 -Wall -Wextra -Werror
  -isysroot "$TOOLCHAIN" -isystem "$TOOLCHAIN/include" -Iinclude)
CXX_FLAGS=("${COMMON_FLAGS[@]}" -isystem "$TOOLCHAIN/include/c++/v1")

clang++ "${CXX_FLAGS[@]}" -c src/main.cpp -o "$BUILD_DIR/main.o"
clang++ "${CXX_FLAGS[@]}" -c src/core.cpp -o "$BUILD_DIR/core.o"\nclang++ "${CXX_FLAGS[@]}" -c src/plugin_store.cpp -o "$BUILD_DIR/plugin_store.o"

ld.lld -m elf_x86_64 -pie --script "$TOOLCHAIN/link.x" --eh-frame-hdr \
  -L"$TOOLCHAIN/lib" "$BUILD_DIR/main.o" "$BUILD_DIR/core.o" \
  -lSceVideoOut -lScePad -lSceUserService -lc -lkernel -lc++ "$TOOLCHAIN/lib/crt1.o" \
  -o "$BUILD_DIR/Vypex-Cr_ck.elf"

file "$BUILD_DIR/Vypex-Cr_ck.elf"
