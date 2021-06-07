/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <cstring>
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

    score_t MaterialValues[7] = { { 0, 0 },{ 100, 143 },{ 372, 441 },{ 405, 481 },{ 533, 811 },{ 1010, 1609 },{ 0, 0 } };
    score_t BishopPair = { 28, 71 };

    score_t PawnConnected = { 6, 9 };
    score_t PawnDoubled = { 12, 17 };
    score_t PawnIsolated = { 3, 9 };
    score_t PawnBackward = { 0, 10 };
    score_t PawnIsolatedOpen = { 23, 18 };
    score_t PawnBackwardOpen = { 22, 17 };

    score_t PasserDistOwn[8] = { { 0, 0 },{ 4, 0 },{ 5, 3 },{ 1, 12 },{ 2, 22 },{ 8, 31 },{ 13, 26 },{ 0, 0 } };
    score_t PasserDistEnemy[8] = { { 0, 0 },{ 0, 2 },{ 0, 5 },{ 0, 14 },{ 0, 32 },{ 1, 48 },{ 14, 41 },{ 0, 0 } };
    score_t PasserBonus[8] = { { 0, 0 },{ 0, 0 },{ 0, 6 },{ 0, 5 },{ 24, 16 },{ 85, 28 },{ 115, 89 },{ 0, 0 } };
    score_t PasserNotBlocked[8] = { { 0, 0 },{ 0, 8 },{ 0, 1 },{ 0, 20 },{ 8, 29 },{ 20, 67 },{ 75, 114 },{ 0, 0 } };
    score_t PasserSafePush[8] = { { 0, 0 },{ 0, 2 },{ 0, 4 },{ 0, 11 },{ 0, 17 },{ 19, 37 },{ 78, 21 },{ 0, 0 } };
    score_t PasserSafeProm[8] = { { 0, 0 },{ 0, 22 },{ 0, 6 },{ 0, 26 },{ 0, 53 },{ 0, 120 },{ 37, 200 },{ 0, 0 } };

    score_t KnightMob[9] = { { -80, -45 }, { -24, -46 }, { -4, 18 }, { 7, 34 }, { 13, 40 }, { 16, 54 }, { 22, 55 }, { 28, 56 }, { 31, 50 } };
    score_t BishopMob[14] = { { -64, -72 }, { -22, -38 }, { -12, 8 }, { 0, 29 }, { 13, 33 }, { 21, 42 }, { 26, 49 },
                            { 31, 53 }, { 33, 57 }, { 33, 60 }, { 35, 61 }, { 42, 54 }, { 46, 61 }, { 63, 43 } };
    score_t RookMob[15] = { { -101, -139 }, { -48, -80 }, { -13, -14 }, { -3, 6 }, { 4, 20 }, { 6, 33 }, { 6, 41 }, { 9, 41 },
                            { 13, 45 }, { 18, 50 }, { 17, 58 }, { 17, 63 }, { 15, 70 }, { 18, 72 }, { 17, 72 } };
    score_t QueenMob[28] = { { -224, -224 }, { -221, 9 }, { -11, -203 }, { -8, -130 }, { 5, 56 }, { 24, 107 }, { 36, 34 },
                            { 38, 73 }, { 40, 95 }, { 45, 96 }, { 50, 98 }, { 54, 98 }, { 56, 109 }, { 60, 103 },
                            { 60, 112 }, { 60, 122 }, { 57, 132 }, { 61, 131 }, { 58, 128 }, { 54, 138 }, { 58, 130 },
                            { 67, 122 }, { 66, 115 }, { 59, 116 }, { 67, 115 },{ 133, 13 }, { 71, 48 }, { 227, -11 } };
    score_t RookOn7th = { 1, 47 };
    score_t RookOnSemiOpenFile = { 15, 11 };
    score_t RookOnOpenFile = { 40, 10 };
    score_t OutpostBonus = { 17, 12 };
    score_t BishopPawns = { 6, 10 };
    score_t CenterSquares = { 14, 2 };
    basic_score_t Tempo = 37;

    score_t PawnPush = { 15, 13 };
    score_t WeakPawns = { 10, 52 };
    score_t PawnsxMinors = { 65, 27 };
    score_t MinorsxMinors = { 30, 36 };
    score_t MajorsxWeakMinors = { 34, 77 };
    score_t PawnsMinorsxMajors = { 37, 31 };
    score_t AllxQueens = { 29, 20 };
    score_t KingxMinors = { 28, 67 };
    score_t KingxRooks = { 0, 44 };

    basic_score_t KnightAtk = 63;
    basic_score_t BishopAtk = 32;
    basic_score_t RookAtk = 32;
    basic_score_t QueenAtk = 84;
    basic_score_t KingZoneAttacks = 35;
    basic_score_t WeakSquares = 29;
    basic_score_t EnemyPawns = 19;
    basic_score_t QueenSafeCheckValue = 74;
    basic_score_t RookSafeCheckValue = 108;
    basic_score_t BishopSafeCheckValue = 61;
    basic_score_t KnightSafeCheckValue = 123;

    basic_score_t KingShelter1 = 30;
    basic_score_t KingShelter2 = 14;
    basic_score_t KingStorm1 = 100;
    basic_score_t KingStorm2 = 72;

    score_t PieceSpace = { 3, 3 };
    score_t EmptySpace = { 5, 0 };

    basic_score_t KnightPhase = 2;
    basic_score_t BishopPhase = 5;
    basic_score_t RookPhase = 9;
    basic_score_t QueenPhase = 23;

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
                KingZoneBB[color][sq] = kingMovesBB(sq) | BitMask[sq];
                if (Rank1ByColorBB[color] & BitMask[sq]) KingZoneBB[color][sq] |= shift8BB[color](kingMovesBB(sq));
                KingZoneBB[color][sq] |= sqFile(sq) == FileA ? KingZoneBB[color][sq] << 1 : 0;
                KingZoneBB[color][sq] |= sqFile(sq) == FileH ? KingZoneBB[color][sq] >> 1 : 0;
            }
            //PrintOutput() << "sq = " << sq;
            //Utils::printBitBoard(KingZoneBB[WHITE][sq]);
        }
        for (int color = WHITE; color <= BLACK; ++color) {
            for (int castle = 0; castle <= 2; ++castle) {
                int sq = KingSquare[color][castle];
                KingShelterBB[color][castle] = (kingMovesBB(sq) | BitMask[sq]) & Rank2ByColorBB[color];
                KingShelter2BB[color][castle] = shift8BB[color](KingShelterBB[color][castle]);
                KingShelter3BB[color][castle] = shift16BB[color](KingShelterBB[color][castle]);
                //PrintOutput() << "color = " << color;
                //PrintOutput() << "castle = " << castle;
                //Utils::printBitBoard(KingShelter3BB[color][castle]);
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
                            if (!wp && !bp) {
                                if (majors == 0 && wminors < 2 && bminors < 2) mat.flags |= 1; // minor vs minor
                                if (majors == 0 && minors == 2 && (wn == 2 || bn == 2)) mat.flags |= 1; // 2 knights
                                if (majors == 1 && wr == 1 && wminors == 0 && bminors == 1) mat.flags |= 2; // rook vs minor
                                if (majors == 1 && br == 1 && bminors == 0 && wminors == 1) mat.flags |= 2; // rook vs minor
                                if (majors == 2 && wr == 1 && br == 1 && minors < 2) mat.flags |= 2; // rook+minor vs rook
                            }
                            if (majors == 0 && minors == 2 && wb == 1 && bb == 1) mat.flags |= 4; // possible ocb
                            // TODO: material scaling like opposite colored bishops, KRkb, KRkn
                            // TODO: material recognizer (KBPk, KRPkr, insufficient material, etc)
                            // TODO: endgame knowledge (KRPkr, KRkp, KBPkb)
                        }
    }
    material_t& getMaterial(int idx1, int idx2) {
        return MaterialTable[idx1][idx2];
    }
}