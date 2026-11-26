#pragma once
#include <vector>
#include <array>
#include <limits>
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
    std::array<uint64_t,256> counts{};
    for (uint8_t b : data) ++counts[b];
    for (size_t i=0;i<counts.size();++i) if (counts[i]) freq[static_cast<uint8_t>(i)]=counts[i];
    return freq;
}


inline vector<uint8_t> encode(const std::vector<uint8_t>& data, HuffmanTree* treeOut = nullptr) {
    if (data.size() > UINT32_MAX) throw runtime_error("Input exceeds format limit");
    vector<uint8_t> out = {'H', 'U', 'F', '2'};

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
    std::array<std::string,256> lookup;
    for (const auto& e : tree.codes) lookup[e.first]=e.second;
    for (uint8_t b : data) writer.writeBits(lookup[b]);
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
    if (in.size()<17 || in[0]!='H' || in[1]!='U' || in[2]!='F' || in[3]!='2')
        throw runtime_error("Invalid HUF2 header (legacy HUFF requires original decoder)");
    size_t pos=4;
    uint32_t originalSize=detail::readUint32(in,pos);
    uint32_t freqCount=detail::readUint32(in,pos);
    if (freqCount>256 || in.size()<17+5*freqCount) throw runtime_error("Invalid frequency table");
    unordered_map<uint8_t,uint64_t> freqTable;
    uint64_t sum=0;
    for(uint32_t i=0;i<freqCount;++i) {
        uint8_t b=in[pos++]; uint32_t f=detail::readUint32(in,pos);
        if (!f || !freqTable.emplace(b,f).second) throw runtime_error("Invalid frequency entry");
        sum+=f;
    }
    if(sum!=originalSize) throw runtime_error("Frequency sum mismatch");
    uint8_t padding=in[pos++];
    uint32_t expectedChecksum=detail::readUint32(in,pos);
    vector<uint8_t> body(in.begin()+pos,in.end());
    if(padding>7 || (body.empty() && padding) || (padding && (body.back() & ((1u<<padding)-1)))) throw runtime_error("Invalid padding");
    HuffmanTree tree; tree.build(freqTable);
    uint64_t expectedBits=0;
    for(const auto& e:freqTable) expectedBits+=e.second*tree.codes.at(e.first).size();
    if(expectedBits!=uint64_t(body.size())*8-padding) throw runtime_error("Invalid payload length");
    BitReader reader(body,padding);
    vector<uint8_t> output; output.reserve(originalSize);
    Node* node=tree.root;
    while(reader.hasMore()) {
        int bit=reader.readBit();
        if(tree.root->isLeaf) { if(bit!=0) throw runtime_error("Invalid single-symbol bit"); }
        else node=bit?node->right:node->left;
        if(node->isLeaf) { output.push_back(node->byteValue); node=tree.root; }
        if(output.size()>originalSize) throw runtime_error("Excess decoded data");
    }
    if(output.size()!=originalSize || node!=tree.root || crc32(output)!=expectedChecksum) throw runtime_error("Corrupt Huffman data");
    if(checksumOk) *checksumOk=true;
    return output;
}
} // namespace huffcodec
