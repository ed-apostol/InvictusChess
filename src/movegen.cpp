/**************************************************/
/*  Invictus 2021                                 */
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

enum move_type {
    MT_PAWN, MT_PAWN2, MT_PAWNCAP, MT_PAWNPROM, MT_PAWNCAPPROM, MT_EP, MT_KNIGHT, MT_BISHOP, MT_ROOK, MT_QUEEM, MT_KING
};
static const std::function<uint64_t(int, uint64_t)> AttackFuncs[11] = {
    pawnMovesBB, pawnMoves2BB, pawnAttacksBB, pawnMovesBB, pawnAttacksBB, pawnAttacksBB,
    knightAttacksBB, bishopAttacksBB, rookAttacksBB, queenAttacksBB, kingAttacksBB
};
static const PieceTypes Pieces[11] = {
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};
static const MoveFlags Flags[11] = {
    MF_NORMAL, MF_PAWN2, MF_NORMAL, MF_PROMN, MF_PROMN, MF_ENPASSANT, MF_NORMAL, MF_NORMAL, MF_NORMAL, MF_NORMAL, MF_NORMAL
};

#define genMoves(mt, PCB, SP, TBB)\
    for (uint64_t from, bits = getPieceBB(Pieces[mt], side) & PCB; bits;) \
        for (uint64_t mvbits = AttackFuncs[mt](int(from = popFirstBit(bits)), SP) & TBB; mvbits;) \
            mvlist.add(move_t(int(from), popFirstBit(mvbits), Flags[mt] ));

#define genProms(mt, PCB, SP, TBB)\
    for (uint64_t from, bits = getPieceBB(Pieces[mt], side) & PCB; bits;) \
        for (uint64_t mvbits = AttackFuncs[mt](int(from = popFirstBit(bits)), SP) & TBB; mvbits;) \
            for (int to = popFirstBit(mvbits), fl = MF_PROMN; fl <= MF_PROMQ; ++fl) \
                mvlist.add(move_t(int(from), to, fl));

#define genMovesPcs(PCB, TBB)\
    genMoves(MT_KNIGHT, PCB, occupiedBB, TBB);\
    genMoves(MT_BISHOP, PCB, occupiedBB, TBB);\
    genMoves(MT_ROOK, PCB, occupiedBB, TBB);\
    genMoves(MT_QUEEM, PCB, occupiedBB, TBB);

void position_t::genLegal(movelist_t<220>& mvlist) {
    if (kingIsInCheck())
        genCheckEvasions(mvlist);
    else {
        movelist_t<220> mlt;
        uint64_t pinned = pinnedPiecesBB(side);
        genTacticalMoves(mlt);
        genQuietMoves(mlt);
        for (move_t m : mlt) {
            if (!moveIsLegal(m, pinned, false)) continue;
            mvlist.add(m);
        }
    }
}

void position_t::genQuietMoves(movelist_t<220>& mvlist) {
    const int xside = side ^ 1;
    if (canCastleKS(side) && !(occupiedBB & CastleSquareMask1[side][0]))
        mvlist.add(move_t(CastleSquareFrom[side], CastleSquareTo[side][0], MF_CASTLE));
    if (canCastleQS(side) && !(occupiedBB & CastleSquareMask1[side][1]))
        mvlist.add(move_t(CastleSquareFrom[side], CastleSquareTo[side][1], MF_CASTLE));

    genMoves(MT_PAWN, ~Rank7ByColorBB[side] & shift8BB[xside](~occupiedBB), side, ~occupiedBB);
    genMoves(MT_PAWN2, Rank2ByColorBB[side] & shift8BB[xside](~occupiedBB) & shift16BB[xside](~occupiedBB), side, ~occupiedBB);
    genMovesPcs(occupiedBB, ~occupiedBB);
    genMoves(MT_KING, occupiedBB, 0, ~occupiedBB & ~kingMovesBB(kpos[xside]));
}

void position_t::genTacticalMoves(movelist_t<220>& mvlist) {
    const int xside = side ^ 1;
    const uint64_t targetBB = colorBB[xside] & ~piecesBB[KING];

    if (stack.epsq != -1)
        genMoves(MT_EP, pawnAttacksBB(stack.epsq, xside), side, BitMask[stack.epsq]);

    genProms(MT_PAWNPROM, Rank7ByColorBB[side], side, ~occupiedBB);
    genProms(MT_PAWNCAPPROM, Rank7ByColorBB[side], side, targetBB);
    genMoves(MT_PAWNCAP, ~Rank7ByColorBB[side], side, targetBB);
    genMovesPcs(occupiedBB, targetBB);
    genMoves(MT_KING, occupiedBB, 0, targetBB & ~kingMovesBB(kpos[xside]));
}

void position_t::genCheckEvasions(movelist_t<220>& mvlist) {
    const int xside = side ^ 1;
    const int ksq = kpos[side];
    const uint64_t checkersBB = getAttacksBB(ksq, xside);

    genMoves(MT_KING, occupiedBB, 0, safeSqsBB(xside, occupiedBB ^ BitMask[ksq], kingMovesBB(ksq)) & ~colorBB[side] & ~kingMovesBB(kpos[xside]));

    if (checkersBB & (checkersBB - 1)) return;

    const int sqchecker = getFirstBit(checkersBB);
    const uint64_t notpinned = ~pinnedPiecesBB(side);
    const uint64_t inbetweenBB = InBetween[sqchecker][ksq];

    uint64_t pcbits = notpinned & pawnAttacksBB(sqchecker, xside);
    genMoves(MT_PAWNCAP, pcbits & ~Rank7ByColorBB[side], side, checkersBB);
    genProms(MT_PAWNCAPPROM, pcbits & Rank7ByColorBB[side], side, checkersBB);

    if (checkersBB & getPieceBB(PAWN, xside) && (sqchecker + ((side == WHITE) ? 8 : -8)) == stack.epsq)
        genMoves(MT_EP, notpinned, side, BitMask[stack.epsq]);

    genMovesPcs(notpinned, (inbetweenBB | checkersBB));

    if (!inbetweenBB) return;

    pcbits = notpinned & shift8BB[xside](inbetweenBB);
    genMoves(MT_PAWN, pcbits & ~Rank7ByColorBB[side], side, inbetweenBB);
    genProms(MT_PAWNPROM, pcbits & Rank7ByColorBB[side], side, inbetweenBB);
    pcbits = notpinned & shift8BB[xside](~occupiedBB) & shift16BB[xside](~occupiedBB) & shift16BB[xside](inbetweenBB);
    genMoves(MT_PAWN2, pcbits & Rank2ByColorBB[side], side, inbetweenBB);
}