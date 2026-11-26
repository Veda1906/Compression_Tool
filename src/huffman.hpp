#pragma once
#include <cstdint>
#include <unordered_map>
#include "min_heap.hpp"
#include <string>
#include <vector>
#include <algorithm>

struct Node {
    uint8_t byteValue;
    bool isLeaf;
    uint64_t freq;
    uint8_t minByte;
    Node* left;
    Node* right;

    // Leaf node constructor
    Node(uint8_t b, uint64_t f)
        : byteValue(b), isLeaf(true), freq(f), minByte(b), left(nullptr), right(nullptr) {}

    // Internal node constructor (merges two subtrees)
    Node(Node* l, Node* r)
        : byteValue(0), isLeaf(false), freq(l->freq + r->freq), minByte(std::min(l->minByte,r->minByte)), left(l), right(r) {}
};

// Min-heap comparator: smallest frequency has highest priority.
struct Compare {
    bool operator()(Node* a, Node* b) const { return a->freq != b->freq ? a->freq < b->freq : a->minByte < b->minByte; }
};

class HuffmanTree {
public:
    Node* root = nullptr;
    std::unordered_map<uint8_t, std::string> codes;

    // Builds the tree greedily from a byte -> frequency table.
    //
    // Break frequency ties by the smallest byte in each subtree.
    void build(const std::unordered_map<uint8_t, uint64_t>& freqTable) {
        destroy(root); root = nullptr;
        MinHeap<Node*, Compare> heap;
        const auto& entries = freqTable;
        for (const auto& entry : entries) {
            heap.push(new Node(entry.first, entry.second));
        }

        while (heap.size() > 1) {
            Node* left = heap.extractMin();
            Node* right = heap.extractMin();
            heap.push(new Node(left, right));
        }
        if (!heap.empty()) root = heap.extractMin();

        codes.clear();
        std::string path;
        generateCodes(root, path);
    }

    ~HuffmanTree() { destroy(root); }

    // Non-copyable: this class owns raw tree pointers.
    HuffmanTree() = default;
    HuffmanTree(const HuffmanTree&) = delete;
    HuffmanTree& operator=(const HuffmanTree&) = delete;

private:
    void generateCodes(Node* node, std::string& path) {
        if (!node) return;
        if (node->isLeaf) {
            codes[node->byteValue] = path.empty() ? "0" : path;
            return;
        }
        path.push_back('0');
        generateCodes(node->left, path);
        path.pop_back();

        path.push_back('1');
        generateCodes(node->right, path);
        path.pop_back();
    }

    void destroy(Node* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
};
