#pragma once
#include <cstdint>
#include <unordered_map>
#include <queue>
#include <string>
#include <vector>
#include <algorithm>

struct Node {
    uint8_t byteValue;
    bool isLeaf;
    uint64_t freq;
    Node* left;
    Node* right;

    // Leaf node constructor
    Node(uint8_t b, uint64_t f)
        : byteValue(b), isLeaf(true), freq(f), left(nullptr), right(nullptr) {}

    // Internal node constructor (merges two subtrees)
    Node(Node* l, Node* r)
        : byteValue(0), isLeaf(false), freq(l->freq + r->freq), left(l), right(r) {}
};

// Min-heap comparator: smallest frequency has highest priority.
struct Compare {
    bool operator()(Node* a, Node* b) const { return a->freq > b->freq; }
};

class HuffmanTree {
public:
    Node* root = nullptr;
    std::unordered_map<uint8_t, std::string> codes;

    // Builds the tree greedily from a byte -> frequency table.
    //
    // IMPORTANT: unordered_map iteration order is not guaranteed to match
    // between the encoder's map and the decoder's separately-rebuilt map
    // (it depends on insertion order, not just key contents). If two bytes
    // tie on frequency, inserting them into the heap in a different order
    // would break the tie differently and silently produce a DIFFERENT
    // tree on decode -> corrupted output. Sorting by byte value first makes
    // heap insertion order deterministic, so ties always resolve the same
    // way on both ends.
    void build(const std::unordered_map<uint8_t, uint64_t>& freqTable) {
        std::vector<std::pair<uint8_t, uint64_t>> entries(freqTable.begin(), freqTable.end());
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        std::priority_queue<Node*, std::vector<Node*>, Compare> heap;
        for (const auto& entry : entries) {
            heap.push(new Node(entry.first, entry.second));
        }

        // Edge case: only one unique byte in the whole file.
        // We still need a valid 1-bit code, so pair it with a dummy leaf.
        if (heap.size() == 1) {
            Node* only = heap.top();
            heap.pop();
            root = new Node(only, new Node(static_cast<uint8_t>(0), 0));
        }

        while (heap.size() > 1) {
            Node* left = heap.top(); heap.pop();
            Node* right = heap.top(); heap.pop();
            heap.push(new Node(left, right));
        }

        if (!root && !heap.empty()) {
            root = heap.top();
        }

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
