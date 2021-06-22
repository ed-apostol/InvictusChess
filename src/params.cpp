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

    score_t MaterialValues[7] = { {0, 0}, {100, 154}, {376, 412}, {404, 460}, {506, 821}, {1006, 1590}, {0, 0}, };

    score_t ImbalanceInternal[6][6] = {
    { {35, 68}, },
    { {0, -1}, {0, 0}, },
    { {-5, -7}, {0, 9}, {0, -6}, },
    { {-2, 30}, {0, 4}, {6, 7}, {4, -11}, },
    { {1, -14}, {-1, 5}, {0, 14}, {4, 25}, {-4, -2}, },
    { {-7, -26}, {0, -4}, {-15, -5}, {-23, 24}, {-56, 37}, {50, -399}, },
    };

    score_t ImbalanceExternal[6][6] = {
    { },
    { {0, 1}, },
    { {-1, 4}, {0, 5}, },
    { {3, 14}, {0, 6}, {-1, -9}, },
    { {-4, -10}, {-1, 7}, {-4, -11}, {-1, -7}, },
    { {2, -25}, {2, 40}, {9, 81}, {3, 127}, {-102, 194}, },
    };

    score_t KnightMob[9] = { {-82, -78}, {-23, -75}, {-5, -10}, {6, 7}, {13, 14}, {15, 29}, {22, 32}, {28, 34}, {31, 29}, };
    score_t BishopMob[14] = { {-61, -86}, {-20, -47}, {-12, 3}, {0, 24}, {13, 29}, {21, 39}, {25, 47}, {30, 51}, {31, 56}, {32, 59}, {33, 60}, {42, 53}, {47, 61}, {65, 46}, };
    score_t RookMob[15] = { {-99, -149}, {-47, -88}, {-13, -17}, {-2, 5}, {5, 20}, {6, 33}, {6, 42}, {9, 42}, {13, 46}, {17, 52}, {16, 60}, {16, 65}, {14, 71}, {17, 72}, {18, 71}, };
    score_t QueenMob[28] = { {-2271, -735}, {-736, 771}, {24, -527}, {8, -169}, {4, 49}, {25, 96}, {36, 24}, {39, 63}, {41, 86}, {45, 91}, {50, 93}, {54, 94}, {56, 105}, {59, 103}, {59, 113}, {60, 121}, {57, 133}, {60, 135}, {57, 135}, {53, 148}, {58, 140}, {65, 138}, {66, 126}, {58, 134}, {69, 125}, {118, 50}, {77, 49}, {389, -148}, };

    score_t PasserDistOwn[8] = { {0, 0}, {-1, 3}, {2, -5}, {6, -13}, {0, -22}, {-8, -31}, {-13, -27}, {0, 0}, };
    score_t PasserDistEnemy[8] = { {0, 0}, {-3, 3}, {-4, 6}, {-5, 16}, {-1, 33}, {1, 49}, {14, 42}, {0, 0}, };
    score_t PasserBonus[8] = { {0, 0}, {-9, -5}, {-13, 11}, {-6, 2}, {23, 12}, {85, 28}, {121, 92}, {0, 0}, };
    score_t PasserNotBlocked[8] = { {0, 0}, {6, 7}, {2, 1}, {4, 18}, {8, 29}, {19, 67}, {77, 114}, {0, 0}, };
    score_t PasserSafePush[8] = { {0, 0}, {10, -5}, {12, -2}, {0, 10}, {-2, 18}, {22, 36}, {77, 23}, {0, 0}, };
    score_t PasserSafeProm[8] = { {0, 0}, {-42, 25}, {-48, 14}, {-56, 36}, {-75, 67}, {-38, 129}, {-5, 213}, {0, 0}, };

    score_t PawnConnected = { 8, 9 };
    score_t PawnDoubled = { -9, -18 };
    score_t PawnIsolated = { -3, -9 };
    score_t PawnBackward = { 2, -11 };
    score_t PawnIsolatedOpen = { -21, -21 };
    score_t PawnBackwardOpen = { -20, -20 };

    score_t RookOn7th = { -2, 48 };
    score_t RookOnSemiOpenFile = { 14, 10 };
    score_t RookOnOpenFile = { 41, 11 };
    score_t KnightOutpost = { 32, 27 };
    score_t KnightXOutpost = { 17, 14 };
    score_t BishopOutpost = { 39, 1 };
    score_t BishopPawns = { -6, -10 };
    score_t BishopCenterControl = { 16, 1 };
    score_t PieceSpace = { 3, 3 };
    score_t EmptySpace = { 5, 0 };

    score_t PawnPush = { 16, 13 };
    score_t WeakPawns = { 7, 53 };
    score_t PawnsxMinors = { 68, 30 };
    score_t MinorsxMinors = { 29, 40 };
    score_t MajorsxWeakMinors = { 31, 81 };
    score_t PawnsMinorsxMajors = { 39, 29 };
    score_t AllxQueens = { 29, 21 };
    score_t KingxMinors = { 22, 67 };
    score_t KingxRooks = { -28, 54 };

    basic_score_t KnightAtk = 64;
    basic_score_t BishopAtk = 34;
    basic_score_t RookAtk = 29;
    basic_score_t QueenAtk = 78;
    basic_score_t KingZoneAttacks = 36;
    basic_score_t WeakSquares = 32;
    basic_score_t EnemyPawns = -15;
    basic_score_t QueenSafeCheckValue = 75;
    basic_score_t RookSafeCheckValue = 108;
    basic_score_t BishopSafeCheckValue = 54;
    basic_score_t KnightSafeCheckValue = 126;
    basic_score_t KingShelter1 = -35;
    basic_score_t KingShelter2 = -4;
    basic_score_t KingShelterF1 = -57;
    basic_score_t KingShelterF2 = -39;
    basic_score_t KingStorm1 = 122;
    basic_score_t KingStorm2 = 66;

    basic_score_t KnightPhase = 2;
    basic_score_t BishopPhase = 5;
    basic_score_t RookPhase = 9;
    basic_score_t QueenPhase = 24;
    basic_score_t Tempo = 38;

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
        }
        for (int color = WHITE; color <= BLACK; ++color) {
            for (int castle = 0; castle <= 2; ++castle) {
                int sq = KingSquare[color][castle];
                KingShelterBB[color][castle] = (kingMovesBB(sq) | BitMask[sq]) & Rank2ByColorBB[color];
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

    score_t imbalance(const int pieceCount[2][6], int side) {
        int xside = side ^ 1;
        score_t bonus;
        // Second-degree polynomial material imbalance, by Tord Romstad
        for (int pt1 = 0; pt1 <= QUEEN; ++pt1) {
            if (!pieceCount[side][pt1]) continue;
            score_t value = ImbalanceInternal[pt1][pt1] * pieceCount[side][pt1];
            for (int pt2 = 0; pt2 < pt1; ++pt2)
                value += ImbalanceInternal[pt1][pt2] * pieceCount[side][pt2] + ImbalanceExternal[pt1][pt2] * pieceCount[xside][pt2];
            bonus += value * pieceCount[side][pt1];
        }
        return bonus;
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

                            int pieceCount[2][6] = {
                                {wb > 1, wp, wn, wb, wr, wq},
                                {bb > 1, bp, bn, bb, br, bq},
                            };

                            for (int side = WHITE; side <= BLACK; ++side) {
                                score_t scr;
                                scr += MaterialValues[PAWN] * (side == WHITE ? wp : bp);
                                scr += MaterialValues[KNIGHT] * (side == WHITE ? wn : bn);
                                scr += MaterialValues[BISHOP] * (side == WHITE ? wb : bb);
                                scr += MaterialValues[ROOK] * (side == WHITE ? wr : br);
                                scr += MaterialValues[QUEEN] * (side == WHITE ? wq : bq);
                                mat.value += scr * (side == WHITE ? 1 : -1);
                                mat.value += imbalance(pieceCount, side) * (side == WHITE ? 1 : -1);
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