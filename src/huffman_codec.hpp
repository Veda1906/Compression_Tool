#pragma once
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <stdexcept>
#include "bitio.hpp"
#include "huffman.hpp"
#include "crc32.hpp"
using namespace std;

namespace huffcodec {

namespace detail {
inline void writeUint32(vector<uint8_t>& buf, uint32_t val) {
    for (int i = 3; i >= 0; i--) buf.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
}
inline uint32_t readUint32(const vector<uint8_t>& buf, size_t& pos) {
    uint32_t val = 0;
    for (int i = 0; i < 4; i++) val = (val << 8) | buf[pos++];
    return val;
}
} 

inline unordered_map<uint8_t, uint64_t> buildFrequencyTable(const vector<uint8_t>& data) {
    unordered_map<uint8_t, uint64_t> freq;
    for (uint8_t b : data) freq[b]++;
    return freq;
}


inline vector<uint8_t> encode(const std::vector<uint8_t>& data, HuffmanTree* treeOut = nullptr) {
    vector<uint8_t> out = {'H', 'U', 'F', 'F'};

    if (data.empty()) {
        detail::writeUint32(out, 0);
        detail::writeUint32(out, 0);
        out.push_back(0);
        detail::writeUint32(out, 0);
        return out;
    }

    auto freqTable = buildFrequencyTable(data);

    HuffmanTree localTree;
    HuffmanTree& tree = treeOut ? *treeOut : localTree;
    tree.build(freqTable);

    BitWriter writer;
    for (uint8_t b : data) writer.writeBits(tree.codes[b]);
    int padding = writer.flush();
    const std::vector<uint8_t>& body = writer.getBuffer();

    detail::writeUint32(out, static_cast<uint32_t>(data.size()));
    detail::writeUint32(out, static_cast<uint32_t>(freqTable.size()));
    for (const auto& entry : freqTable) {
        out.push_back(entry.first);
        detail::writeUint32(out, static_cast<uint32_t>(entry.second));
    }
    out.push_back(static_cast<uint8_t>(padding));
    detail::writeUint32(out, crc32(data));
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

inline vector<uint8_t> decode(const std::vector<uint8_t>& in, bool* checksumOk = nullptr) {
    size_t pos = 0;
    if (in.size() < 4 || in[0] != 'H' || in[1] != 'U' || in[2] != 'F' || in[3] != 'F') {
        throw runtime_error("Not a valid .huff buffer (bad magic bytes).");
    }
    pos = 4;

    uint32_t originalSize = detail::readUint32(in, pos);
    uint32_t freqCount = detail::readUint32(in, pos);

    if (originalSize == 0) {
        if (checksumOk) *checksumOk = true;
        return {};
    }

    std::unordered_map<uint8_t, uint64_t> freqTable;
    for (uint32_t i = 0; i < freqCount; i++) {
        uint8_t byte = in[pos++];
        uint32_t freq = detail::readUint32(in, pos);
        freqTable[byte] = freq;
    }

    uint8_t padding = in[pos++];
    uint32_t expectedChecksum = detail::readUint32(in, pos);
    std::vector<uint8_t> body(in.begin() + static_cast<long>(pos), in.end());

    HuffmanTree tree;
    tree.build(freqTable);

    BitReader reader(body, padding);
    std::vector<uint8_t> output;
    output.reserve(originalSize);

    Node* node = tree.root;
    while (output.size() < originalSize) {
        int bit = reader.readBit();
        if (bit == -1) break; 
        node = (bit == 0) ? node->left : node->right;
        if (node->isLeaf) {
            output.push_back(node->byteValue);
            node = tree.root;
        }
    }

    if (checksumOk) *checksumOk = (crc32(output) == expectedChecksum);
    return output;
}

} // namespace huffcodec
