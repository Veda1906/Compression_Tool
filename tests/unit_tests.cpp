#include <cassert>
#include <functional>
#include "min_heap.hpp"
#include "huffman_codec.hpp"
#include "rle.hpp"
int main(){
 MinHeap<int,std::less<int>> h; for(int x:{5,1,9,1,3}) h.push(x); for(int x:{1,1,3,5,9}) assert(h.extractMin()==x); assert(h.empty());
 BitWriter w; w.writeBits("101100101"); int p=w.flush(); assert(p==7 && w.getBuffer()[0]==178 && w.getBuffer()[1]==128); BitReader r(w.getBuffer(),p); for(char c:std::string("101100101")) assert(r.readBit()==c-'0'); assert(r.readBit()==-1);
 HuffmanTree t; t.build({{0,3},{1,2},{2,1}}); assert(t.root->freq==6); t.build({{0,5}}); assert(t.codes.size()==1 && t.codes.at(0)=="0");
 for(auto d: {std::vector<uint8_t>{},std::vector<uint8_t>(500,0),std::vector<uint8_t>{0,255,1,2,3}}){assert(huffcodec::decode(huffcodec::encode(d))==d);assert(rle::decodeFile(rle::encodeFile(d))==d);}
 assert(rle::encodeRaw(std::vector<uint8_t>(256,7))==std::vector<uint8_t>({255,7,1,7}));
 assert(crc32(std::vector<uint8_t>{'1','2','3','4','5','6','7','8','9'})==0xcbf43926u);
 bool rejected=false;try{huffcodec::decode({'H','U','F','2'});}catch(const std::runtime_error&){rejected=true;}assert(rejected);
}
