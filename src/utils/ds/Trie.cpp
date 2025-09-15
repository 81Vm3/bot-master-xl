//
// Created by rain on 13/9/2025.
//

#include "Trie.h"

void Trie::insert(const char* s, int l, DataNode d) {
    int p = 0;
    for (int i = 0; i < l; i++) {
        int c = s[i] - 'a';
        if (!nex[p][c]) {
            nex[p][c] = ++cnt;
        }
        p = nex[p][c];
    }
    exist[p] = true;
    if (d) {
        data[p] = std::move(d);
    }
}

void Trie::insert(const std::string& s, DataNode d) {
    insert(s.c_str(), s.size(), std::move(d));
}

const std::vector<std::byte>* Trie::find(const char* s, int l) {
    int p = 0;
    for (int i = 0; i < l; i++) {
        int c = s[i] - 'a';
        if (!nex[p][c]) return nullptr;
        p = nex[p][c];
    }
    if (exist[p] && data.count(p)) {
        return data[p].get();
    }
    return nullptr;
}

const std::vector<std::byte>* Trie::find(const std::string& s) {
    return find(s.c_str(), static_cast<int>(s.length()));
}
