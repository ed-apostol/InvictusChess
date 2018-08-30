/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

namespace Attacks {
	extern void initArr(void);
	extern inline uint64_t knightAttacksBB(int from, uint64_t occ);
	extern inline uint64_t pawnMovesBB(int from, uint64_t s);
	extern inline uint64_t pawnMoves2BB(int from, uint64_t s);
	extern inline uint64_t pawnAttacksBB(int from, uint64_t s);
	extern inline uint64_t bishopAttacksBB(int from, uint64_t occ);
	extern inline uint64_t bishopAttacksBBX(int from, uint64_t occ);
	extern inline uint64_t rookAttacksBB(int from, uint64_t occ);
	extern inline uint64_t rookAttacksBBX(int from, uint64_t occ);
	extern inline uint64_t queenAttacksBB(int from, uint64_t occ);
	extern inline uint64_t kingAttacksBB(int from, uint64_t occ);

	extern uint64_t knightMovesBB(int from);
	extern inline uint64_t kingMovesBB(int from);
}