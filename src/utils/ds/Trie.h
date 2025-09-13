//
// Created by rain on 13/9/2025.
//

#ifndef BOTMASTERXL_TRIE_H
#define BOTMASTERXL_TRIE_H

#include <string>
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

using DataNode = std::unique_ptr<std::vector<std::byte>>;

class Trie {
private:
    static constexpr int MAX_NODES = 100000;
    static constexpr int ALPHABET_SIZE = 26;
    
    std::array<std::array<int, ALPHABET_SIZE>, MAX_NODES> nex{};
    std::array<bool, MAX_NODES> exist{};
    std::unordered_map<int, DataNode> data;
    int cnt = 0;

public:
    Trie() = default;
    
    void insert(const char* s, int l, DataNode d);
    void insert(const std::string& s, DataNode d);
    
    const std::vector<std::byte>* find(const char* s, int l);
    const std::vector<std::byte>* find(const std::string& s);
};

#endif //BOTMASTERXL_TRIE_H