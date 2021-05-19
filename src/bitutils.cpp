/**************************************************/
/*  Invictus 2021						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "typedefs.h"
#include "constants.h"

//#define NOPOPCNT

#ifdef _MSC_VER

#include <intrin.h>

#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)

namespace BitUtils {
    int getFirstBit(uint64_t bb) {
        unsigned long index = 0;
        _BitScanForward64(&index, bb);
        return index;
    }

    int popFirstBit(uint64_t& b) {
        unsigned long index = 0;
        _BitScanForward64(&index, b);
        b &= (b - 1);
        return index;
    }

#ifdef NOPOPCNT
    int bitCnt(uint64_t x) {
        x -= (x >> 1) & 0x5555555555555555ULL;
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
        return (x * 0x0101010101010101ULL) >> 56;
    }
#else
    int bitCnt(uint64_t x) {
        return __popcnt64(x);
    }
}
#endif
#else
namespace BitUtils {
    int getFirstBit(uint64_t bb) {
        return __builtin_ctzll(bb);
    }

    int popFirstBit(uint64_t& b) {
        int index = __builtin_ctzll(b);
        b &= (b - 1);
        return index;
    }

    int bitCnt(uint64_t x) {
        return __builtin_popcountll(x);
    }
}
#endif