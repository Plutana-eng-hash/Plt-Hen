#!/usr/bin/env bash
set -euo pipefail

: "${OO_PS4_TOOLCHAIN:?Set OO_PS4_TOOLCHAIN to the OpenOrbis toolchain root}"
: "${GOLDHEN_SDK:?Set GOLDHEN_SDK to a cloned GoldHEN_Plugins_SDK directory}"

TOOLCHAIN="$(cd "$OO_PS4_TOOLCHAIN" && pwd)"
GH_SDK="$(cd "$GOLDHEN_SDK" && pwd)"
BUILD_DIR="${1:-build-prx}"
mkdir -p "$BUILD_DIR"

for required in link.x include lib/crt1.o; do
  [[ -e "$TOOLCHAIN/$required" ]] || { echo "Missing OpenOrbis item: $required" >&2; exit 2; }
done
[[ -f "$GH_SDK/Makefile" ]] || { echo "GoldHEN SDK Makefile missing" >&2; exit 2; }

make -C "$GH_SDK" clean all OO_PS4_TOOLCHAIN="$TOOLCHAIN"

TARGET="x86_64-pc-freebsd12-elf"
COMMON=(--target="$TARGET" -fPIC -funwind-tables -O2 -Wall -Wextra -Werror
  -isysroot "$TOOLCHAIN" -isystem "$TOOLCHAIN/include" -I"$GH_SDK/include")

clang++ "${COMMON[@]}" -c src/vypex_prx.cpp -o "$BUILD_DIR/vypex_prx.o"

ld.lld -m elf_x86_64 -pie --script "$TOOLCHAIN/link.x" -e _init --eh-frame-hdr
  -L"$TOOLCHAIN/lib" -L"$GH_SDK" "$GH_SDK/build/crtprx.o" "$BUILD_DIR/vypex_prx.o"
  -lGoldHEN_Hook -lSceLibcInternal -lkernel -lSceSysmodule
  -o "$BUILD_DIR/Vypex-Cr_ck.prx.elf"

mkdir -p "$BUILD_DIR/out"
"$TOOLCHAIN/bin/linux/create-fself" -in="$BUILD_DIR/Vypex-Cr_ck.prx.elf"
  -out="$BUILD_DIR/out/Vypex-Cr_ck.prx" --lib="$BUILD_DIR/out/Vypex-Cr_ck.prx"
  --paid 0x3800000000000011

file "$BUILD_DIR/out/Vypex-Cr_ck.prx"
