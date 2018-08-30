/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

namespace BitUtils {
	extern inline int getFirstBit(uint64_t b);
	extern inline int popFirstBit(uint64_t& b);
	extern inline int bitCnt(uint64_t b);
	extern inline uint64_t pawnAttackBB(const uint64_t pawns, const int color);
}
