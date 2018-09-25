/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "typedefs.h"
#include "constants.h"
#include "attacks.h"
#include "utils.h"
#include "bitutils.h"
#include "search.h"
#include "position.h"

using namespace Attacks;
using namespace BitUtils;
using namespace PositionData;

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

#define genMoves(ML, FNC, PCB, SP, TBB, F) \
	for (uint64_t bits = PCB; bits;) { \
		int from = popFirstBit(bits); \
		for (uint64_t mvbits = FNC(from, SP) & TBB; mvbits;) \
			ML.add(move_t(from, popFirstBit(mvbits), F)); }

#define genMovesProm(ML, FNC, PCB, SP, TBB) \
	for (uint64_t bits = PCB; bits;) { \
		int from = popFirstBit(bits); \
		for (uint64_t mvbits = FNC(from, SP) & TBB; mvbits;) { \
			int to = popFirstBit(mvbits); \
			ML.add(move_t(from, to, MF_PROMQ)); \
			ML.add(move_t(from, to, MF_PROMR)); \
			ML.add(move_t(from, to, MF_PROMB)); \
			ML.add(move_t(from, to, MF_PROMN)); }}

void position_t::genQuietMoves(movelist_t<256>& mvlist) {
    uint64_t pcbits;

    if (stack.castle & (side ? BCKS : WCKS) && !(occupiedBB & CastleSquareMask1[side][0]) && !areaIsAttacked(side ^ 1, CastleSquareMask2[side][0]))
        mvlist.add(move_t(CastleSquareFrom[side], CastleSquareTo[side][0], MF_CASTLE));
    if (stack.castle & (side ? BCQS : WCQS) && !(occupiedBB & CastleSquareMask1[side][1]) && !areaIsAttacked(side ^ 1, CastleSquareMask2[side][1]))
        mvlist.add(move_t(CastleSquareFrom[side], CastleSquareTo[side][1], MF_CASTLE));

    pcbits = (getPieceBB(PAWN, side) & ~Rank7ByColorBB[side]) & ShiftPtr[side ^ 1](~occupiedBB, 8);
    genMoves(mvlist, pawnMovesBB, pcbits, side, ~occupiedBB, MF_NORMAL);

    pcbits = (getPieceBB(PAWN, side) & Rank2ByColorBB[side]) & ShiftPtr[side ^ 1](~occupiedBB, 8) & ShiftPtr[side ^ 1](~occupiedBB, 16);
    genMoves(mvlist, pawnMoves2BB, pcbits, side, ~occupiedBB, MF_PAWN2);

    genMoves(mvlist, knightAttacksBB, getPieceBB(KNIGHT, side), 0, ~occupiedBB, MF_NORMAL);
    genMoves(mvlist, bishopAttacksBB, getPieceBB(BISHOP, side), occupiedBB, ~occupiedBB, MF_NORMAL);
    genMoves(mvlist, rookAttacksBB, getPieceBB(ROOK, side), occupiedBB, ~occupiedBB, MF_NORMAL);
    genMoves(mvlist, queenAttacksBB, getPieceBB(QUEEN, side), occupiedBB, ~occupiedBB, MF_NORMAL);
    genMoves(mvlist, kingAttacksBB, getPieceBB(KING, side), 0, ~occupiedBB & ~kingMovesBB(kpos[side ^ 1]), MF_NORMAL);
}

void position_t::genTacticalMoves(movelist_t<256>& mvlist) {
    const uint64_t targetBB = colorBB[side ^ 1] & ~piecesBB[KING];

    if (stack.epsq != -1)
        genMoves(mvlist, pawnAttacksBB, getPieceBB(PAWN, side) & pawnAttacksBB(stack.epsq, side ^ 1), side, BitMask[stack.epsq], MF_ENPASSANT);

    uint64_t pcbits = getPieceBB(PAWN, side) & Rank7ByColorBB[side];
    genMovesProm(mvlist, pawnMovesBB, pcbits, side, ~occupiedBB);
    genMovesProm(mvlist, pawnAttacksBB, pcbits, side, targetBB);

    genMoves(mvlist, pawnAttacksBB, getPieceBB(PAWN, side) & ~Rank7ByColorBB[side], side, targetBB, MF_NORMAL);
    genMoves(mvlist, knightAttacksBB, getPieceBB(KNIGHT, side), 0, targetBB, MF_NORMAL);
    genMoves(mvlist, bishopAttacksBB, getPieceBB(BISHOP, side), occupiedBB, targetBB, MF_NORMAL);
    genMoves(mvlist, rookAttacksBB, getPieceBB(ROOK, side), occupiedBB, targetBB, MF_NORMAL);
    genMoves(mvlist, queenAttacksBB, getPieceBB(QUEEN, side), occupiedBB, targetBB, MF_NORMAL);
    genMoves(mvlist, kingAttacksBB, getPieceBB(KING, side), 0, targetBB & ~kingMovesBB(kpos[side ^ 1]), MF_NORMAL);
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
    genMoves(mvlist, pawnAttacksBB, pcbits & ~Rank7ByColorBB[side], side, BitMask[sqchecker], MF_NORMAL);
    pcbits &= Rank7ByColorBB[side];
    genMovesProm(mvlist, pawnAttacksBB, pcbits, side, BitMask[sqchecker]);

    if (BitMask[sqchecker] & getPieceBB(PAWN, xside) && (sqchecker + ((side == WHITE) ? 8 : -8)) == stack.epsq)
        genMoves(mvlist, pawnAttacksBB, pawnAttacksBB(stack.epsq, xside) & getPieceBB(PAWN, side) & notpinned, side, BitMask[stack.epsq], MF_ENPASSANT);

    genMoves(mvlist, knightAttacksBB, getPieceBB(KNIGHT, side) & notpinned & knightMovesBB(sqchecker), 0, BitMask[sqchecker], MF_NORMAL);
    genMoves(mvlist, bishopAttacksBB, getPieceBB(BISHOP, side) & notpinned & bishopAttacksBB(sqchecker, occupiedBB), occupiedBB, BitMask[sqchecker], MF_NORMAL);
    genMoves(mvlist, rookAttacksBB, getPieceBB(ROOK, side) & notpinned & rookAttacksBB(sqchecker, occupiedBB), occupiedBB, BitMask[sqchecker], MF_NORMAL);
    genMoves(mvlist, queenAttacksBB, getPieceBB(QUEEN, side) & notpinned & queenAttacksBB(sqchecker, occupiedBB), occupiedBB, BitMask[sqchecker], MF_NORMAL);

    if (!(checkers & (piecesBB[QUEEN] | piecesBB[ROOK] | piecesBB[BISHOP]) & colorBB[xside])) return;

    const uint64_t inbetweenBB = InBetween[sqchecker][ksq];

    pcbits = getPieceBB(PAWN, side) & notpinned & ShiftPtr[side ^ 1](inbetweenBB, 8);
    genMoves(mvlist, pawnMovesBB, pcbits & ~Rank7ByColorBB[side], side, inbetweenBB, MF_NORMAL);
    pcbits &= Rank7ByColorBB[side];
    genMovesProm(mvlist, pawnMovesBB, pcbits, side, inbetweenBB);

    pcbits = getPieceBB(PAWN, side) & Rank2ByColorBB[side] & notpinned & ShiftPtr[side ^ 1](~occupiedBB, 8)
        & ShiftPtr[side ^ 1](~occupiedBB, 16) & ShiftPtr[side ^ 1](inbetweenBB, 16);
    genMoves(mvlist, pawnMoves2BB, pcbits, side, inbetweenBB, MF_PAWN2);

    genMoves(mvlist, knightAttacksBB, getPieceBB(KNIGHT, side) & notpinned, 0, inbetweenBB, MF_NORMAL);
    genMoves(mvlist, bishopAttacksBB, getPieceBB(BISHOP, side) & notpinned, occupiedBB, inbetweenBB, MF_NORMAL);
    genMoves(mvlist, rookAttacksBB, getPieceBB(ROOK, side) & notpinned, occupiedBB, inbetweenBB, MF_NORMAL);
    genMoves(mvlist, queenAttacksBB, getPieceBB(QUEEN, side) & notpinned, occupiedBB, inbetweenBB, MF_NORMAL);
}

bool position_t::areaIsAttacked(int c, uint64_t target) {
    while (target)
        if (sqIsAttacked(occupiedBB, popFirstBit(target), c))
            return true;
    return false;
}

bool position_t::sqIsAttacked(uint64_t occ, int sq, int c) {
    return
        (rookAttacksBB(sq, occ) & ((piecesBB[ROOK] | piecesBB[QUEEN]) & colorBB[c])) ||
        (bishopAttacksBB(sq, occ) & ((piecesBB[BISHOP] | piecesBB[QUEEN]) & colorBB[c])) ||
        (knightMovesBB(sq) & piecesBB[KNIGHT] & colorBB[c]) ||
        ((pawnAttacksBB(sq, c ^ 1) & piecesBB[PAWN] & colorBB[c])) ||
        (kingMovesBB(sq) & piecesBB[KING] & colorBB[c]);
}

uint64_t position_t::pieceAttacksFromBB(int pc, int sq, uint64_t occ) {
    switch (pc) {
    case PAWN: return pawnAttacksBB(sq, side);
    case KNIGHT: return knightMovesBB(sq);
    case BISHOP: return bishopAttacksBB(sq, occ);
    case ROOK: return rookAttacksBB(sq, occ);
    case QUEEN: return queenAttacksBB(sq, occ);
    case KING: return kingMovesBB(sq);
    }
    return 0;
}

uint64_t position_t::getAttacksBB(int sq, int c) {
    return colorBB[c] & ((pawnAttacksBB(sq, c ^ 1) & piecesBB[PAWN]) |
        (knightMovesBB(sq) & piecesBB[KNIGHT]) |
        (kingMovesBB(sq) & piecesBB[KING]) |
        (bishopAttacksBB(sq, occupiedBB) & (piecesBB[BISHOP] | piecesBB[QUEEN])) |
        (rookAttacksBB(sq, occupiedBB) & (piecesBB[ROOK] | piecesBB[QUEEN])));
}

bool position_t::kingIsInCheck() {
    return sqIsAttacked(occupiedBB, kpos[side], side ^ 1);
}

uint64_t position_t::pinnedPieces(int c) {
    uint64_t pinned = 0;
    int ksq = kpos[c];

    uint64_t pinners = getRookSlidersBB(c ^ 1);
    pinners &= pinners ? rookAttacksBBX(ksq, occupiedBB) : 0;
    while (pinners) pinned |= InBetween[popFirstBit(pinners)][ksq] & colorBB[c];

    pinners = getBishopSlidersBB(c ^ 1);
    pinners &= pinners ? bishopAttacksBBX(ksq, occupiedBB) : 0;
    while (pinners) pinned |= InBetween[popFirstBit(pinners)][ksq] & colorBB[c];

    return pinned;
}

uint64_t position_t::discoveredCheckCandidates(int c) {
    uint64_t pinned = 0;
    int ksq = kpos[c ^ 1];

    uint64_t pinners = getRookSlidersBB(c);
    pinners &= pinners ? rookAttacksBBX(ksq, occupiedBB) : 0;
    while (pinners) pinned |= InBetween[popFirstBit(pinners)][ksq] & colorBB[c] & ~getRookSlidersBB(c);

    pinners = getBishopSlidersBB(c);
    pinners &= pinners ? bishopAttacksBBX(ksq, occupiedBB) : 0;
    while (pinners) pinned |= InBetween[popFirstBit(pinners)][ksq] & colorBB[c] & ~getBishopSlidersBB(c);;

    return pinned;
}

bool position_t::moveIsLegal(move_t move, uint64_t pinned, bool incheck) {
    if (incheck) return true;
    if (move.isCastle()) return true;

    int xside = side ^ 1;
    int from = move.moveFrom();
    int to = move.moveTo();
    int ksq = kpos[side];

    if (move.isEnPassant()) {
        uint64_t b = occupiedBB ^ BitMask[from] ^ BitMask[(sqRank(from) << 3) + sqFile(to)] ^ BitMask[to];
        return !(rookAttacksBB(ksq, b) & getRookSlidersBB(xside)) && !(bishopAttacksBB(ksq, b) & getBishopSlidersBB(xside));
    }
    if (from == ksq) return !(sqIsAttacked(occupiedBB ^ BitMask[ksq], to, xside));
    if (!(pinned & BitMask[from])) return true;
    if (DirFromTo[from][ksq] == DirFromTo[to][ksq]) return true;
    return false;
}

bool position_t::moveIsCheck(move_t move, uint64_t dcc) {
    const int xside = side ^ 1;
    const int from = move.moveFrom();
    const int to = move.moveTo();
    const int enemy_ksq = kpos[xside];
    const int pc = pieces[from];
    const int prom = move.movePromote();

    if ((dcc & BitMask[from]) && DirFromTo[from][enemy_ksq] != DirFromTo[to][enemy_ksq]) return true;
    if (pc != PAWN && pc != KING && pieceAttacksFromBB(pc, enemy_ksq, occupiedBB)  & BitMask[to]) return true;
    if (pc == PAWN && pawnAttacksBB(enemy_ksq, xside) & BitMask[to]) return true;
    uint64_t tempOccBB = occupiedBB ^ BitMask[from] ^ BitMask[to];
    if (move.isPromote() && pieceAttacksFromBB(prom, enemy_ksq, tempOccBB)  & BitMask[to]) return true;
    if (move.isEnPassant() && sqIsAttacked(tempOccBB ^ BitMask[(sqRank(from) << 3) + sqFile(to)], enemy_ksq, side)) return true;
    if (move.isCastle() && rookAttacksBB(enemy_ksq, tempOccBB) & BitMask[RookTo[to / 56][(to % 8) > 5]]) return true;
    return false;
}

bool position_t::moveIsValid(move_t m, uint64_t pinned) {
    const int from = m.moveFrom();
    const int to = m.moveTo();
    const int pc = pieces[from];
    const int us = side;
    const int cap = pieces[to];
    const int absdiff = abs(from - to);
    const int flag = m.moveFlags();
    const int prom = m.movePromote();

    if (m.m == 0) return false;
    if (pc < PAWN || pc > KING) return false;
    if (cap < EMPTY || cap > QUEEN) return false;
    if (prom != EMPTY && (prom < KNIGHT || prom > QUEEN)) return false;
    if (from < 0 || from > 63) return false;
    if (to < 0 || to > 63) return false;
    if (getSide(from) != us) return false;
    if (cap != EMPTY && getSide(to) == us) return false;
    if (prom != EMPTY && flag != MF_PROMB && flag != MF_PROMN && flag != MF_PROMR && flag != MF_PROMQ) return false;
    if ((pinned & BitMask[from]) && (DirFromTo[from][kpos[us]] != DirFromTo[to][kpos[us]])) return false;
    if (pc != PAWN && (pc != KING || absdiff != 2) && !(pieceAttacksFromBB(pc, from, occupiedBB) & BitMask[to])) return false;
    if (pc == KING) {
        if (BitMask[to] & kingMovesBB(kpos[us ^ 1])) return false;
        if (absdiff == 2 && flag != MF_CASTLE) return false;
    }
    if (pc == PAWN) {
        if (cap != EMPTY && !(pawnAttacksBB(from, us) & BitMask[to])) return false;
        if (cap == EMPTY && to != stack.epsq && !((pawnMovesBB(from, us) | pawnMoves2BB(from, us)) & BitMask[to])) return false;
        if (absdiff == 16 && flag != MF_PAWN2) return false;
        if (to == stack.epsq && flag != MF_ENPASSANT) return false;
    }

    switch (flag) {
    case MF_PAWN2: {
        if (pc != PAWN) return false;
        if (!(Rank2ByColorBB[us] & BitMask[from])) return false;
        if (absdiff != 16) return false;
        if (cap != EMPTY) return false;
        if (pieces[(from + to) / 2] != EMPTY) return false;
    } break;
    case MF_ENPASSANT: {
        if (pc != PAWN) return false;
        if (cap != EMPTY) return false;
        if (to != stack.epsq) return false;
        if (pieces[(sqRank(from) << 3) + sqFile(to)] != PAWN) return false;
        if (absdiff != 9 && absdiff != 7) return false;
    } break;
    case MF_CASTLE: {
        if (pc != KING) return false;
        if (absdiff != 2) return false;
        if (from != E1 && from != E8) return false;
        if (pieces[RookFrom[to / 56][(to % 8) > 5]] != ROOK) return false;
        if (to > from) {
            if (!(stack.castle & (us ? BCKS : WCKS))) return false;
            if (occupiedBB & CastleSquareMask1[us][0]) return false;
            if (areaIsAttacked(us ^ 1, CastleSquareMask2[us][0])) return false;
        }
        if (to < from) {
            if (!(stack.castle & (us ? BCQS : WCQS))) return false;
            if (occupiedBB & CastleSquareMask1[us][1]) return false;
            if (areaIsAttacked(us ^ 1, CastleSquareMask2[us][1])) return false;
        }
    } break;
    case MF_PROMN: case MF_PROMB: case MF_PROMR: case MF_PROMQ: {
        if (pc != PAWN) return false;
        if (prom == EMPTY) return false;
        if (!(Rank7ByColorBB[us] & BitMask[from])) return false;
    } break;
    }
    return true;
}

bool position_t::moveIsTactical(move_t m) {
    return pieces[m.moveTo()] != EMPTY || m.isPromote() || m.isEnPassant();
}