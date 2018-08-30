/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "eval.h"
#include "bitutils.h"
#include "log.h"
#include "constants.h"
#include "attacks.h"

using namespace BitUtils;

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
}

namespace EvalPar {
	static const score_t mat_values[7] = { { 0,0 },{ 100, 125 },{ 460, 390 },{ 470, 420 },{ 640, 720 },{ 1310,1350 },{ 0,0 } };
	static score_t KnightMob = { 6,8 };
	static score_t BishopMob = { 3,3 };
	static score_t RookMob = { 1,2 };
	static score_t QueenMob = { 1,2 };

	score_t pst[2][8][64];

	void initArr() {
		memset(pst, 0, sizeof(pst));
		for (int sq = 0; sq < 64; ++sq) {
			int rsq = ((7 - sqRank(sq)) * 8) + sqFile(sq);
			pst[WHITE][PAWN][sq] = pawnPST(sq);
			pst[WHITE][KNIGHT][sq] = knightPST(sq);
			pst[WHITE][BISHOP][sq] = bishopPST(sq);
			pst[WHITE][ROOK][sq] = rookPST(sq);
			pst[WHITE][QUEEN][sq] = queenPST(sq);
			pst[WHITE][KING][sq] = kingPST(sq);
			pst[BLACK][PAWN][sq] = pawnPST(rsq);
			pst[BLACK][KNIGHT][sq] = knightPST(rsq);
			pst[BLACK][BISHOP][sq] = bishopPST(rsq);
			pst[BLACK][ROOK][sq] = rookPST(rsq);
			pst[BLACK][QUEEN][sq] = queenPST(rsq);
			pst[BLACK][KING][sq] = kingPST(rsq);
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
using namespace Attacks;

score_t eval_t::evalMaterial(position_t& p) {
	score_t s;
	for (uint64_t pcbits = p.colorBB[p.side]; pcbits;) {
		int sq = popFirstBit(pcbits);
		int pc = p.pieces[sq];
		s += mat_values[pc];
		s += pst[p.side][pc][sq];
	}
	for (uint64_t pcbits = p.colorBB[p.side ^ 1]; pcbits;) {
		int sq = popFirstBit(pcbits);
		int pc = p.pieces[sq];
		s -= mat_values[pc];
		s -= pst[p.side ^ 1][pc][sq];
	}
	return s;
}

void eval_t::mobility(position_t& p, score_t& scr, int side) {
	uint64_t mobmask = ~p.colorBB[side] & ~pawnatks[side ^ 1];
	for (uint64_t pcbits = p.getPieceBB(KNIGHT, side); pcbits;) {
		int sq = popFirstBit(pcbits);
		scr += KnightMob * bitCnt(knightMovesBB(sq) & mobmask);
	}
	for (uint64_t pcbits = p.getPieceBB(BISHOP, side); pcbits;) {
		int sq = popFirstBit(pcbits);
		scr += BishopMob * bitCnt(bishopAttacksBB(sq, p.occupiedBB) & mobmask);
	}
	for (uint64_t pcbits = p.getPieceBB(ROOK, side); pcbits;) {
		int sq = popFirstBit(pcbits);
		scr += RookMob * bitCnt(rookAttacksBB(sq, p.occupiedBB) & mobmask);
	}
	for (uint64_t pcbits = p.getPieceBB(QUEEN, side); pcbits;) {
		int sq = popFirstBit(pcbits);
		scr += QueenMob * bitCnt(queenAttacksBB(sq, p.occupiedBB) & mobmask);
	}
}

int eval_t::score(position_t& p) {
	int side = p.side;
	int xside = side ^ 1;

	pawnatks[side] = pawnAttackBB(p.getPieceBB(PAWN, side), side);
	pawnatks[xside] = pawnAttackBB(p.getPieceBB(PAWN, xside), xside);

	score_t scr[2];

	scr[side] = p.stack.score[side];
	scr[xside] = p.stack.score[xside];

	mobility(p, scr[side], side);
	mobility(p, scr[xside], xside);

	int phase = 4 * bitCnt(p.piecesBB[QUEEN]) + 2 * bitCnt(p.piecesBB[ROOK]) + 1 * bitCnt(p.piecesBB[KNIGHT] | p.piecesBB[BISHOP]);
	score_t s = scr[side] - scr[xside];
	return ((s.m*phase) + (s.e*(24 - phase))) / 24;
}