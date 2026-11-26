// Huffman-coding based file compression tool, plus:
//   - RLE (run-length encoding) as a second method for comparison
//   - Huffman tree visualization (ASCII + Graphviz DOT/PNG) for demos
//   - A `compare` command that benchmarks methods and writes a report
//
// Usage:
//   compressor compress       <input> <output>
//   compressor decompress     <input> <output>
//   compressor rle-compress   <input> <output>
//   compressor rle-decompress <input> <output>
//   compressor tree           <input> [output.dot]
//   compressor compare        <input> [report.md]

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <iterator>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <algorithm>

#include "bitio.hpp"
#include "huffman.hpp"
#include "crc32.hpp"
#include "huffman_codec.hpp"
#include "rle.hpp"
#include "tree_viz.hpp"

namespace {

std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open file: " + path);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),
                                 std::istreambuf_iterator<char>());
}

void writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot write file: " + path);
    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
    file.close();
    if (!file) throw std::runtime_error("Write failed: " + path);
}

// Compression ratio as "percent smaller than the original".
// 0% = no change, 90% = shrunk to a tenth of the size, negative = grew.
double compressionRatioPercent(size_t originalSize, size_t compressedSize) {
    if (originalSize == 0) return 0.0;
    return 100.0 * (1.0 - static_cast<double>(compressedSize) / static_cast<double>(originalSize));
}

// Average bits/byte needed by an ideal entropy coder for this data
// (Shannon entropy). Huffman coding gets close to this bound; it's a
// useful reference point when explaining *why* a file compressed well
// or poorly.
double shannonEntropyBitsPerByte(const std::unordered_map<uint8_t, uint64_t>& freq, size_t total) {
    if (total == 0) return 0.0;
    double entropy = 0.0;
    for (const auto& entry : freq) {
        double p = static_cast<double>(entry.second) / static_cast<double>(total);
        entropy -= p * std::log2(p);
    }
    return entropy;
}

} // namespace

// ---------------------------------------------------------------------
// compress / decompress (Huffman) - real file I/O via huffcodec
// ---------------------------------------------------------------------

void compress(const std::string& inputPath, const std::string& outputPath) {
    std::vector<uint8_t> data = readFile(inputPath);
    std::vector<uint8_t> out = huffcodec::encode(data);
    writeFile(outputPath, out);

    std::cout << "Original size:     " << data.size() << " bytes\n";
    std::cout << "Compressed size:   " << out.size() << " bytes\n";
    std::cout << "Compression ratio: " << std::fixed << std::setprecision(2)
               << compressionRatioPercent(data.size(), out.size()) << "%\n";
}

void decompress(const std::string& inputPath, const std::string& outputPath) {
    std::vector<uint8_t> in = readFile(inputPath);
    bool checksumOk = false;
    std::vector<uint8_t> output = huffcodec::decode(in, &checksumOk);

    if (!checksumOk) {
        std::cerr << "WARNING: checksum mismatch - output may be corrupted.\n";
    } else {
        std::cout << "Checksum verified OK.\n";
    }

    writeFile(outputPath, output);
    std::cout << "Decompressed size: " << output.size() << " bytes -> " << outputPath << "\n";
}

// ---------------------------------------------------------------------
// rle-compress / rle-decompress - real file I/O via rle::*File
// ---------------------------------------------------------------------

void rleCompress(const std::string& inputPath, const std::string& outputPath) {
    std::vector<uint8_t> data = readFile(inputPath);
    std::vector<uint8_t> out = rle::encodeFile(data);
    writeFile(outputPath, out);

    std::cout << "Original size:     " << data.size() << " bytes\n";
    std::cout << "Compressed size:   " << out.size() << " bytes\n";
    std::cout << "Compression ratio: " << std::fixed << std::setprecision(2)
               << compressionRatioPercent(data.size(), out.size()) << "%\n";
}

void rleDecompress(const std::string& inputPath, const std::string& outputPath) {
    std::vector<uint8_t> in = readFile(inputPath);
    bool checksumOk = false;
    std::vector<uint8_t> output = rle::decodeFile(in, &checksumOk);

    if (!checksumOk) {
        std::cerr << "WARNING: checksum mismatch - output may be corrupted.\n";
    } else {
        std::cout << "Checksum verified OK.\n";
    }

    writeFile(outputPath, output);
    std::cout << "Decompressed size: " << output.size() << " bytes -> " << outputPath << "\n";
}

// ---------------------------------------------------------------------
// tree - visualize the Huffman tree that would be built for a file
// ---------------------------------------------------------------------

void treeCommand(const std::string& inputPath, const std::string& dotPath) {
    std::vector<uint8_t> data = readFile(inputPath);
    if (data.empty()) {
        std::cout << "File is empty - no tree to build.\n";
        return;
    }

    auto freqTable = huffcodec::buildFrequencyTable(data);
    HuffmanTree tree;
    tree.build(freqTable);

    treeviz::printAscii(tree);

    std::vector<std::pair<uint8_t, std::string>> sortedCodes(tree.codes.begin(), tree.codes.end());
    std::sort(sortedCodes.begin(), sortedCodes.end());

    std::cout << "\nCode table (" << sortedCodes.size() << " unique byte" << (sortedCodes.size() == 1 ? "" : "s") << "):\n";
    for (const auto& entry : sortedCodes) {
        std::cout << "  " << std::left << std::setw(14) << treeviz::byteLabel(entry.first)
                   << " code=" << std::setw(12) << entry.second
                   << " freq=" << freqTable[entry.first] << "\n";
    }

    if (!dotPath.empty()) {
        treeviz::exportDot(tree, dotPath);
        std::cout << "\nGraphviz DOT file written to: " << dotPath << "\n";

        std::cout << "Render with Graphviz: dot -Tpng <input.dot> -o <output.png>\n";
    }
}

// ---------------------------------------------------------------------
// compare - benchmark Huffman vs RLE vs no compression on one file
// ---------------------------------------------------------------------

struct MethodStats {
    std::string name;
    size_t compressedSize;
    double ratioPercent;
    double compressMs;
    double decompressMs;
    bool verifiedRoundTrip;
};

namespace {

using clock_type = std::chrono::steady_clock;

MethodStats benchmarkHuffman(const std::vector<uint8_t>& data) {
    MethodStats stats;
    stats.name = "Huffman Coding";

    auto t0 = clock_type::now();
    std::vector<uint8_t> compressed = huffcodec::encode(data);
    auto t1 = clock_type::now();
    bool checksumOk = false;
    std::vector<uint8_t> restored = huffcodec::decode(compressed, &checksumOk);
    auto t2 = clock_type::now();

    stats.compressedSize = compressed.size();
    stats.ratioPercent = compressionRatioPercent(data.size(), compressed.size());
    stats.compressMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    stats.decompressMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    stats.verifiedRoundTrip = checksumOk && (restored == data);
    return stats;
}

MethodStats benchmarkRLE(const std::vector<uint8_t>& data) {
    MethodStats stats;
    stats.name = "Run-Length Encoding";

    auto t0 = clock_type::now();
    std::vector<uint8_t> compressed = rle::encodeFile(data);
    auto t1 = clock_type::now();
    bool checksumOk = false;
    std::vector<uint8_t> restored = rle::decodeFile(compressed, &checksumOk);
    auto t2 = clock_type::now();

    stats.compressedSize = compressed.size();
    stats.ratioPercent = compressionRatioPercent(data.size(), compressed.size());
    stats.compressMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    stats.decompressMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    stats.verifiedRoundTrip = checksumOk && (restored == data);
    return stats;
}

void printComparisonTable(std::ostream& os, size_t originalSize, const std::vector<MethodStats>& methods) {
    os << std::left << std::setw(22) << "Method"
       << std::right << std::setw(14) << "Size (bytes)"
       << std::setw(12) << "Ratio"
       << std::setw(14) << "Encode (ms)"
       << std::setw(14) << "Decode (ms)"
       << std::setw(12) << "Verified" << "\n";
    os << std::string(88, '-') << "\n";

    os << std::left << std::setw(22) << "No compression"
       << std::right << std::setw(14) << originalSize
       << std::setw(11) << "0.00%"
       << std::setw(14) << "-"
       << std::setw(14) << "-"
       << std::setw(12) << "-" << "\n";

    for (const auto& m : methods) {
        os << std::left << std::setw(22) << m.name
           << std::right << std::setw(14) << m.compressedSize
           << std::setw(10) << std::fixed << std::setprecision(2) << m.ratioPercent << "%"
           << std::setw(14) << std::setprecision(3) << m.compressMs
           << std::setw(14) << m.decompressMs
           << std::setw(12) << (m.verifiedRoundTrip ? "yes" : "NO - FAIL") << "\n";
    }
}

void writeReport(const std::string& reportPath, const std::string& inputName, size_t originalSize,
                  const std::vector<MethodStats>& methods, double entropyBitsPerByte, size_t uniqueBytes) {
    std::ofstream out(reportPath);
    if (!out) throw std::runtime_error("Cannot write report file: " + reportPath);

    out << "# Compression Method Comparison Report\n\n";
    out << "**Input file:** `" << inputName << "`  \n";
    out << "**Original size:** " << originalSize << " bytes  \n";
    out << "**Unique byte values:** " << uniqueBytes << " / 256  \n";
    out << "**Shannon entropy:** " << std::fixed << std::setprecision(3) << entropyBitsPerByte
        << " bits/byte (theoretical minimum average code length for any byte-wise entropy coder"
        << " on this data; a Huffman coder that reaches this bound would produce roughly "
        << static_cast<size_t>(std::ceil(entropyBitsPerByte * static_cast<double>(originalSize) / 8.0))
        << " bytes)\n\n";

    out << "## Results\n\n";
    out << "| Method | Compressed Size (bytes) | Compression Ratio | Encode Time (ms) | Decode Time (ms) | Round-trip Verified |\n";
    out << "|---|---:|---:|---:|---:|:---:|\n";
    out << "| No compression (baseline) | " << originalSize << " | 0.00% | - | - | - |\n";
    for (const auto& m : methods) {
        out << "| " << m.name << " | " << m.compressedSize << " | "
            << std::fixed << std::setprecision(2) << m.ratioPercent << "% | "
            << std::setprecision(3) << m.compressMs << " | " << m.decompressMs << " | "
            << (m.verifiedRoundTrip ? "Yes" : "**NO - MISMATCH**") << " |\n";
    }

    out << "\n## How to read this\n\n";
    out << "- **Compression ratio** = `100 * (1 - compressed_size / original_size)`. "
           "Higher is better; 0% means no size change; negative means the output grew.\n";
    out << "- **Huffman coding** exploits skew in overall *byte frequency* (some bytes appearing far "
           "more often than others). Its output size approaches the Shannon entropy bound listed above "
           "regardless of *where* those bytes occur in the file.\n";
    out << "- **Run-length encoding (RLE)** only exploits *consecutive* repeated bytes. It excels on long "
           "runs (e.g. sparse data, simple bitmaps) but can inflate data with few repeats, since every "
           "single non-repeating byte still costs 2 bytes (a count + the value) in this implementation.\n";
    out << "- Real-world tools such as gzip/zip combine LZ77 substring matching with Huffman coding, which "
           "is why they typically beat plain Huffman coding on natural text that has repeated words or "
           "phrases rather than just skewed byte frequency.\n";

    std::cout << "\nReport written to: " << reportPath << "\n";
}

} // namespace

void compareCommand(const std::string& inputPath, const std::string& reportPath) {
    std::vector<uint8_t> data = readFile(inputPath);
    if (data.empty()) {
        std::cout << "File is empty - nothing to compare.\n";
        return;
    }

    auto freqTable = huffcodec::buildFrequencyTable(data);
    double entropy = shannonEntropyBitsPerByte(freqTable, data.size());

    std::vector<MethodStats> methods = {
        benchmarkHuffman(data),
        benchmarkRLE(data),
    };

    std::cout << "Comparing compression methods on: " << inputPath
               << " (" << data.size() << " bytes, " << freqTable.size() << " unique byte values,"
               << " entropy " << std::fixed << std::setprecision(3) << entropy << " bits/byte)\n\n";

    printComparisonTable(std::cout, data.size(), methods);

    if (!reportPath.empty()) {
        writeReport(reportPath, inputPath, data.size(), methods, entropy, freqTable.size());
    }
}

// ---------------------------------------------------------------------
// CLI dispatch
// ---------------------------------------------------------------------

void printUsage(const char* prog) {
    std::cout << "Usage:\n"
              << "  " << prog << " compress       <input> <output>\n"
              << "  " << prog << " decompress     <input> <output>\n"
              << "  " << prog << " rle-compress   <input> <output>\n"
              << "  " << prog << " rle-decompress <input> <output>\n"
              << "  " << prog << " tree           <input> [output.dot]\n"
              << "  " << prog << " compare        <input> [report.md]\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];
    std::string inputPath = argv[2];

    try {
        if (mode == "compress" || mode == "decompress" || mode == "rle-compress" || mode == "rle-decompress") {
            if (argc != 4) { printUsage(argv[0]); return 1; }
            std::string outputPath = argv[3];
            if (mode == "compress") compress(inputPath, outputPath);
            else if (mode == "decompress") decompress(inputPath, outputPath);
            else if (mode == "rle-compress") rleCompress(inputPath, outputPath);
            else rleDecompress(inputPath, outputPath);
        } else if (mode == "tree") {
            if (argc < 3 || argc > 4) { printUsage(argv[0]); return 1; }
            std::string dotPath = (argc == 4) ? argv[3] : "";
            treeCommand(inputPath, dotPath);
        } else if (mode == "compare") {
            if (argc < 3 || argc > 4) { printUsage(argv[0]); return 1; }
            std::string reportPath = (argc == 4) ? argv[3] : "";
            compareCommand(inputPath, reportPath);
        } else {
            std::cerr << "Unknown mode '" << mode << "'.\n\n";
            printUsage(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
