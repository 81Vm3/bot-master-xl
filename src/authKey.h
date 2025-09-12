//
// Created by rain on 7/13/25.
//

#ifndef AUTHKEY_H
#define AUTHKEY_H
#include <cstdint>

int gen_gpci(char buf[64], uint32_t factor);
extern char AuthKeyTable[512][2][128];

#endif //AUTHKEY_H
