/**************************************************/
/*  Invictus 2019                                 */
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
        score_t file[8] = { { -4, 5 },{ -2, 3 },{ 0, 1 },{ 2, -1 },{ 2, -1 },{ 0, 1 },{ -2, 3 },{ -4, 5 } };
        score_t rank[8] = { { 0, 0 },{ -1, -10 },{ 0, -5 },{ 1, 0 },{ 1 ,5 },{ 0, 10 },{ 0, 15 },{ 0, 0 } };
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

    score_t MaterialValues[7] = { { 0, 0 },{ 100, 134 },{ 430, 431 },{ 469, 454 },{ 589, 789 },{ 1356, 1449 },{ 0, 0 } };

    score_t PawnConnected = { 6, 8 };
    score_t PawnDoubled = { 7, 16 };
    score_t PawnIsolated = { 19, 10 };
    score_t PawnBackward = { 1, 4 };

    score_t PasserBonusMin = { 0, 24 };
    score_t PasserBonusMax = { 131, 45 };
    score_t PasserDistOwn = { 15, 26 };
    score_t PasserDistEnemy = { 5, 72 };
    score_t PasserNotBlocked = { 64, 0 };
    score_t PasserSafePush = { 40, 0 };

    score_t KnightMob = { 7, 11 };
    score_t BishopMob = { 9, 8 };
    score_t RookMob = { 6, 8 };
    score_t QueenMob = { 1, 11 };

    score_t NumKZoneAttacks = { 5, 13 };
    score_t ShelterBonus = { 10, 11 };

    score_t PawnsxMinors = { 57, 0 };
    score_t MinorsxMinors = { 24, 38 };
    score_t MajorsxWeakMinors = { 9, 52 };
    score_t PawnsMinorsxMajors = { 8, 0 };
    score_t AllxQueens = { 39, 25 };

    int16_t KnightAtk = 3;
    int16_t BishopAtk = 0;
    int16_t RookAtk = 4;
    int16_t QueenAtk = 1;

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