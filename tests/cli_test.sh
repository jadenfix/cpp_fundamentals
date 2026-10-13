#!/usr/bin/env bash
set -euo pipefail

binary=${1:-./build/fundamentals}
first=$(mktemp)
second=$(mktemp)
trap 'rm -f "$first" "$second"' EXIT

"$binary" --seed 8257 >"$first"
"$binary" --seed 8257 >"$second"
cmp "$first" "$second"

grep -Fq -- "Rejected: order ID 2001 has already been accepted in this session." "$first"
grep -Fq -- "Rejected: order ID must be positive." "$first"
grep -Fq -- "Resting:1001" "$first"
grep -Fq -- "Resting:1002" "$first"
grep -Fq -- "Conservation: PASS" "$first"

if "$binary" --seed invalid >/dev/null 2>&1; then
    echo "invalid seed unexpectedly succeeded" >&2
    exit 1
fi

echo "Deterministic CLI checks passed."
