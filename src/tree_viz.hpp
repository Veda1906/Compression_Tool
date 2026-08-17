#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>
#include <stdexcept>
#include "huffman.hpp"

// Renders a HuffmanTree two ways:
//   1. printAscii()  -> a readable tree diagram straight to the terminal
//   2. exportDot()   -> a Graphviz .dot file you can render into an image,
//                       e.g.  dot -Tpng tree.dot -o tree.png
namespace treeviz {

// Human-readable label for a byte: the printable character itself where
// possible, otherwise a hex escape (control chars, high bytes, etc).
inline std::string byteLabel(uint8_t b) {
    std::ostringstream oss;
    if (b == ' ') {
        oss << "' '(space)";
    } else if (b == '\n') {
        oss << "\\n";
    } else if (b == '\t') {
        oss << "\\t";
    } else if (std::isprint(b)) {
        oss << "'" << static_cast<char>(b) << "'";
    } else {
        oss << "0x" << std::hex << std::uppercase << static_cast<int>(b);
    }
    return oss.str();
}

namespace detail {

inline void printNode(std::ostream& os, Node* node, const std::string& prefix, bool isLast,
                       const std::unordered_map<uint8_t, std::string>& codes) {
    if (!node) return;
    os << prefix << (isLast ? "\\-- " : "|-- ");
    if (node->isLeaf) {
        os << byteLabel(node->byteValue) << "  freq=" << node->freq;
        auto it = codes.find(node->byteValue);
        if (it != codes.end()) os << "  code=" << it->second;
        os << "\n";
    } else {
        os << "* freq=" << node->freq << "\n";
        std::string childPrefix = prefix + (isLast ? "    " : "|   ");
        printNode(os, node->left, childPrefix, false, codes);
        printNode(os, node->right, childPrefix, true, codes);
    }
}

inline std::string escapeDot(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

inline void writeDotNode(std::ostream& os, Node* node, int& idCounter, int myId) {
    if (!node) return;
    if (node->isLeaf) {
        os << "  n" << myId << " [label=\"" << escapeDot(byteLabel(node->byteValue))
           << "\\nfreq=" << node->freq << "\", shape=box, style=filled, fillcolor=\"#FFF3B0\"];\n";
    } else {
        os << "  n" << myId << " [label=\"" << node->freq
           << "\", shape=circle, style=filled, fillcolor=\"#B0D9FF\"];\n";
        if (node->left) {
            int leftId = ++idCounter;
            os << "  n" << myId << " -> n" << leftId << " [label=\"0\"];\n";
            writeDotNode(os, node->left, idCounter, leftId);
        }
        if (node->right) {
            int rightId = ++idCounter;
            os << "  n" << myId << " -> n" << rightId << " [label=\"1\"];\n";
            writeDotNode(os, node->right, idCounter, rightId);
        }
    }
}

} // namespace detail

// Prints an ASCII tree diagram. '*' marks internal (merged) nodes; leaves
// show the original byte, its frequency, and the Huffman code assigned.
inline void printAscii(const HuffmanTree& tree, std::ostream& os = std::cout) {
    if (!tree.root) {
        os << "(empty tree - file had no data)\n";
        return;
    }
    os << "Huffman Tree  (* = internal/merged node; leaves show byte, frequency, code)\n\n";
    if (tree.root->isLeaf) {
        os << byteLabel(tree.root->byteValue) << "  freq=" << tree.root->freq << "  code=0\n";
        return;
    }
    os << "* freq=" << tree.root->freq << "\n";
    detail::printNode(os, tree.root->left, "", false, tree.codes);
    detail::printNode(os, tree.root->right, "", true, tree.codes);
}

// Writes a Graphviz DOT file describing the tree.
inline void exportDot(const HuffmanTree& tree, const std::string& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot write DOT file: " + path);
    out << "digraph HuffmanTree {\n";
    out << "  node [fontname=\"Helvetica\"];\n";
    out << "  edge [fontname=\"Helvetica\"];\n";
    if (tree.root) {
        int idCounter = 0;
        detail::writeDotNode(out, tree.root, idCounter, 0);
    }
    out << "}\n";
}

} // namespace treeviz
