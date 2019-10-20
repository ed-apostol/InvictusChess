/**************************************************/
/*  Invictus 2019						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "eval.h"
#include "bitutils.h"
#include "log.h"
#include "constants.h"
#include "attacks.h"

#include <algorithm>

using namespace BitUtils;
using namespace Attacks;

namespace {
    score_t pawnPST(int sq) {
        score_t file[8] = { { -4, 5 },{ -2, 3 },{ 0, 1 },{ 2, -1 },{ 2, -1 },{ 0, 1 },{ -2, 3 },{ -4, 5 } };
        score_t rank[8] = { { 0, 0 },{ -1, -10 },{ 0, -5 },{ 1, 0 },{ 1 ,10 },{ 0, 20 },{ 0, 30 },{ 0, 0 } };
        return file[sqFile(sq)] + rank[sqRank(sq)];
    }
    score_t knightPST(int sq) {
        score_t file[8] = { { -26, -10 },{ -9, -3 },{ 2, 0 },{ 5, 2 },{ 5, 2 },{ 2, 0 },{ -9, -3 },{ -26, -10 } };
        score_t rank[8] = { { -30, -10 },{ -9, -4 },{ 6, -1 },{ 16, 2 },{ 20, 4 },{ 19, 6 },{ 11, 3 },{ -11, -5 } };
        return file[sqFile(sq)] + rank[sqRank(sq)];
    }
    score_t bishopPST(int sq) {
        score_t rank[8] = { { -7, -5 },{ -3, -2 },{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 },{ -3, -2 },{ -7, -5 } };
        score_t file[8] = { { -7, -5 },{ -3, -2 },{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 },{ -3, -2 },{ -7, -5 } };
        return file[sqFile(sq)] + rank[sqRank(sq)];
    }
    score_t rookPST(int sq) {
        score_t file[8] = { { -1, 0 },{ -1, 0 },{ 4, 0 },{ 7, 0 },{ 7, 0 },{ 4, 0 },{ -1, 0 },{ -1, 0 } };
        return file[sqFile(sq)];
    }
    score_t queenPST(int sq) {
        score_t file[8] = { { -3, -3 },{ 0, 0 },{ 1, 1 },{ 3, 3 },{ 3, 3 },{ 1, 1 },{ 0, 0 },{ -3, -3 } };
        score_t rank[8] = { { -7, -3 },{ 0, 0 },{ 0, 1 },{ 1, 3 },{ 1, 3 },{ 0, 1 },{ 0, 0 },{ -1, -3 } };
        return file[sqFile(sq)] + rank[sqRank(sq)];
    }
    score_t kingPST(int sq) {
        score_t file[8] = { { 26, -13 },{ 30, 1 },{ 0, 11 },{ -20, 16 },{ -20, 16 },{ 0, 11 },{ 30, 1 },{ 26, -13 } };
        score_t rank[8] = { {20, -29 },{ -5, -4 },{ -25, 1 },{ -29, 6 },{ -33, 10 },{ -37, 6 },{ -37, 1 },{ -37, -10 } };
        return file[sqFile(sq)] + rank[sqRank(sq)];
    }
    inline int distance(int a, int b) {
        return std::max(abs(sqFile(a) - sqFile(b)), abs(sqRank(a) - sqRank(b)));
    }
    inline score_t hyperbolic(score_t min, score_t max, int r) {
        static const int bonus[8] = { 0, 0, 0, 13, 34, 77, 128, 0 };
        return (min + ((max - min) * bonus[r]) / bonus[6]);
    }
}

namespace EvalPar {
    static const int FileWing[8] = { 0, 0, 0, 1, 1, 2, 2, 2 };
    const score_t mat_values[7] = { { 0, 0 },{ 100, 125 },{ 460, 390 },{ 470, 420 },{ 640, 720 },{ 1310, 1350 },{ 0, 0 } };

    score_t PawnConnected = { 3, 3 };
    score_t PawnDoubled = { 8, 10 };
    score_t PawnIsolated = { 12, 16 };
    score_t PawnBackward = { 15, 10 };

    score_t PasserBonusMin = { 20, 20 };
    score_t PasserBonusMax = { 140, 140 };
    score_t PasserDistOwn = { 0, 20 };
    score_t PasserDistEnemy = { 0, 40 };
    score_t PasserNotBlocked = { 40, 60 };
    score_t PasserSafePush = { 40, 60 };

    score_t KnightMob = { 6, 8 };
    score_t BishopMob = { 3, 3 };
    score_t RookMob = { 1, 2 };
    score_t QueenMob = { 1, 2 };

    score_t NumKZoneAttacks = { 10, 0 };
    score_t AttackWeights = { 12, 0 };
    score_t ShelterBonus = { 7, 0 };

    score_t AllxPawns = { 12, 24 };
    score_t PawnsxMinors = { 50, 50 };
    score_t MinorsxMinors = { 25, 35 };
    score_t MajorsxWeakMinors = { 25, 45 };
    score_t PawnsMinorsxMajors = { 50, 20 };
    score_t AllxQueens = { 40, 30 };
    score_t PawnPushxAll = { 15, 20 };

    int KnightAtk = 2;
    int BishopAtk = 2;
    int RookAtk = 3;
    int QueenAtk = 5;
    score_t pst[2][8][64];
    uint64_t KingZoneBB[2][64];
    uint64_t KingShelterBB[2][3];
    uint64_t KingShelter2BB[2][3];

    void initArr() {
        std::function<score_t(int)> pstInit[] = { pawnPST, knightPST,bishopPST,rookPST,queenPST,kingPST };
        memset(pst, 0, sizeof(pst));
        for (int pc = PAWN; pc <= KING; ++pc) {
            for (int sq = 0; sq < 64; ++sq) {
                int rsq = ((7 - sqRank(sq)) * 8) + sqFile(sq);
                pst[WHITE][pc][sq] = pstInit[pc - 1](sq);
                pst[BLACK][pc][sq] = pstInit[pc - 1](rsq);
            }
        }
        for (int sq = 0; sq < 64; ++sq) {
            for (int color = WHITE; color <= BLACK; ++color) {
                KingZoneBB[color][sq] = kingMovesBB(sq) | (1ull << sq) | shiftBB[color](kingMovesBB(sq), 8);
                KingZoneBB[color][sq] |= sqFile(sq) == FileA ? KingZoneBB[color][sq] << 1 : 0;
                KingZoneBB[color][sq] |= sqFile(sq) == FileH ? KingZoneBB[color][sq] >> 1 : 0;
            }
        }
        const int KingSquare[2][3] = { {B1, E1, G1}, {B8, E8, G8} };
        for (int color = WHITE; color <= BLACK; ++color) {
            for (int castle = 0; castle <= 2; ++castle) {
                KingShelterBB[color][castle] = kingMovesBB(KingSquare[color][castle]) & Rank2ByColorBB[color];
                KingShelter2BB[color][castle] = shiftBB[color](KingShelterBB[color][castle], 8);
            }
        }
    }
    void displayPSTbyPC(score_t A[], std::string piece, bool midgame) {
        LogAndPrintOutput() << piece << ":";
        for (int r = 56; r >= 0; r -= 8) {
            LogAndPrintOutput logger;
            for (int f = 0; f <= 7; ++f) {
                logger << (midgame ? A[r + f].m : A[r + f].e) << " ";
            }
        }
        LogAndPrintOutput() << "\n\n";
    }
    void displayPST() {
        static const std::string colstr[2] = { "WHITE", "BLACK" };
        static const std::string pcstr[7] = { "EMPTY", "PAWN", "KNIGHT", "BISHOP", "ROOK", "QUEEN", "KING" };
        for (int c = 0; c <= 1; ++c) {
            for (int pc = 1; pc <= 6; ++pc) {
                LogAndPrintOutput() << "MIDGAME";
                displayPSTbyPC(pst[c][pc], colstr[c] + " " + pcstr[pc], true);
                LogAndPrintOutput() << "ENDGAME";
                displayPSTbyPC(pst[c][pc], colstr[c] + " " + pcstr[pc], false);
            }
        }
    }
}

using namespace EvalPar;

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
    if (tot_atkrs >= 2 && kzone_atkcnt >= 1) {
        scr[side] += NumKZoneAttacks * kzone_atkcnt;
        scr[side] += AttackWeights * ((kingzoneatks[side] & ((1 << 20) - 1)) >> 10);
    }
}

void eval_t::threats(position_t& p, int side) {
    const int xside = side ^ 1;
    uint64_t minors = p.pieceBB(KNIGHT, xside) | p.pieceBB(BISHOP, xside);
    uint64_t weak = (allatks[side] & ~allatks[xside]) | (allatks2[side] & ~allatks2[xside] & ~pawnatks[xside]);
    scr[side] += AllxPawns * bitCnt(p.pieceBB(PAWN, xside) & weak);
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
        score_t local_scr = PasserDistEnemy * distance(p.kpos[xside], sq);
        local_scr -= PasserDistOwn * distance(p.kpos[side], sq);
        if (pawnMovesBB(sq, side) & ~p.occupiedBB) local_scr += PasserNotBlocked;
        if (pawnMovesBB(sq, side) & ~allatks[xside]) local_scr += PasserSafePush;
        scr[side] += hyperbolic(PasserBonusMin, PasserBonusMax + local_scr, side == WHITE ? sqRank(sq) : 7 - sqRank(sq));
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