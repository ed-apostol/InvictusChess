/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "params.h"
#include "attacks.h"
#include "log.h"
#include "constants.h"

using namespace Attacks;

namespace EvalParam {
    score_t pawnPST(int sq) {
        score_t file[8] = { { -3, 3 },{ -1, 1 },{ 0, -1 },{ 3, -3 },{ 3, -3 },{ 0, -1 },{ -1, 1 },{ -3, 3 } };
        score_t rank[8] = { { 0, 0 },{ 0, -20 },{ 0, -10 },{ 1, 0 },{ 1 ,5 },{ 0, 10 },{ 0, 15 },{ 0, 0 } };
        return file[sqFile(sq)] + rank[sqRank(sq)];
    }
    score_t knightPST(int sq) {
        score_t file[8] = { { -15, -7 },{ -8, -3 },{ 8, 3 },{ 15, 7 },{ 15, 7 },{ 8, 3 },{ -8, -3 },{ -15, -7 } };
        score_t rank[8] = { { -30, -10 },{ -14, -4 },{ -2, -1 },{ 8, 3 },{ 16, 7 },{ 22, 12 },{ 11, 3 },{ -11, -10 } };
        return file[sqFile(sq)] + rank[sqRank(sq)];
    }
    score_t bishopPST(int sq) {
        score_t rank[8] = { { -4, -5 },{ 0, -1 },{ 0, 1 },{ 4, 5 },{ 4, 5 },{ 0, 1 },{ 0, -1 },{ -4, -5 } };
        score_t file[8] = { { -4, -5 },{ 0, -1 },{ 0, 1 },{ 4, 5 },{ 4, 5 },{ 0, 1 },{ 0, -1 },{ -4, -5 } };
        return file[sqFile(sq)] + rank[sqRank(sq)];
    }
    score_t rookPST(int sq) {
        score_t file[8] = { { -4, 0 },{ -1, 0 },{ 1, 0 },{ 4, 0 },{ 4, 0 },{ 1, 0 },{ -1, 0 },{ -4, 0 } };
        return file[sqFile(sq)];
    }
    score_t queenPST(int sq) {
        score_t file[8] = { { -3, -3 },{ -1, -1 },{ 1, 1 },{ 3, 3 },{ 3, 3 },{ 1, 1 },{ -1, -1 },{ -3, -3 } };
        score_t rank[8] = { { -4, -5 },{ 0, 0 },{ 1, 1 },{ 2, 3 },{ 2, 3 },{ 1, 1 },{ 1, 0 },{ -3, -3 } };
        return file[sqFile(sq)] + rank[sqRank(sq)];
    }
    score_t kingPST(int sq) {
        score_t file[8] = { { 26, -13 },{ 30, 1 },{ 0, 11 },{ -20, 16 },{ -20, 16 },{ 0, 11 },{ 30, 1 },{ 26, -13 } };
        score_t rank[8] = { {20, -29 },{ -5, -4 },{ -25, 1 },{ -29, 6 },{ -33, 10 },{ -37, 6 },{ -37, 1 },{ -37, -10 } };
        return file[sqFile(sq)] + rank[sqRank(sq)];
    }

    score_t MaterialValues[7] = { { 0, 0 },{ 100, 117 },{ 366, 424 },{ 400, 457 },{ 501, 770 },{ 913, 1592 },{ 0, 0 } };
    score_t BishopPair = { 23, 66 };

    score_t PawnConnected = { 2, 12 };
    score_t PawnDoubled = { 10, 1 };
    score_t PawnIsolated = { 2, 17 };
    score_t PawnBackward = { 0, 14 };
    score_t PawnIsolatedOpen = { 28, 17 };
    score_t PawnBackwardOpen = { 21, 14 };

    score_t PasserBonusMin = { 0, 0 };
    score_t PasserBonusMax = { 34, 105 };
    score_t PasserDistOwn = { 0, 25 };
    score_t PasserDistEnemy = { 0, 48 };
    score_t PasserNotBlocked = { 0, 75 };
    score_t PasserSafePush = { 0, 25 };
    score_t PasserSafeProm = { 0, 166 };

    score_t KnightMob = { 7, 7 };
    score_t BishopMob = { 9, 6 };
    score_t RookMob = { 4, 6 };
    score_t QueenMob = { 3, 5 };
    score_t RookOn7th = { 0, 53 };
    score_t RookOnSemiOpenFile = { 20, 5 };
    score_t RookOnOpenFile = { 38, 12 };
    score_t OutpostBonus = { 15, 15 };
    score_t BishopPawns = { 7, 9 };
    basic_score_t Tempo = 37;

    score_t PawnPush = { 19, 9 };
    score_t WeakPawns = { 2, 57 };
    score_t PawnsxMinors = { 74, 19 };
    score_t MinorsxMinors = { 30, 40 };
    score_t MajorsxWeakMinors = { 30, 78 };
    score_t PawnsMinorsxMajors = { 24, 38 };
    score_t AllxQueens = { 51, 30 };
    score_t KingxMinors = { 29, 57 };
    score_t KingxRooks = { 0, 34 };

    basic_score_t KingShelter1 = 29;
    basic_score_t KingShelter2 = 26;
    basic_score_t KingStorm1 = 69;
    basic_score_t KingStorm2 = 24;

    basic_score_t KnightAtk = 10;
    basic_score_t BishopAtk = 2;
    basic_score_t RookAtk = 12;
    basic_score_t QueenAtk = 3;
    basic_score_t AttackValue = 22;
    basic_score_t WeakSquares = 40;
    basic_score_t NoEnemyQueens = 95;
    basic_score_t EnemyPawns = 21;
    basic_score_t QueenSafeCheckValue = 60;
    basic_score_t RookSafeCheckValue = 117;
    basic_score_t BishopSafeCheckValue = 77;
    basic_score_t KnightSafeCheckValue = 112;

    basic_score_t KnightPhase = 3;
    basic_score_t BishopPhase = 5;
    basic_score_t RookPhase = 9;
    basic_score_t QueenPhase = 17;

    score_t PcSqTab[2][8][64];
    uint64_t KingZoneBB[2][64];
    uint64_t KingShelterBB[2][3];
    uint64_t KingShelter2BB[2][3];
    uint64_t KingShelter3BB[2][3];
    material_t MaterialTable[486][486];

    const int MatMul[8] = { 0,1,9,27,81,243,0,0 };
    const int KingSquare[2][3] = { {B1, E1, G1}, {B8, E8, G8} };

    void initArr() {
        std::function<score_t(int)> pstInit[] = { pawnPST, knightPST,bishopPST,rookPST,queenPST,kingPST };
        memset(PcSqTab, 0, sizeof(PcSqTab));
        for (int pc = PAWN; pc <= KING; ++pc) {
            score_t total;
            for (int sq = 0; sq < 64; ++sq) {
                int rsq = ((7 - sqRank(sq)) * 8) + sqFile(sq);
                PcSqTab[WHITE][pc][sq] = pstInit[pc - 1](sq);
                PcSqTab[BLACK][pc][sq] = pstInit[pc - 1](rsq);
                //total += PcSqTab[WHITE][pc][sq];
            }
            //PrintOutput() << "pc: " << pc << " Mid: " << total.m << " End: " << total.e;
        }
        for (int sq = 0; sq < 64; ++sq) {
            for (int color = WHITE; color <= BLACK; ++color) {
                KingZoneBB[color][sq] = kingMovesBB(sq) | (1ull << sq) | shift8BB[color](kingMovesBB(sq));
                KingZoneBB[color][sq] |= sqFile(sq) == FileA ? KingZoneBB[color][sq] << 1 : 0;
                KingZoneBB[color][sq] |= sqFile(sq) == FileH ? KingZoneBB[color][sq] >> 1 : 0;
            }
        }
        for (int color = WHITE; color <= BLACK; ++color) {
            for (int castle = 0; castle <= 2; ++castle) {
                int sq = KingSquare[color][castle];
                KingShelterBB[color][castle] = (kingMovesBB(sq) | (1ull << sq)) & Rank2ByColorBB[color];
                KingShelter2BB[color][castle] = shift8BB[color](KingShelterBB[color][castle]);
                KingShelter3BB[color][castle] = shift16BB[color](KingShelterBB[color][castle]);
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
                displayPSTbyPC(PcSqTab[c][pc], colstr[c] + " " + pcstr[pc], true);
                LogAndPrintOutput() << "ENDGAME";
                displayPSTbyPC(PcSqTab[c][pc], colstr[c] + " " + pcstr[pc], false);
            }
        }
    }
    void initMaterial() {
        //PrintOutput() << "sizeof(MaterialTable)): " << sizeof(MaterialTable);
        memset(MaterialTable, 0, sizeof(MaterialTable));
        for (int wq = 0; wq <= 1; wq++) for (int bq = 0; bq <= 1; bq++)
            for (int wr = 0; wr <= 2; wr++) for (int br = 0; br <= 2; br++)
                for (int wb = 0; wb <= 2; wb++) for (int bb = 0; bb <= 2; bb++)
                    for (int wn = 0; wn <= 2; wn++) for (int bn = 0; bn <= 2; bn++)
                        for (int wp = 0; wp <= 8; wp++) for (int bp = 0; bp <= 8; bp++) {
                            int idx1 = wp * MatMul[PAWN] + wn * MatMul[KNIGHT] + wb * MatMul[BISHOP] + wr * MatMul[ROOK] + wq * MatMul[QUEEN];
                            int idx2 = bp * MatMul[PAWN] + bn * MatMul[KNIGHT] + bb * MatMul[BISHOP] + br * MatMul[ROOK] + bq * MatMul[QUEEN];

                            material_t &mat = MaterialTable[idx1][idx2];
                            mat.phase = QueenPhase * (wq + bq) + RookPhase * (wr + br) + BishopPhase * (wb + bb) + KnightPhase * (wn + bn);
                            mat.value = { 0, 0 };
                            mat.flags = 0;

                            for (int side = WHITE; side <= BLACK; ++side) {
                                score_t scr;
                                scr += MaterialValues[PAWN] * (side == WHITE ? wp : bp);
                                scr += MaterialValues[KNIGHT] * (side == WHITE ? wn : bn);
                                scr += MaterialValues[BISHOP] * (side == WHITE ? wb : bb);
                                scr += MaterialValues[ROOK] * (side == WHITE ? wr : br);
                                scr += MaterialValues[QUEEN] * (side == WHITE ? wq : bq);
                                if ((side == WHITE ? wb : bb) == 2) scr += BishopPair;
                                mat.value += scr * (side == WHITE ? 1 : -1);
                            }

                            int wminors = wn + wb;
                            int bminors = bn + bb;
                            int wmajors = wr + wq;
                            int bmajors = br + bq;
                            int minors = wminors + bminors;
                            int majors = wmajors + bmajors;

                            if (wp + bp + minors + majors == 0) mat.flags |= 1;
                            else if (!wp && !bp && majors == 0 && wminors < 2 && bminors < 2) mat.flags |= 1;
                            else if (!wp && !bp && majors == 0 && minors == 2 && (wn == 2 || bn == 2)) mat.flags |= 1;
                        }
    }
    material_t& getMaterial(int idx1, int idx2) {
        return MaterialTable[idx1][idx2];
    }
}