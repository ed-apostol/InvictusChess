/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "typedefs.h"
#include "constants.h"
#include "attacks.h"
#include "bitutils.h"
#include "position.h"

using namespace Attacks;
using namespace BitUtils;
using namespace PositionData;

#define genMoves(FNC, PCB, SP, TBB, F) \
    for (uint64_t bits = PCB; bits;) { \
        int from = popFirstBit(bits); \
        for (uint64_t mvbits = FNC(from, SP) & TBB; mvbits;) \
            mvlist.add(move_t(from, popFirstBit(mvbits), F)); }

#define genMovesN(FNC, PCB, SP, TBB) genMoves(FNC, PCB, SP, TBB, MF_NORMAL)

#define genMovesProm(FNC, PCB, SP, TBB) \
    for (uint64_t bits = PCB; bits;) { \
        int from = popFirstBit(bits); \
        for (uint64_t mvbits = FNC(from, SP) & TBB; mvbits;) { \
            int to = popFirstBit(mvbits); \
            mvlist.add(move_t(from, to, MF_PROMQ)); \
            mvlist.add(move_t(from, to, MF_PROMR)); \
            mvlist.add(move_t(from, to, MF_PROMB)); \
            mvlist.add(move_t(from, to, MF_PROMN)); }}

void position_t::genLegal(movelist_t<256>& mvlist) {
    if (kingIsInCheck())
        genCheckEvasions(mvlist);
    else {
        movelist_t<256> mlt;
        uint64_t pinned = pinnedPieces(side);
        genTacticalMoves(mlt);
        genQuietMoves(mlt);
        for (int x = 0; x < mlt.size; ++x) {
            if (!moveIsLegal(mlt.mv(x), pinned, false)) continue;
            mvlist.add(mlt.mv(x));
        }
    }
}

void position_t::genQuietMoves(movelist_t<256>& mvlist) {
    uint64_t pcbits;

    if (stack.castle & (side ? BCKS : WCKS) && !(occupiedBB & CastleSquareMask1[side][0]) && !areaIsAttacked(side ^ 1, CastleSquareMask2[side][0]))
        mvlist.add(move_t(CastleSquareFrom[side], CastleSquareTo[side][0], MF_CASTLE));
    if (stack.castle & (side ? BCQS : WCQS) && !(occupiedBB & CastleSquareMask1[side][1]) && !areaIsAttacked(side ^ 1, CastleSquareMask2[side][1]))
        mvlist.add(move_t(CastleSquareFrom[side], CastleSquareTo[side][1], MF_CASTLE));

    pcbits = (getPieceBB(PAWN, side) & ~Rank7ByColorBB[side]) & ShiftPtr[side ^ 1](~occupiedBB, 8);
    genMovesN(pawnMovesBB, pcbits, side, ~occupiedBB);

    pcbits = (getPieceBB(PAWN, side) & Rank2ByColorBB[side]) & ShiftPtr[side ^ 1](~occupiedBB, 8) & ShiftPtr[side ^ 1](~occupiedBB, 16);
    genMoves(pawnMoves2BB, pcbits, side, ~occupiedBB, MF_PAWN2);

    genMovesN(knightAttacksBB, getPieceBB(KNIGHT, side), 0, ~occupiedBB);
    genMovesN(bishopAttacksBB, getPieceBB(BISHOP, side), occupiedBB, ~occupiedBB);
    genMovesN(rookAttacksBB, getPieceBB(ROOK, side), occupiedBB, ~occupiedBB);
    genMovesN(queenAttacksBB, getPieceBB(QUEEN, side), occupiedBB, ~occupiedBB);
    genMovesN(kingAttacksBB, getPieceBB(KING, side), 0, ~occupiedBB & ~kingMovesBB(kpos[side ^ 1]));
}

void position_t::genTacticalMoves(movelist_t<256>& mvlist) {
    const uint64_t targetBB = colorBB[side ^ 1] & ~piecesBB[KING];

    if (stack.epsq != -1)
        genMoves(pawnAttacksBB, getPieceBB(PAWN, side) & pawnAttacksBB(stack.epsq, side ^ 1), side, BitMask[stack.epsq], MF_ENPASSANT);

    uint64_t pcbits = getPieceBB(PAWN, side) & Rank7ByColorBB[side];
    genMovesProm(pawnMovesBB, pcbits, side, ~occupiedBB);
    genMovesProm(pawnAttacksBB, pcbits, side, targetBB);

    genMovesN(pawnAttacksBB, getPieceBB(PAWN, side) & ~Rank7ByColorBB[side], side, targetBB);
    genMovesN(knightAttacksBB, getPieceBB(KNIGHT, side), 0, targetBB);
    genMovesN(bishopAttacksBB, getPieceBB(BISHOP, side), occupiedBB, targetBB);
    genMovesN(rookAttacksBB, getPieceBB(ROOK, side), occupiedBB, targetBB);
    genMovesN(queenAttacksBB, getPieceBB(QUEEN, side), occupiedBB, targetBB);
    genMovesN(kingAttacksBB, getPieceBB(KING, side), 0, targetBB & ~kingMovesBB(kpos[side ^ 1]));
}

void position_t::genCheckEvasions(movelist_t<256>& mvlist) {
    const int xside = side ^ 1;
    const int ksq = kpos[side];
    const uint64_t checkers = getAttacksBB(ksq, xside);

    uint64_t mv_bits = kingMovesBB(ksq) & ~colorBB[side] & ~kingMovesBB(kpos[xside]);
    while (mv_bits) {
        int to = popFirstBit(mv_bits);
        if (!sqIsAttacked(occupiedBB ^ BitMask[ksq], to, xside))
            mvlist.add(move_t(ksq, to, MF_NORMAL));
    }

    if (checkers & (checkers - 1)) return;

    const int sqchecker = getFirstBit(checkers);
    const uint64_t notpinned = ~pinnedPieces(side);

    uint64_t pcbits = getPieceBB(PAWN, side) & notpinned & pawnAttacksBB(sqchecker, xside);
    genMovesN(pawnAttacksBB, pcbits & ~Rank7ByColorBB[side], side, BitMask[sqchecker]);
    genMovesProm(pawnAttacksBB, pcbits & Rank7ByColorBB[side], side, BitMask[sqchecker]);

    if (BitMask[sqchecker] & getPieceBB(PAWN, xside) && (sqchecker + ((side == WHITE) ? 8 : -8)) == stack.epsq)
        genMoves(pawnAttacksBB, pawnAttacksBB(stack.epsq, xside) & getPieceBB(PAWN, side) & notpinned, side, BitMask[stack.epsq], MF_ENPASSANT);

    genMovesN(knightAttacksBB, getPieceBB(KNIGHT, side) & notpinned & knightMovesBB(sqchecker), 0, BitMask[sqchecker]);
    genMovesN(bishopAttacksBB, getPieceBB(BISHOP, side) & notpinned & bishopAttacksBB(sqchecker, occupiedBB), occupiedBB, BitMask[sqchecker]);
    genMovesN(rookAttacksBB, getPieceBB(ROOK, side) & notpinned & rookAttacksBB(sqchecker, occupiedBB), occupiedBB, BitMask[sqchecker]);
    genMovesN(queenAttacksBB, getPieceBB(QUEEN, side) & notpinned & queenAttacksBB(sqchecker, occupiedBB), occupiedBB, BitMask[sqchecker]);

    if (!(checkers & (piecesBB[QUEEN] | piecesBB[ROOK] | piecesBB[BISHOP]) & colorBB[xside])) return;

    const uint64_t inbetweenBB = InBetween[sqchecker][ksq];

    pcbits = getPieceBB(PAWN, side) & notpinned & ShiftPtr[side ^ 1](inbetweenBB, 8);
    genMovesN(pawnMovesBB, pcbits & ~Rank7ByColorBB[side], side, inbetweenBB);
    genMovesProm(pawnMovesBB, pcbits & Rank7ByColorBB[side], side, inbetweenBB);

    pcbits = getPieceBB(PAWN, side) & Rank2ByColorBB[side] & notpinned & ShiftPtr[side ^ 1](~occupiedBB, 8)
        & ShiftPtr[side ^ 1](~occupiedBB, 16) & ShiftPtr[side ^ 1](inbetweenBB, 16);
    genMoves(pawnMoves2BB, pcbits, side, inbetweenBB, MF_PAWN2);

    genMovesN(knightAttacksBB, getPieceBB(KNIGHT, side) & notpinned, 0, inbetweenBB);
    genMovesN(bishopAttacksBB, getPieceBB(BISHOP, side) & notpinned, occupiedBB, inbetweenBB);
    genMovesN(rookAttacksBB, getPieceBB(ROOK, side) & notpinned, occupiedBB, inbetweenBB);
    genMovesN(queenAttacksBB, getPieceBB(QUEEN, side) & notpinned, occupiedBB, inbetweenBB);
}

