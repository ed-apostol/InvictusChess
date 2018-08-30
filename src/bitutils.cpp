/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <intrin.h>
#include "typedefs.h"
#include "constants.h"

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

	uint64_t shiftLeft(uint64_t b, int i) {
		return (b << i);
	}
	uint64_t shiftRight(uint64_t b, int i) {
		return (b >> i);
	}
	uint64_t(*ShiftPtr[])(uint64_t, int) = { &shiftLeft, &shiftRight };

	uint64_t pawnAttackBB(const uint64_t pawns, const int color) {
		static const int Shift[] = { 9, 7 };
		const uint64_t pawnAttackLeft = (*ShiftPtr[color])(pawns, Shift[color ^ 1]) & ~FileHBB;
		const uint64_t pawnAttackright = (*ShiftPtr[color])(pawns, Shift[color]) & ~FileABB;
		return (pawnAttackLeft | pawnAttackright);
	}
}