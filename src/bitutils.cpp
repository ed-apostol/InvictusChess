/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <intrin.h>
#include "typedefs.h"

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

	int bitCnt(uint64_t x) {
		return __popcnt64(x);
	}
}