/**************************************************/
/*  Invictus 2019                                 */
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

    extern inline uint64_t knightMovesBB(int from);
    extern inline uint64_t kingMovesBB(int from);
    extern std::function<uint64_t(uint64_t)> shift8BB[2];
    extern std::function<uint64_t(uint64_t)> shift16BB[2];
    extern std::function<uint64_t(uint64_t)> fillBB[2];
    extern std::function<uint64_t(uint64_t)> fillBBEx[2];
    extern inline uint64_t pawnAttackBB(uint64_t pawns, size_t color);
}