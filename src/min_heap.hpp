#pragma once
#include <vector>
#include <stdexcept>
#include <utility>

template<class T, class Less>
class MinHeap {
    std::vector<T> items;
    Less less;
public:
    size_t size() const { return items.size(); }
    bool empty() const { return items.empty(); }
    void push(T value) {
        items.push_back(value);
        size_t i = items.size()-1;
        while (i) {
            size_t p = (i-1)/2;
            if (!less(items[i], items[p])) break;
            std::swap(items[i], items[p]); i=p;
        }
    }
    T extractMin() {
        if (empty()) throw std::underflow_error("empty heap");
        T result=items.front();
        items.front()=items.back(); items.pop_back();
        size_t i=0;
        while (2*i+1 < items.size()) {
            size_t c=2*i+1;
            if (c+1<items.size() && less(items[c+1],items[c])) ++c;
            if (!less(items[c],items[i])) break;
            std::swap(items[c],items[i]); i=c;
        }
        return result;
    }
};
