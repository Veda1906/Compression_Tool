#!/usr/bin/env bash
# Round-trip and sanity tests for the compressor.
set -u
cd "$(dirname "$0")/.."

BIN=./compressor
PASS=0
FAIL=0

roundtrip() {
    local mode_compress="$1" mode_decompress="$2" file="$3" ext="$4"
    local packed="${file}.${ext}"
    local restored="${file}.${ext}.out"

    "$BIN" "$mode_compress" "$file" "$packed" > /dev/null
    "$BIN" "$mode_decompress" "$packed" "$restored" > /dev/null

    if cmp -s "$file" "$restored"; then
        echo "PASS: $mode_compress/$mode_decompress on $file"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $mode_compress/$mode_decompress on $file"
        FAIL=$((FAIL + 1))
    fi
    rm -f "$packed" "$restored"
}

# Prepare fixtures, including edge cases
mkdir -p samples
[ -f samples/empty.txt ]  || : > samples/empty.txt
[ -f samples/single.txt ] || printf 'x' > samples/single.txt
[ -f samples/repeat.txt ] || python3 -c "print('a' * 5000)" > samples/repeat.txt
[ -f samples/test1.txt ]  || echo "the quick brown fox jumps over the lazy dog" > samples/test1.txt

for f in samples/*.txt; do
    [ -f "$f" ] || continue
    roundtrip compress decompress "$f" huff
    roundtrip rle-compress rle-decompress "$f" rle
done

# Smoke-test tree and compare commands (just check they exit 0)
if "$BIN" tree samples/test1.txt /tmp/_ct_test_tree.dot > /dev/null 2>&1; then
    echo "PASS: tree command"
    PASS=$((PASS + 1))
else
    echo "FAIL: tree command"
    FAIL=$((FAIL + 1))
fi
rm -f /tmp/_ct_test_tree.dot /tmp/_ct_test_tree.png

if "$BIN" compare samples/test1.txt /tmp/_ct_test_report.md > /dev/null 2>&1; then
    echo "PASS: compare command"
    PASS=$((PASS + 1))
else
    echo "FAIL: compare command"
    FAIL=$((FAIL + 1))
fi
rm -f /tmp/_ct_test_report.md

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
