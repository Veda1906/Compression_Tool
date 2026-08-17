#pragma once
#include <cstdint>
#include <vector>

// Standard CRC-32 (IEEE 802.3 polynomial), table-based for speed.
inline uint32_t crc32(const std::vector<uint8_t>& data) {
    static uint32_t table[256];
    static bool tableReady = false;
    if (!tableReady) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int j = 0; j < 8; j++) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        tableReady = true;
    }

    uint32_t crc = 0xFFFFFFFFu;
    for (uint8_t byte : data) {
        crc = table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}
