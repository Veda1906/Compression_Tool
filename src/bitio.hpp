#pragma once
#include <vector>
#include <cstdint>
#include <string>

// Writes individual bits, packing them into bytes (MSB-first).
class BitWriter {
public:
    void writeBit(int bit) {
        currentByte = static_cast<uint8_t>((currentByte << 1) | (bit & 1));
        bitCount++;
        if (bitCount == 8) {
            buffer.push_back(currentByte);
            currentByte = 0;
            bitCount = 0;
        }
    }

    void writeBits(const std::string& bits) {
        for (char c : bits) writeBit(c - '0');
    }

    // Pads the final partial byte with zeros and returns the number of
    // padding bits added (needed by the reader to know where data ends).
    int flush() {
        int padding = 0;
        if (bitCount > 0) {
            padding = 8 - bitCount;
            currentByte = static_cast<uint8_t>(currentByte << padding);
            buffer.push_back(currentByte);
            currentByte = 0;
            bitCount = 0;
        }
        return padding;
    }

    const std::vector<uint8_t>& getBuffer() const { return buffer; }

private:
    std::vector<uint8_t> buffer;
    uint8_t currentByte = 0;
    int bitCount = 0;
};

// Reads individual bits back out of a byte buffer (MSB-first),
// stopping before the padding bits added at compression time.
class BitReader {
public:
    BitReader(const std::vector<uint8_t>& data, int paddingBits)
        : data(data) {
        totalBits = data.size() * 8 - static_cast<size_t>(paddingBits);
    }

    // Returns 0 or 1, or -1 if no more real bits remain.
    int readBit() {
        if (bitPos >= totalBits) return -1;
        size_t byteIndex = bitPos / 8;
        int bitIndex = 7 - static_cast<int>(bitPos % 8);
        int bit = (data[byteIndex] >> bitIndex) & 1;
        bitPos++;
        return bit;
    }

    bool hasMore() const { return bitPos < totalBits; }

private:
    const std::vector<uint8_t>& data;
    size_t totalBits;
    size_t bitPos = 0;
};
