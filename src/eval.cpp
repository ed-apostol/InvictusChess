/**************************************************/
/*  Invictus 2021                                 */
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
#include <cstring>

using namespace BitUtils;
using namespace Attacks;
using namespace EvalParam;

namespace {
    static const int FileWing[8] = { 0, 0, 0, 1, 1, 2, 2, 2 };
    inline int distance(int a, int b) {
        return std::max(abs(sqFile(a) - sqFile(b)), abs(sqRank(a) - sqRank(b)));
    }
    inline int getRelativeRank(int c, int sq) {
        return c == WHITE ? sqRank(sq) : 7 - sqRank(sq);
    }
}

void eval_t::material(position_t & p, int side) {
    scr[side] += MaterialValues[PAWN] * bitCnt(p.getPieceBB(PAWN, side));
    scr[side] += MaterialValues[KNIGHT] * bitCnt(p.getPieceBB(KNIGHT, side));
    scr[side] += MaterialValues[BISHOP] * bitCnt(p.getPieceBB(BISHOP, side));
    scr[side] += MaterialValues[ROOK] * bitCnt(p.getPieceBB(ROOK, side));
    scr[side] += MaterialValues[QUEEN] * bitCnt(p.getPieceBB(QUEEN, side));
    if (bitCnt(p.getPieceBB(BISHOP, side)) == 2) {
        scr[side] += BishopPair;
    }
}

void eval_t::pawnstructure(position_t& p, int side) {
    const int xside = side ^ 1;
    const uint64_t pawns = p.getPieceBB(PAWN, side);
    const uint64_t xpawns = p.getPieceBB(PAWN, xside);
    const uint64_t open = pawns & ~(pawns & fillBB[xside](xpawns));
    const uint64_t connected = pawns & (pawnatks[side] | shift8BB[xside](pawnatks[side]));
    const uint64_t doubled = pawns & fillBBEx[xside](pawns);
    const uint64_t isolated = pawns & ~fillBB[xside](pawnfillatks[side]);
    const uint64_t backward = pawns & ~isolated & shift8BB[xside]((pawnatks[xside] | xpawns) & ~pawnfillatks[side]);
    scr[side] += PawnConnected * bitCnt(connected);
    scr[side] -= PawnDoubled * bitCnt(doubled);
    scr[side] -= PawnIsolated * bitCnt(isolated & ~open);
    scr[side] -= PawnBackward * bitCnt(backward & ~open);
    scr[side] -= PawnIsolatedOpen * bitCnt(isolated & open);
    scr[side] -= PawnBackwardOpen * bitCnt(backward & open);
}

void eval_t::pieceactivity(position_t& p, int side) {
    static const uint64_t OutpostMask[2] = { Rank4BB | Rank5BB | Rank6BB, Rank5BB | Rank4BB | Rank3BB };
    const int xside = side ^ 1;
    const uint64_t mobmask = ~(p.getPieceBB(KING, side) | pawnatks[xside] | (shift8BB[xside](p.occupiedBB) & p.getPieceBB(PAWN, side)));
    const uint64_t outpostsqs = OutpostMask[side] & pawnatks[side] & ~pawnfillatks[xside];
    const uint64_t centersqs = D4BB | D5BB | E4BB | E5BB;

    for (uint64_t pcbits = p.getPieceBB(KNIGHT, side); pcbits;) {
        int sq = popFirstBit(pcbits);
        uint64_t atk = knightMovesBB(sq);
        knightatks[side] |= atk;
        allatks2[side] |= allatks[side] & atk;
        allatks[side] |= atk;
        scr[side] += KnightMob[bitCnt(atk & mobmask)];
        if (atk & kingzone[xside]) katkrs[side] += (1 << 10) + (int(KnightAtk) << 20) + bitCnt(atk & kingzone[xside]);
        if (BitMask[sq] & outpostsqs) scr[side] += OutpostBonus * 2;
        else if (atk & outpostsqs & ~p.colorBB[side]) scr[side] += OutpostBonus;
    }
    for (uint64_t pcbits = p.getPieceBB(BISHOP, side); pcbits;) {
        int sq = popFirstBit(pcbits);
        uint64_t atk = bishopAttacksBB(sq, p.occupiedBB & ~p.bishopSlidersBB(side));
        bishopatks[side] |= atk;
        allatks2[side] |= allatks[side] & atk;
        allatks[side] |= atk;
        scr[side] += BishopMob[bitCnt(atk & mobmask)];
        if (atk & kingzone[xside]) katkrs[side] += (1 << 10) + (int(BishopAtk) << 20) + bitCnt(atk & kingzone[xside]);
        if (BitMask[sq] & outpostsqs) scr[side] += OutpostBonus;
        if (bitCnt(atk & centersqs) >= 2) scr[side] += CenterSquares;
        scr[side] -= BishopPawns * bitCnt(p.getPieceBB(PAWN, side) & (BitMask[sq] & WhiteSquaresBB ? WhiteSquaresBB : BlackSquaresBB));
    }
    for (uint64_t pcbits = p.getPieceBB(ROOK, side); pcbits;) {
        int sq = popFirstBit(pcbits);
        uint64_t atk = rookAttacksBB(sq, p.occupiedBB & ~p.rookSlidersBB(side));
        rookatks[side] |= atk;
        allatks2[side] |= allatks[side] & atk;
        allatks[side] |= atk;
        scr[side] += RookMob[bitCnt(atk & mobmask)];
        if (atk & kingzone[xside]) katkrs[side] += (1 << 10) + (int(RookAtk) << 20) + bitCnt(atk & kingzone[xside]);
        if ((BitMask[sq] & Rank7ByColorBB[side]) && (BitMask[p.kpos[xside]] & (Rank7ByColorBB[side] | Rank8ByColorBB[side]))) {
            scr[side] += RookOn7th;
        }
        if (!(FileBB[sqFile(sq)] & p.getPieceBB(PAWN, side))) {
            if (!(FileBB[sqFile(sq)] & p.getPieceBB(PAWN, xside))) scr[side] += RookOnOpenFile;
            else scr[side] += RookOnSemiOpenFile;
        }
    }
    for (uint64_t pcbits = p.getPieceBB(QUEEN, side); pcbits;) {
        int sq = popFirstBit(pcbits);
        uint64_t atk = queenAttacksBB(sq, p.occupiedBB & ~(p.rookSlidersBB(side) | p.bishopSlidersBB(side)));
        queenatks[side] |= atk;
        allatks2[side] |= allatks[side] & atk;
        allatks[side] |= atk;
        scr[side] += QueenMob[bitCnt(atk & mobmask)];
        if (atk & kingzone[xside]) katkrs[side] += (1 << 10) + (int(QueenAtk) << 20) + bitCnt(atk & kingzone[xside]);
    }
}

basic_score_t eval_t::kingshelter(position_t& p, int sq, int side) {
    const int file = FileWing[sqFile(sq)];
    const uint64_t kshelter = p.getPieceBB(PAWN, side);
    const uint64_t kstorm = p.getPieceBB(PAWN, side ^ 1);
    basic_score_t shelter = 0;
    shelter += KingShelter1 * bitCnt(kshelter & KingShelterBB[side][file]);
    shelter += KingShelter1 * bitCnt(kshelter & KingShelterBB[side][file] & FileBB[sqFile(sq)]);
    shelter += KingShelter2 * bitCnt(kshelter & KingShelter2BB[side][file]);
    shelter += KingShelter2 * bitCnt(kshelter & KingShelter2BB[side][file] & FileBB[sqFile(sq)]);
    shelter -= KingStorm1 * bitCnt(kstorm & KingShelter2BB[side][file]);
    shelter -= KingStorm2 * bitCnt(kstorm & KingShelter3BB[side][file]);
    return shelter;
}

void eval_t::kingsafety(position_t& p, int side) {
    const int xside = side ^ 1;
    int katkrscnt = (katkrs[side] & ((1 << 20) - 1)) >> 10;
    if (katkrscnt > (p.getPieceBB(QUEEN, side) ? 0 : 1)) {
        const uint64_t king_atkmask = kingMovesBB(p.kpos[xside]);
        const uint64_t weaksqs = allatks[side] & ~allatks2[xside] & (~allatks[xside] | queenatks[xside] | king_atkmask);
        const uint64_t safesqs = ~p.colorBB[side] & (~allatks[xside] | (weaksqs & allatks2[side]));
        const uint64_t knightthreats = knightMovesBB(p.kpos[xside]);
        const uint64_t bishopthreats = bishopAttacksBB(p.kpos[xside], p.occupiedBB);
        const uint64_t rookthreats = rookAttacksBB(p.kpos[xside], p.occupiedBB);
        const uint64_t queenthreats = bishopthreats | rookthreats;

        basic_score_t shelter = kingshelter(p, p.kpos[xside], xside);
        if (p.canCastleQS(xside)) shelter = std::max(shelter, kingshelter(p, KingSquare[xside][0], xside));
        if (p.canCastleKS(xside)) shelter = std::max(shelter, kingshelter(p, KingSquare[xside][2], xside));

        basic_score_t bonus = katkrs[side] >> 20;
        bonus += KingZoneAttacks * (katkrs[side] & ((1 << 10) - 1));
        bonus += WeakSquares * bitCnt(king_atkmask & weaksqs);
        bonus -= EnemyPawns * bitCnt(p.getPieceBB(PAWN, xside) & king_atkmask & ~weaksqs);
        bonus += QueenSafeCheckValue * bitCnt(queenthreats & queenatks[side] & safesqs);
        bonus += RookSafeCheckValue * bitCnt(rookthreats & rookatks[side] & safesqs);
        bonus += BishopSafeCheckValue * bitCnt(bishopthreats & bishopatks[side] & safesqs);
        bonus += KnightSafeCheckValue * bitCnt(knightthreats & knightatks[side] & safesqs);
        bonus -= shelter;
        if (bonus > 0) scr[side] += score_t(bonus * bonus / 1024, bonus / 20);
    }
}

void eval_t::threats(position_t& p, int side) {
    const int xside = side ^ 1;
    const uint64_t minors = p.getPieceBB(KNIGHT, xside) | p.getPieceBB(BISHOP, xside);
    const uint64_t weak = (allatks[side] & ~allatks[xside]) | (allatks2[side] & ~allatks2[xside] & ~pawnatks[xside]);
    const uint64_t safepush = ~pawnatks[xside] & ~p.occupiedBB;
    const uint64_t pushtarget = pawnAttackBB(p.colorBB[xside] & ~p.piecesBB[PAWN], xside) & (allatks[side] | ~allatks[xside]);
    uint64_t push = shift8BB[side](p.getPieceBB(PAWN, side)) & safepush;
    push |= shift8BB[side](push & Rank3ByColorBB[side]) & safepush;
    scr[side] += PawnPush * bitCnt(push & pushtarget);
    scr[side] += WeakPawns * bitCnt(p.getPieceBB(PAWN, xside) & weak);
    scr[side] += PawnsxMinors * bitCnt(pawnatks[side] & minors);
    scr[side] += MinorsxMinors * bitCnt((knightatks[side] | bishopatks[side]) & minors);
    scr[side] += MajorsxWeakMinors * bitCnt((rookatks[side] | queenatks[side]) & minors & weak);
    scr[side] += PawnsMinorsxMajors * bitCnt((pawnatks[side] | knightatks[side] | bishopatks[side]) & p.rookSlidersBB(xside));
    scr[side] += AllxQueens * bitCnt(allatks[side] & p.getPieceBB(QUEEN, xside));
    scr[side] += KingxMinors * bitCnt(kingMovesBB(p.kpos[side]) & minors & weak);
    scr[side] += KingxRooks * bitCnt(kingMovesBB(p.kpos[side]) & p.getPieceBB(ROOK, xside) & weak);
}

void eval_t::passedpawns(position_t& p, int side) {
    const int xside = side ^ 1;
    uint64_t passers = p.getPieceBB(PAWN, side) & ~fillBBEx[xside](p.piecesBB[PAWN]) & ~pawnfillatks[xside];
    if (!passers) return;
    const uint64_t notblocked = ~shift8BB[xside](p.occupiedBB);
    const uint64_t safepush = ~shift8BB[xside](allatks[xside]);
    const uint64_t safeprom = ~fillBBEx[xside](allatks[xside] | p.colorBB[xside]);
    while (passers) {
        int sq = popFirstBit(passers);
        int rank = getRelativeRank(side, sq);
        scr[side] += PasserDistEnemy[rank] * distance(p.kpos[xside], sq);
        scr[side] -= PasserDistOwn[rank] * distance(p.kpos[side], sq);
        scr[side] += PasserBonus[rank];
        if (BitMask[sq] & notblocked) scr[side] += PasserNotBlocked[rank];
        if (BitMask[sq] & safepush) scr[side] += PasserSafePush[rank];
        if (BitMask[sq] & safeprom) scr[side] += PasserSafeProm[rank];
    }
}

void eval_t::space(position_t& p, int side) {
    const int xside = side ^ 1;
    uint64_t controlled = allatks2[side] & allatks[xside] & ~allatks2[xside] & ~pawnatks[xside];
    scr[side] += PieceSpace * bitCnt(controlled & p.occupiedBB);
    scr[side] += EmptySpace * bitCnt(controlled & ~p.occupiedBB);
}

basic_score_t eval_t::score(position_t& p) {
    int scale = 32;
    for (int color = WHITE; color <= BLACK; ++color) {
        scr[color] = p.stack.score[color];
    }
    if (p.isMatIdxValid()) {
        material_t& mat = getMaterial(p.mat_idx[WHITE], p.mat_idx[BLACK]);
        phase = mat.phase;
        scr[WHITE] += mat.value;
        if (mat.flags & 1) return 0;
        if (mat.flags & 2) scale = 1;
        if (mat.flags & 4 && bitCnt(p.piecesBB[BISHOP] & WhiteSquaresBB) == 1) scale = 16;
    }
    else {
        for (int color = WHITE; color <= BLACK; ++color) {
            material(p, color);
        }
        phase = QueenPhase * bitCnt(p.piecesBB[QUEEN]) + RookPhase * bitCnt(p.piecesBB[ROOK])
            + BishopPhase * bitCnt(p.piecesBB[BISHOP]) + KnightPhase * bitCnt(p.piecesBB[KNIGHT]);
    }
    for (int color = WHITE; color <= BLACK; ++color) {
        katkrs[color] = 0;
        allatks2[color] = knightatks[color] = bishopatks[color] = rookatks[color] = queenatks[color] = 0;
        kingzone[color] = KingZoneBB[color][p.kpos[color]];
        allatks[color] = kingMovesBB(p.kpos[color]);
        pawnatks[color] = pawnAttackBB(p.getPieceBB(PAWN, color), color);
        pawnfillatks[color] = fillBB[color](pawnatks[color]);
        allatks2[color] |= allatks[color] & pawnatks[color];
        allatks[color] |= pawnatks[color];
    }
    for (int color = WHITE; color <= BLACK; ++color) {
        pawnstructure(p, color);
        pieceactivity(p, color);
    }
    for (int color = WHITE; color <= BLACK; ++color) {
        kingsafety(p, color);
        passedpawns(p, color);
        threats(p, color);
        space(p, color);
    }
    static const basic_score_t total_phase = QueenPhase * 2 + RookPhase * 4 + BishopPhase * 4 + KnightPhase * 4;
    score_t score = scr[p.side] - scr[p.side ^ 1];
    basic_score_t tapered = (score.m * phase + (score.e * (total_phase - phase))) / total_phase;
    return ((tapered + Tempo) * scale) / 32;
}