/**************************************************/
/*  Invictus 2019                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "eval.h"
#include "bitutils.h"
#include "log.h"
#include "constants.h"
#include "attacks.h"
#include "params.h"

#include <algorithm>

using namespace BitUtils;
using namespace Attacks;
using namespace EvalParam;

namespace {
    inline int distance(int a, int b) {
        return std::max(abs(sqFile(a) - sqFile(b)), abs(sqRank(a) - sqRank(b)));
    }
    inline score_t hyperbolic(score_t min, score_t max, int r) {
        static const int bonus[8] = { 0, 0, 0, 13, 34, 77, 128, 0 };
        return min + ((max * bonus[r]) / bonus[6]);
    }
}

void eval_t::pawnstructure(position_t& p, int side) {
    const int xside = side ^ 1;
    const uint64_t pawns = p.pieceBB(PAWN, side);
    const uint64_t xpawns = p.pieceBB(PAWN, xside);
    const uint64_t connected = pawns & (pawnatks[side] | shiftBB[xside](pawnatks[side], 8));
    const uint64_t doubled = pawns & fillBBEx[xside](pawns);
    const uint64_t isolated = pawns & ~fillBB[side](fillBB[xside](pawnatks[side]));
    const uint64_t backward = shiftBB[xside](shiftBB[side](pawns, 8) & (fillBB[xside](pawnatks[xside]) | xpawns) & ~fillBB[side](pawnatks[side]), 8) & ~isolated;

    scr[side] += PawnConnected * bitCnt(connected);
    scr[side] -= PawnDoubled * bitCnt(doubled);
    scr[side] -= PawnIsolated * bitCnt(isolated);
    scr[side] -= PawnBackward * bitCnt(backward);
}

void eval_t::mobility(position_t& p, int side) {
    const int xside = side ^ 1;
    uint64_t mobmask = ~p.colorBB[side] & ~pawnatks[xside];
    for (uint64_t pcbits = p.pieceBB(KNIGHT, side); pcbits;) {
        int sq = popFirstBit(pcbits);
        uint64_t atk = knightMovesBB(sq);
        knightatks[side] |= atk;
        allatks2[side] |= allatks[side] & atk;
        allatks[side] |= atk;
        scr[side] += KnightMob * bitCnt(atk & mobmask);
        if (atk & kingzone[xside]) kingzoneatks[side] += (1 << 20) + (KnightAtk << 10) + bitCnt(atk & kingzone[xside]);
    }
    for (uint64_t pcbits = p.pieceBB(BISHOP, side); pcbits;) {
        int sq = popFirstBit(pcbits);
        uint64_t atk = bishopAttacksBB(sq, p.occupiedBB);
        bishopatks[side] |= atk;
        allatks2[side] |= allatks[side] & atk;
        allatks[side] |= atk;
        scr[side] += BishopMob * bitCnt(atk & mobmask);
        if (atk & kingzone[xside]) kingzoneatks[side] += (1 << 20) + (BishopAtk << 10) + bitCnt(atk & kingzone[xside]);
    }
    for (uint64_t pcbits = p.pieceBB(ROOK, side); pcbits;) {
        int sq = popFirstBit(pcbits);
        uint64_t atk = rookAttacksBB(sq, p.occupiedBB);
        rookatks[side] |= atk;
        allatks2[side] |= allatks[side] & atk;
        allatks[side] |= atk;
        scr[side] += RookMob * bitCnt(atk & mobmask);
        if (atk & kingzone[xside]) kingzoneatks[side] += (1 << 20) + (RookAtk << 10) + bitCnt(atk & kingzone[xside]);
    }
    for (uint64_t pcbits = p.pieceBB(QUEEN, side); pcbits;) {
        int sq = popFirstBit(pcbits);
        uint64_t atk = queenAttacksBB(sq, p.occupiedBB);
        queenatks[side] |= atk;
        allatks2[side] |= allatks[side] & atk;
        allatks[side] |= atk;
        scr[side] += QueenMob * bitCnt(atk & mobmask);
        if (atk & kingzone[xside]) kingzoneatks[side] += (1 << 20) + (QueenAtk << 10) + bitCnt(atk & kingzone[xside]);
    }
}

void eval_t::kingsafety(position_t& p, int side) {
    static const int FileWing[8] = { 0, 0, 0, 1, 1, 2, 2, 2 };

    if (!p.pieceBB(QUEEN, side)) return;
    if (!p.pieceBB(ROOK, side) || !p.pieceBB(BISHOP, side) || !p.pieceBB(KNIGHT, side)) return;

    const int xside = side ^ 1;
    int curr_shelter = bitCnt(KingShelterBB[xside][FileWing[sqFile(p.kpos[xside])]] & p.pieceBB(PAWN, xside)) * 2;
    curr_shelter += bitCnt(KingShelter2BB[xside][FileWing[sqFile(p.kpos[xside])]] & p.pieceBB(PAWN, xside));
    int best_shelter = curr_shelter;
    if (p.stack.castle & (xside ? BCQS : WCQS)) {
        int shelter = bitCnt(KingShelterBB[xside][0] & p.pieceBB(PAWN, xside)) * 2;
        shelter += bitCnt(KingShelter2BB[xside][0] & p.pieceBB(PAWN, xside));
        if (shelter > best_shelter) best_shelter = shelter;
    }
    if (p.stack.castle & (xside ? BCKS : WCKS)) {
        int shelter = bitCnt(KingShelterBB[xside][2] & p.pieceBB(PAWN, xside)) * 2;
        shelter += bitCnt(KingShelter2BB[xside][2] & p.pieceBB(PAWN, xside));
        if (shelter > best_shelter) best_shelter = shelter;
    }
    scr[side] -= ShelterBonus * ((best_shelter + curr_shelter) / 2);

    int tot_atkrs = kingzoneatks[side] >> 20;
    int kzone_atkcnt = kingzoneatks[side] & ((1 << 10) - 1);
    int pc_atkweights = (kingzoneatks[side] >> 10) & ((1 << 10) - 1);
    if (tot_atkrs >= 2 && kzone_atkcnt >= 1) {
        scr[side] += NumKZoneAttacks * ((kzone_atkcnt * pc_atkweights) / tot_atkrs);
    }
}

void eval_t::threats(position_t& p, int side) {
    const int xside = side ^ 1;
    const uint64_t minors = p.pieceBB(KNIGHT, xside) | p.pieceBB(BISHOP, xside);
    const uint64_t weak = (allatks[side] & ~allatks[xside]) | (allatks2[side] & ~allatks2[xside] & ~pawnatks[xside]);

    scr[side] += PawnsxMinors * bitCnt(pawnatks[side] & minors);
    scr[side] += MinorsxMinors * bitCnt((knightatks[side] | bishopatks[side]) & minors);
    scr[side] += MajorsxWeakMinors * bitCnt((rookatks[side] | queenatks[side]) & minors & weak);
    scr[side] += PawnsMinorsxMajors * bitCnt((pawnatks[side] | knightatks[side] | bishopatks[side]) & (p.pieceBB(ROOK, xside) | p.pieceBB(QUEEN, xside)));
    scr[side] += AllxQueens * bitCnt(allatks[side] & p.pieceBB(QUEEN, xside));
}

void eval_t::passedpawns(position_t& p, int side) {
    const int xside = side ^ 1;
    uint64_t passers = p.pieceBB(PAWN, side) & ~fillBBEx[xside](p.pieceBB(PAWN)) & ~fillBB[xside](pawnatks[xside]);
    while (passers) {
        int sq = popFirstBit(passers);
        score_t local = PasserBonusMax;
        local += PasserDistEnemy * distance(p.kpos[xside], sq);
        local -= PasserDistOwn * distance(p.kpos[side], sq);
        if (pawnMovesBB(sq, side) & ~p.occupiedBB) local += PasserNotBlocked;
        if (pawnMovesBB(sq, side) & ~allatks[xside]) local += PasserSafePush;
        scr[side] += hyperbolic(PasserBonusMin, local, side == WHITE ? sqRank(sq) : 7 - sqRank(sq));
    }
}

int eval_t::score(position_t& p) {
    for (int color = WHITE; color <= BLACK; ++color) {
        scr[color] = p.stack.score[color];
        allatks2[color] = kingzoneatks[color] = knightatks[color] = bishopatks[color] = rookatks[color] = queenatks[color] = 0;
        kingzone[color] = KingZoneBB[color][p.kpos[color]];
        allatks[color] = kingMovesBB(p.kpos[color]);
        pawnatks[color] = pawnAttackBB(p.pieceBB(PAWN, color), color);
        allatks2[color] |= allatks[color] & pawnatks[color];
        allatks[color] |= pawnatks[color];
    }
    for (int color = WHITE; color <= BLACK; ++color) {
        pawnstructure(p, color);
        mobility(p, color);
        kingsafety(p, color);
        passedpawns(p, color);
        threats(p, color);
    }

    int phase = 4 * bitCnt(p.piecesBB[QUEEN]) + 2 * bitCnt(p.piecesBB[ROOK]) + 1 * bitCnt(p.piecesBB[KNIGHT] | p.piecesBB[BISHOP]);
    score_t s = scr[p.side] - scr[p.side ^ 1];
    return ((s.m*phase) + (s.e*(24 - phase))) / 24;
}