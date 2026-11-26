CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
TARGET   := compressor
SRC      := src/main.cpp

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC) src/min_heap.hpp src/bitio.hpp src/huffman.hpp src/crc32.hpp src/huffman_codec.hpp src/rle.hpp src/tree_viz.hpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET) unit_tests
	./unit_tests
	bash tests/run_tests.sh

clean:
	rm -f $(TARGET) unit_tests
	rm -f samples/*.huff samples/*.huff.out samples/*.rle samples/*.rle.out
	rm -f samples/*.dot samples/*.png samples/*.md

unit_tests: tests/unit_tests.cpp $(wildcard src/*.hpp)
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $<
