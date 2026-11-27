#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UPSTREAM="$ROOT/vendor/ps4-hen"
HEN_COMMIT="8c62f6628827055df0dfece1c4d14e045761288b"
SDK_COMMIT="fd4f7299d1da5f5601c164a02b6d6d019ac8630a"
SDK_URL="https://github.com/Scene-Collective/ps4-payload-sdk.git"
SDK_DIR="${PS4SDK_ROOT:-$ROOT/.deps/ps4-payload-sdk}"
OUT="$ROOT/out"

command -v git >/dev/null
command -v make >/dev/null
command -v gcc >/dev/null
command -v objcopy >/dev/null
command -v xxd >/dev/null
command -v curl >/dev/null
command -v unzip >/dev/null

bash "$ROOT/scripts/verify-source.sh"

if [[ ! -d "$SDK_DIR/.git" ]]; then
  mkdir -p "$(dirname "$SDK_DIR")"
  git clone "$SDK_URL" "$SDK_DIR"
fi

git -C "$SDK_DIR" fetch --depth 1 origin "$SDK_COMMIT"
git -C "$SDK_DIR" checkout --detach "$SDK_COMMIT"

echo "Building pinned PS4 Payload SDK..."
make -C "$SDK_DIR/libPS4" clean
make -C "$SDK_DIR/libPS4" -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"

export PS4SDK="$SDK_DIR"

echo "Building upstream PS4-HEN payload..."
(
  cd "$UPSTREAM"
  bash ./build.sh
)

SOURCE_BIN="$UPSTREAM/hen.bin"
if [[ ! -s "$SOURCE_BIN" ]]; then
  echo "ERROR: upstream build did not produce a non-empty hen.bin."
  exit 1
fi

# Afterglow's loader documentation identifies a raw HEN payload by its
# JMP-rel32 (0xe9) entry byte. Refuse to package an unexpected output format.
first_byte="$(od -An -N1 -tx1 "$SOURCE_BIN" | tr -d ' \n')"
if [[ "$first_byte" != "e9" ]]; then
  echo "ERROR: unexpected payload header: 0x$first_byte (expected raw payload byte e9)."
  file "$SOURCE_BIN" || true
  exit 1
fi

rm -rf "$OUT"
mkdir -p "$OUT"
cp "$SOURCE_BIN" "$OUT/Plutana-HEN-14.02.bin"
(
  cd "$OUT"
  sha256sum "Plutana-HEN-14.02.bin" > "Plutana-HEN-14.02.bin.sha256"
)

cat > "$OUT/build-info.txt" <<EOF
Project: Plutana-HEN
Output: Plutana-HEN-14.02.bin
Format: raw PS4 HEN payload (not ELF, SELF, PKG, or browser exploit)
Firmware target added by source: 14.02
Upstream HEN: Scene-Collective/ps4-hen
Upstream HEN commit: $HEN_COMMIT
Payload SDK: Scene-Collective/ps4-payload-sdk
Payload SDK commit: $SDK_COMMIT
Upstream build script: vendor/ps4-hen/build.sh
Plugin dependency: upstream latest ps4-hen-plugins release (not commit-pinned)
Physical console test: NOT PERFORMED BY CI
EOF

echo
echo "Build completed."
ls -lh "$OUT"
cat "$OUT/Plutana-HEN-14.02.bin.sha256"
