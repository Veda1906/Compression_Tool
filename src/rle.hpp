#pragma once
#include <vector>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include "crc32.hpp"

// Simple byte-wise Run-Length Encoding (RLE), included as a second, much
// simpler compression method to compare Huffman coding against. It only
// exploits CONSECUTIVE repeated bytes (e.g. "aaaa" -> count=4,value='a'),
// unlike Huffman which exploits overall byte-frequency skew.
namespace rle {

namespace detail {
inline void writeUint32(std::vector<uint8_t>& buf, uint32_t val) {
    for (int i = 3; i >= 0; i--) buf.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
}
inline uint32_t readUint32(const std::vector<uint8_t>& buf, size_t& pos) {
    uint32_t val = 0;
    for (int i = 0; i < 4; i++) val = (val << 8) | buf[pos++];
    return val;
}
} // namespace detail

// Raw RLE encode: emits (count, value) byte pairs. Runs longer than 255
// are split across multiple pairs.
inline std::vector<uint8_t> encodeRaw(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    size_t i = 0;
    while (i < data.size()) {
        uint8_t value = data[i];
        size_t runLength = 1;
        while (i + runLength < data.size() && data[i + runLength] == value && runLength < 255) {
            runLength++;
        }
        out.push_back(static_cast<uint8_t>(runLength));
        out.push_back(value);
        i += runLength;
    }
    return out;
}

inline std::vector<uint8_t> decodeRaw(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    for (size_t i = 0; i + 1 < data.size(); i += 2) {
        uint8_t count = data[i];
        uint8_t value = data[i + 1];
        out.insert(out.end(), count, value);
    }
    return out;
}

// Self-describing file format, mirroring the Huffman one:
//   [4 bytes] magic "RLE1"
//   [4 bytes] original size (u32)
//   [4 bytes] CRC32 of original data
//   [...]     RLE-encoded body
inline std::vector<uint8_t> encodeFile(const std::vector<uint8_t>& data) {
    if (data.size()>UINT32_MAX) throw std::runtime_error("Input exceeds format limit");
    std::vector<uint8_t> out = {'R', 'L', 'E', '1'};
    detail::writeUint32(out, static_cast<uint32_t>(data.size()));
    detail::writeUint32(out, crc32(data));
    std::vector<uint8_t> body = encodeRaw(data);
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

inline std::vector<uint8_t> decodeFile(const std::vector<uint8_t>& in, bool* checksumOk = nullptr) {
    if (in.size() < 12 || in[0] != 'R' || in[1] != 'L' || in[2] != 'E' || in[3] != '1') {
        throw std::runtime_error("Not a valid .rle buffer (bad magic bytes).");
    }
    size_t pos = 4;
    uint32_t originalSize = detail::readUint32(in, pos);
    uint32_t expectedChecksum = detail::readUint32(in, pos);
    std::vector<uint8_t> body(in.begin() + static_cast<long>(pos), in.end());
    if(body.size()%2) throw std::runtime_error("Incomplete RLE pair");
    uint64_t sum=0;
    for(size_t i=0;i<body.size();i+=2) { if(!body[i]) throw std::runtime_error("Zero RLE count"); sum+=body[i]; }
    if(sum!=originalSize) throw std::runtime_error("RLE size mismatch");
    std::vector<uint8_t> output = decodeRaw(body);

    bool ok = (output.size() == originalSize) && (crc32(output) == expectedChecksum);
    if (!ok) throw std::runtime_error("RLE checksum mismatch");
    if (checksumOk) *checksumOk = ok;
    return output;
}

} // namespace rle
