// Host-only platform shim; never part of the Xbox build.
#pragma once
#include <windows.h>
#include <cstring>
inline int XNetRandom(unsigned char *bytes, unsigned long length) {
    static unsigned char seed = 1;
    std::memset(bytes, seed++, length);
    return 0;
}
