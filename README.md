# Compression Tool (C++, Huffman Coding)

## What this project is

This is a command line file compressor built from scratch in C++, without
using any external compression libraries. It shrinks files by using
**Huffman coding** It is  a classic algorithm that gives shorter binary codes to
bytes that appear frequently in a file, and longer codes to bytes that are
rare. This is the same core idea real tools like ZIP and gzip are built on.

Alongside Huffman coding, the project also includes:
- **RLE (Run-Length Encoding)** It is  a simpler compression method, included so
  the two can be compared side by side
- **Tree visualization**  to see the actual Huffman tree built for a file, as
  a picture
- **A comparison/report tool** it benchmarks both methods on a file and
  generates a short report

The goal is to demonstrate core data structures and algorithms — hash maps,
binary trees, min-heaps, greedy algorithms, and bit manipulation — through
a real, working, testable tool rather than just theory.

## Build

```bash
make
```

## Basic usage

```bash
# Compress and decompress a file
./compressor compress   input.txt   output.huff
./compressor decompress output.huff restored.txt

# Compare Huffman vs RLE on a file, with a report
./compressor compare input.txt report.md

# Visualize the Huffman tree built for a file
./compressor tree input.txt tree.dot
```

## Run tests

```bash
make test
```

## How it works (short version)

1. Count how often each byte appears in the file.
2. Build a binary tree by repeatedly merging the two least frequent bytes
   (using a min-heap)  common bytes end up near the top, rare bytes end
   up deep in the tree.
3. Walk the tree to assign each byte a binary code: shorter codes for
   common bytes, longer codes for rare ones.
4. Replace every byte in the file with its code and pack the result into
   bytes to write out.
5. To decompress: rebuild the same tree, then walk it bit by bit,
   outputting a byte each time a leaf is reached.

## Compression ratio

```
ratio = 100 * (1 - compressed_size / original_size)
```

0% = no change, higher = better, negative = the file got bigger (this
happens on small files or already-compressed formats like PDFs, JPEGs,
and ZIPs — expected, not a bug).

## Notes

Huffman coding compresses well when some bytes are much more common
  than others (e.g. plain English text). It does poorly on already
  compressed or random data.
RLE compresses well only when the same byte repeats many times *in a
  row*. It can make files bigger otherwise.
Real tools like gzip combine both ideas (and more) for better results
  than either method alone.



## Updated implementation
Uses a custom binary min-heap and deterministic frequency/minimum-byte tie breaking.
Huffman archives now use HUF2 magic; old HUFF archives require the original decoder.
RLE1 is unchanged. Both formats store 32-bit sizes and reject inputs above UINT32_MAX.
Tree command exports DOT; render manually with `dot -Tpng tree.dot -o tree.png`.
`make test` runs C++ unit assertions and CLI round-trip tests.
