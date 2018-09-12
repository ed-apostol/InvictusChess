/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <algorithm>
#include "typedefs.h"
#include "constants.h"
#include "position.h"
#include "utils.h"
#include "bitutils.h"
#include "attacks.h"
#include "log.h"
#include "eval.h"

namespace PositionData {
	const uint64_t  CastleSquareMask1[2][2] = { { F1BB | G1BB, B1BB | C1BB | D1BB }, { F8BB | G8BB, B8BB | C8BB | D8BB } };
	const uint64_t  CastleSquareMask2[2][2] = { { E1BB | F1BB | G1BB, C1BB | D1BB | E1BB }, { E8BB | F8BB | G8BB, C8BB | D8BB | E8BB } };
	const int CastleSquareFrom[2] = { E1, E8 };
	const int CastleSquareTo[2][2] = { { G1, C1 },{ G8, C8 } };
	const int RookFrom[2][2] = { { A1, H1 },{ A8, H8 } };
	const int RookTo[2][2] = { { D1, F1 },{ D8, F8 } };

	uint64_t DirBitmap[64][8];
	uint64_t InBetween[64][64];
	int DirFromTo[64][64];
	int CastleMask[64];

	uint64_t ZobPiece[2][8][64];
	uint64_t ZobCastle[16];
	uint64_t ZobEpsq[8];
	uint64_t ZobColor;

	static uint64_t xorshift128plus(void) {
		static uint64_t s[2] = { 4123659995ull, 9981545732273789042ull };
		uint64_t x = s[0];
		uint64_t const y = s[1];
		s[0] = y;
		x ^= x << 23;
		s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
		return s[1] + y;
	}

	void initArr(void) {
		const std::vector<int> kingd = { -9, -1, 7, 8, 9, 1, -7, -8 };

		memset(ZobPiece, 0, sizeof(ZobPiece));
		memset(ZobCastle, 0, sizeof(ZobCastle));
		memset(ZobEpsq, 0, sizeof(ZobEpsq));

		for (int side = 0; side < 2; ++side) {
			for (int pc = 1; pc <= 6; ++pc) {
				for (int sq = 0; sq < 64; ++sq) {
					ZobPiece[side][pc][sq] = xorshift128plus();
				}
			}
		}
		for (int sq = 1; sq < 16; ++sq) ZobCastle[sq] = xorshift128plus();
		for (int sq = 0; sq < 8; ++sq) ZobEpsq[sq] = xorshift128plus();
		ZobColor = xorshift128plus();

		for (int sq = 0; sq < 64; sq++) CastleMask[sq] = 0xF;

		CastleMask[E1] &= ~WCKS;
		CastleMask[H1] &= ~WCKS;
		CastleMask[E1] &= ~WCQS;
		CastleMask[A1] &= ~WCQS;
		CastleMask[E8] &= ~BCKS;
		CastleMask[H8] &= ~BCKS;
		CastleMask[E8] &= ~BCQS;
		CastleMask[A8] &= ~BCQS;

		for (int sq = 0; sq < 64; ++sq) {
			for (int sq2 = 0; sq2 < 64; ++sq2) DirFromTo[sq][sq2] = 8;
			for (int k = 0; k < 8; ++k) {
				DirBitmap[sq][k] = 0;
				for (int p = sq, n = sq + kingd[k]; n >= 0 && n < 64 && abs((n & 7) - (p & 7)) < 2; p = n, n += kingd[k]) {
					DirFromTo[sq][n] = k;
					DirBitmap[sq][k] |= BitMask[n];
				}
			}
		}
		for (int sq = 0; sq < 64; ++sq) {
			for (int sq2 = 0; sq2 < 64; ++sq2) {
				InBetween[sq][sq2] = 0;
				int k = DirFromTo[sq][sq2];
				if (k != 8) {
					int k2 = DirFromTo[sq2][sq];
					InBetween[sq][sq2] = DirBitmap[sq][k] & DirBitmap[sq2][k2];
				}
			}
		}
	}
}

using namespace Attacks;
using namespace BitUtils;
using namespace PositionData;
using namespace EvalPar;

void position_t::initPosition() {
	for (auto& p : piecesBB) p = EmptyBoardBB;
	for (auto& c : colorBB) c = EmptyBoardBB;
	for (auto& h : history) h = 0;
	for (auto& pc : pieces) pc = EMPTY;
	occupiedBB = EmptyBoardBB;
	kpos[0] = kpos[1] = A1;
	side = WHITE;
	num_moves = 0;
	stack.init();
}

void position_t::undoNullMove(undo_t& undo) {
	side ^= 1;
	stack = undo;
}

void position_t::doNullMove(undo_t& undo) {
	undo = stack;

	stack.lastmove.m = 0;
	stack.epsq = -1;
	stack.fifty += 1;
	stack.pliesfromnull = 0;

	if (undo.epsq != -1) stack.hash ^= ZobEpsq[sqFile(undo.epsq)];

	stack.hash ^= ZobColor;
	side ^= 1;
}

void position_t::undoMove(undo_t& undo) {
	move_t m = stack.lastmove;
	int xside = side;
	int from = m.moveFrom();
	int to = m.moveTo();
	int pc = pieces[to];

	side = xside ^ 1;

	pieces[from] = pc;
	piecesBB[pc] ^= BitMask[from] | BitMask[to];
	colorBB[side] ^= BitMask[from] | BitMask[to];

	int cap = stack.capturedpc;
	pieces[to] = cap;
	if (cap != EMPTY) {
		piecesBB[cap] ^= BitMask[to];
		colorBB[xside] ^= BitMask[to];
	}
	if (pc == KING) kpos[side] = from;

	switch (m.moveFlags()) {
	case MF_CASTLE: {
		int rook_from = RookFrom[to / 56][(to % 8) > 5];
		int rook_to = RookTo[to / 56][(to % 8) > 5];
		pieces[rook_from] = ROOK;
		pieces[rook_to] = EMPTY;
		piecesBB[ROOK] ^= BitMask[rook_from] | BitMask[rook_to];
		colorBB[side] ^= BitMask[rook_from] | BitMask[rook_to];
	} break;
	case MF_ENPASSANT: {
		int epsq = (sqRank(from) << 3) + sqFile(to);
		pieces[epsq] = PAWN;
		piecesBB[PAWN] ^= BitMask[epsq];
		colorBB[xside] ^= BitMask[epsq];
	} break;
	case MF_PROMQ: case MF_PROMR: case MF_PROMB: case MF_PROMN: {
		int prom = m.movePromote();
		pieces[from] = PAWN;
		piecesBB[PAWN] ^= BitMask[from];
		piecesBB[prom] ^= BitMask[from];
	} break;
	}
	occupiedBB = colorBB[side] | colorBB[xside];
	stack = undo;
	--num_moves;
}

void position_t::doMove(undo_t& undo, move_t m) {
	int from = m.moveFrom();
	int to = m.moveTo();
	int xside = side ^ 1;

	ASSERT(pieces[to] != KING);

	undo = stack;

	if (undo.epsq != -1) stack.hash ^= ZobEpsq[sqFile(undo.epsq)];
	stack.hash ^= ZobCastle[undo.castle];

	stack.lastmove = m;
	stack.epsq = -1;
	stack.castle &= CastleMask[from] & CastleMask[to];
	stack.hash ^= ZobCastle[stack.castle];
	stack.fifty += 1;
	stack.pliesfromnull += 1;
	stack.capturedpc = pieces[to];
	stack.hash ^= ZobColor;

	int pc = pieces[from];
	pieces[to] = pieces[from];
	pieces[from] = EMPTY;
	piecesBB[pc] ^= BitMask[from] | BitMask[to];
	colorBB[side] ^= BitMask[from] | BitMask[to];
	stack.score[side] += pst[side][pc][to];
	stack.score[side] -= pst[side][pc][from];
	stack.hash ^= ZobPiece[side][pc][from] ^ ZobPiece[side][pc][to];
	if (pc == PAWN) {
		stack.phash ^= ZobPiece[side][pc][from] ^ ZobPiece[side][pc][to];
		stack.fifty = 0;
	}
	if (pc == KING) kpos[side] = to;

	int cap = stack.capturedpc;
	if (cap != EMPTY) {
		piecesBB[cap] ^= BitMask[to];
		colorBB[xside] ^= BitMask[to];
		stack.score[xside] -= mat_values[cap];
		stack.score[xside] -= pst[xside][cap][to];
		stack.fifty = 0;
		stack.hash ^= ZobPiece[xside][cap][to];
		if (cap == PAWN)
			stack.phash ^= ZobPiece[xside][cap][to];
	}
	switch (m.moveFlags()) {
	case MF_PAWN2: {
		stack.epsq = (from + to) / 2;
		if (piecesBB[PAWN] & colorBB[xside] & pawnAttacksBB(stack.epsq, side))
			stack.hash ^= ZobEpsq[sqFile(stack.epsq)];
		else
			stack.epsq = -1;
		stack.fifty = 0;
	} break;
	case MF_CASTLE: {
		int rook_from = RookFrom[to / 56][(to % 8) > 5];
		int rook_to = RookTo[to / 56][(to % 8) > 5];
		pieces[rook_to] = ROOK;
		pieces[rook_from] = EMPTY;
		piecesBB[ROOK] ^= BitMask[rook_from] | BitMask[rook_to];
		colorBB[side] ^= BitMask[rook_from] | BitMask[rook_to];
		stack.score[side] += pst[side][ROOK][rook_to];
		stack.score[side] -= pst[side][ROOK][rook_from];
		stack.hash ^= ZobPiece[side][ROOK][rook_from] ^ ZobPiece[side][ROOK][rook_to];
	} break;
	case MF_ENPASSANT: {
		int epsq = (sqRank(from) << 3) + sqFile(to);
		pieces[epsq] = EMPTY;
		piecesBB[PAWN] ^= BitMask[epsq];
		colorBB[xside] ^= BitMask[epsq];
		stack.score[xside] -= mat_values[PAWN];
		stack.score[xside] -= pst[xside][PAWN][epsq];
		stack.hash ^= ZobPiece[xside][PAWN][epsq];
		stack.phash ^= ZobPiece[xside][PAWN][epsq];
		stack.fifty = 0;
	} break;
	case MF_PROMQ: case MF_PROMR: case MF_PROMB: case MF_PROMN: {
		int prom = m.movePromote();
		piecesBB[PAWN] ^= BitMask[to]; // remove pawn
		stack.hash ^= ZobPiece[side][PAWN][to];
		stack.phash ^= ZobPiece[side][PAWN][to];
		pieces[to] = prom;
		piecesBB[prom] ^= BitMask[to];
		stack.score[side] += mat_values[prom];
		stack.score[side] -= mat_values[PAWN];
		stack.score[side] += pst[side][prom][to];
		stack.score[side] -= pst[side][PAWN][to];
		stack.hash ^= ZobPiece[side][prom][to];
		stack.fifty = 0;
	} break;
	}
	occupiedBB = colorBB[side] | colorBB[xside];
	side = xside;
	history[num_moves++] = stack.hash;

	//ASSERT(hashIsValid());
	//ASSERT(phashIsValid());
}

void position_t::setPiece(int sq, int c, int pc) {
	pieces[sq] = pc;
	piecesBB[pc] |= BitMask[sq];
	colorBB[c] |= BitMask[sq];
	occupiedBB |= BitMask[sq];
	stack.score[c] += mat_values[pc];
	stack.score[c] += pst[c][pc][sq];
	stack.hash ^= ZobPiece[c][pc][sq];
	if (pc == PAWN) stack.phash ^= ZobPiece[c][pc][sq];
	if (pc == KING) kpos[c] = sq;
}

void position_t::setPosition(const std::string& fenStr) {
	static const std::string pc2char("PNBRQKpnbrqk");
	static const std::vector<int> char2piece = { PAWN,KNIGHT,BISHOP,ROOK,QUEEN,KING };
	char col, row, token;
	size_t p;
	int sq = A8;
	std::istringstream ss(fenStr);
	initPosition();
	ss >> std::noskipws;
	while ((ss >> token) && !isspace(token)) {
		if (isdigit(token)) sq += (token - '0');
		else if (token == '/') sq -= 16;
		else if ((p = pc2char.find(token)) != std::string::npos) setPiece(sq++, (islower(token) ? BLACK : WHITE), char2piece[p % 6]);
	}
	ss >> token;
	side = (token == 'w' ? WHITE : BLACK);
	ss >> token;
	while ((ss >> token) && !isspace(token)) {
		if (token == 'K') stack.castle |= WCKS;
		else if (token == 'Q') stack.castle |= WCQS;
		else if (token == 'k') stack.castle |= BCKS;
		else if (token == 'q') stack.castle |= BCQS;
	}
	if (((ss >> col) && (col >= 'a' && col <= 'h')) && ((ss >> row) && (row == '3' || row == '6')))
		stack.epsq = (8 * (row - '1')) + (col - 'a');
	ss >> std::skipws >> stack.fifty;
	if (stack.epsq != -1) stack.hash ^= ZobEpsq[sqFile(stack.epsq)];
	if (side == WHITE) stack.hash ^= ZobColor;
	stack.hash ^= ZobCastle[stack.castle];

	ASSERT(hashIsValid());
	ASSERT(phashIsValid());
}

std::string position_t::positionToFEN() {
	static const std::string pcstr = ".PNBRQK.pnbrqk";
	std::string fen;
	for (int rank = 56; rank >= 0; rank -= 8) {
		int empty = 0;
		for (int file = FileA; file <= FileH; file++) {
			if (getPiece(rank + file) != EMPTY) {
				if (empty > 0) fen += (char)empty + '0';
				fen += pcstr[getPiece(rank + file) + (getSide(rank + file) == WHITE ? 0 : 7)];
				empty = 0;
			}
			else empty++;
		}
		if (empty > 0) fen += (char)empty + '0';
		fen += (rank > Rank1) ? '/' : ' ';
	}
	fen += (side == WHITE) ? 'w' : 'b';
	fen += ' ';
	if (stack.castle == 0) fen += '-';
	if (stack.castle & WCKS) fen += 'K';
	if (stack.castle & WCQS) fen += 'Q';
	if (stack.castle & BCKS) fen += 'k';
	if (stack.castle & BCQS) fen += 'q';
	fen += ' ';
	if (stack.epsq == -1) fen += '-';
	else {
		fen += sqFile(stack.epsq) + 'a';
		fen += '1' + sqRank(stack.epsq);
	}
	fen += " " + std::to_string(stack.fifty) + " " + std::to_string(num_moves + 1);
	return fen;
}

std::string position_t::to_str() {
	std::string str;
	const std::string piecestr = " PNBRQK pnbrqk";
	const std::string board = "\n  |---|---|---|---|---|---|---|---|\n";
	str += positionToFEN() + "\n";
	str += "incheck: " + std::to_string(kingIsInCheck());
	str += " capt: " + std::to_string(stack.capturedpc);
	str += " lastmove: " + stack.lastmove.to_str() + "\n";
	str += board;
	for (int j = 7; j >= 0; --j) {
		str += std::to_string(j + 1);
		str += " | ";
		for (int i = 0; i <= 7; ++i) {
			int sq = (j * 8) + i;
			int pc = pieces[sq] + (getSide(sq) ? 7 : 0);
			str += piecestr[pc];
			str += " | ";
		}
		str += board;
	}
	str += "    a   b   c   d   e   f   g   h \n";
	return str;
}

bool position_t::isRepeat() {
	for (int idx = num_moves - 5; idx >= num_moves - stack.fifty; idx -= 2) {
		if (history[idx] == stack.hash)
			return true;
	}
	return false;
}

bool position_t::isMatDrawn() {
	if (piecesBB[PAWN] | piecesBB[ROOK] | piecesBB[QUEEN]) return false;
	int cnt = bitCnt(colorBB[side]), xcnt = bitCnt(colorBB[side ^ 1]);
	if (cnt == 1 || xcnt == 1) {
		if (abs(cnt - xcnt) <= 1) return true;
		if (abs(cnt - xcnt) == 2 && bitCnt(piecesBB[KNIGHT]) == 2) return true;
	}
	return false;
}

int position_t::getPiece(int sq) {
	return pieces[sq];
}

int position_t::getSide(int sq) {
	if (BitMask[sq] & colorBB[WHITE]) return WHITE;
	return BLACK;
}

uint64_t  position_t::getPieceBB(int pc, int c) {
	return piecesBB[pc] & colorBB[c];
}

uint64_t position_t::getBishopSlidersBB(int c) {
	return (piecesBB[QUEEN] | piecesBB[BISHOP]) & colorBB[c];
}
uint64_t position_t::getRookSlidersBB(int c) {
	return (piecesBB[QUEEN] | piecesBB[ROOK]) & colorBB[c];
}

uint64_t position_t::allAttackersToSquare(int sq, uint64_t occupied) {
	return
		(colorBB[WHITE] & pawnAttacksBB(sq, BLACK) & piecesBB[PAWN]) |
		(colorBB[BLACK] & pawnAttacksBB(sq, WHITE) & piecesBB[PAWN]) |
		(knightMovesBB(sq) & piecesBB[KNIGHT]) |
		(kingMovesBB(sq) & piecesBB[KING]) |
		(bishopAttacksBB(sq, occupied) & (piecesBB[BISHOP] | piecesBB[QUEEN])) |
		(rookAttacksBB(sq, occupied) & (piecesBB[ROOK] | piecesBB[QUEEN]));
}

bool position_t::statExEval(move_t m, int threshold) {
	static const int StatExEvPcVals[] = { 0, 100,  450,  450,  675, 1300, 0 };

	const int from = m.moveFrom();
	const int to = m.moveTo();
	const int flag = m.moveFlags();
	const int prom = m.movePromote();

	int nextpiece = prom ? prom : pieces[from];
	int currval = StatExEvPcVals[pieces[to]] - threshold;
	if (prom) currval += StatExEvPcVals[prom] - StatExEvPcVals[PAWN];
	if (flag == MF_ENPASSANT) currval += StatExEvPcVals[PAWN];
	if (currval < 0) return 0;
	currval -= StatExEvPcVals[nextpiece];
	if (currval >= 0) return 1;

	uint64_t bishops = piecesBB[BISHOP] | piecesBB[QUEEN];
	uint64_t rooks = piecesBB[ROOK] | piecesBB[QUEEN];
	uint64_t occupied = occupiedBB ^ (BitMask[from] | BitMask[to]);
	if (flag == MF_ENPASSANT && stack.epsq != -1) occupied ^= BitMask[stack.epsq];
	uint64_t attackers = allAttackersToSquare(to, occupied) & occupied;
	int color = side ^ 1;

	while (true) {
		uint64_t myAttackers = attackers & colorBB[color];
		if (myAttackers == EmptyBoardBB) break;

		for (nextpiece = PAWN; nextpiece <= QUEEN; nextpiece++)
			if (myAttackers & piecesBB[nextpiece])
				break;

		occupied ^= BitMask[getFirstBit(myAttackers & piecesBB[nextpiece])];
		if (nextpiece == PAWN || nextpiece == BISHOP || nextpiece == QUEEN)
			attackers |= bishopAttacksBB(to, occupied) & bishops;
		if (nextpiece == ROOK || nextpiece == QUEEN)
			attackers |= rookAttacksBB(to, occupied) & rooks;

		attackers &= occupied;
		color = !color;
		currval = -currval - 1 - StatExEvPcVals[nextpiece];
		if (currval >= 0) {
			if (nextpiece == KING && (attackers & colorBB[color]))
				color = !color;
			break;
		}
	}
	return side != color;
}

// TEST UTILS
bool position_t::hashIsValid() {
	uint64_t hash = 0;
	for (int sq = A1; sq <= H8; ++sq) {
		if (getPiece(sq) != EMPTY) hash ^= ZobPiece[getSide(sq)][getPiece(sq)][sq];
	}
	if (stack.epsq != -1) hash ^= ZobEpsq[sqFile(stack.epsq)];
	if (side == WHITE) hash ^= ZobColor;
	hash ^= ZobCastle[stack.castle];
	return stack.hash == hash;
}

bool position_t::phashIsValid() {
	uint64_t hash = 0;
	for (int sq = A1; sq <= H8; ++sq) {
		if (getPiece(sq) == PAWN)
			hash ^= ZobPiece[getSide(sq)][PAWN][sq];
	}
	return stack.phash == hash;
}