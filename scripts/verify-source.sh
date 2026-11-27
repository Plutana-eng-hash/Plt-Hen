#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UPSTREAM="$ROOT/vendor/ps4-hen"
EXPECTED_HEN_COMMIT="8c62f6628827055df0dfece1c4d14e045761288b"

if [[ ! -d "$UPSTREAM/.git" && ! -f "$UPSTREAM/.git" ]]; then
  echo "ERROR: HEN submodule is not initialized."
  echo "Run: git submodule update --init --recursive"
  exit 1
fi

actual="$(git -C "$UPSTREAM" rev-parse HEAD)"
if [[ "$actual" != "$EXPECTED_HEN_COMMIT" ]]; then
  echo "ERROR: unexpected HEN source revision: $actual"
  echo "Expected: $EXPECTED_HEN_COMMIT"
  exit 1
fi

required=(
  "kpayload/include/offsets/1402.h"
  "kpayload/source/offsets/1402.c"
  "kpayload/source/offsets_file.c"
  "installer/include/offsets.h"
  "installer/include/version.h"
  "build.sh"
  "kpayload/Makefile"
  "installer/Makefile"
)

for file in "${required[@]}"; do
  if [[ ! -s "$UPSTREAM/$file" ]]; then
    echo "ERROR: required upstream source missing: $file"
    exit 1
  fi
done

grep -q 'case 1402:' "$UPSTREAM/kpayload/source/offsets_file.c"
grep -q 'MAX_FW 1402' "$UPSTREAM/installer/include/version.h"
grep -q 'K1402_COPYIN_PATCH_1' "$UPSTREAM/installer/include/offsets.h"

echo "Source verification passed."
echo "HEN commit: $actual"
