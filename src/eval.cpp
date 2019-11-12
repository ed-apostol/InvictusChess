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
	static const int FileWing[8] = { 0, 0, 0, 1, 1, 2, 2, 2 };
	inline int distance(int a, int b) {
		return std::max(abs(sqFile(a) - sqFile(b)), abs(sqRank(a) - sqRank(b)));
	}
	inline score_t fibonacci(score_t min, score_t max, int r) {
		static const int bonus[8] = { 0, 1, 2, 3, 5, 8, 13, 0 };
		return min + ((max * bonus[r]) / bonus[6]);
	}
}

void eval_t::material(position_t & p, int side) {
	scr[side] += MaterialValues[PAWN] * bitCnt(p.pieceByColorBB(PAWN, side));
	scr[side] += MaterialValues[KNIGHT] * bitCnt(p.pieceByColorBB(KNIGHT, side));
	scr[side] += MaterialValues[BISHOP] * bitCnt(p.pieceByColorBB(BISHOP, side));
	scr[side] += MaterialValues[ROOK] * bitCnt(p.pieceByColorBB(ROOK, side));
	scr[side] += MaterialValues[QUEEN] * bitCnt(p.pieceByColorBB(QUEEN, side));
	if (bitCnt(p.pieceByColorBB(BISHOP, side)) == 2) {
		scr[side] += BishopPair;
	}
	// TODO: material scaling like opposite colored bishops, KRkb, KRkn
	// TODO: add material table with precomputed material recognizer (KBPk, KRPkr, insufficient material, etc)
}

void eval_t::pawnstructure(position_t& p, int side) {
	const int xside = side ^ 1;
	const uint64_t pawns = p.pieceByColorBB(PAWN, side);
	const uint64_t xpawns = p.pieceByColorBB(PAWN, xside);
	const uint64_t open = pawns & ~(pawns & fillBB[xside](xpawns));
	const uint64_t connected = pawns & (pawnatks[side] | shift8BB[xside](pawnatks[side]));
	const uint64_t doubled = pawns & fillBBEx[xside](pawns);
	const uint64_t isolated = pawns & ~fillBB[xside](pawnfillatks[side]);
	const uint64_t backward = pawns & shift8BB[xside]((pawnatks[xside] | xpawns) & ~pawnfillatks[side]) & ~isolated;
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
	const uint64_t mobmask = ~p.pieceByColorBB(KING, side) & ~pawnatks[xside] & ~(shift8BB[xside](p.occupiedBB) & p.pieceByColorBB(PAWN, side));
	const uint64_t outpostsqs = OutpostMask[side] & pawnatks[side] & ~pawnfillatks[xside];

	for (uint64_t pcbits = p.pieceByColorBB(KNIGHT, side); pcbits;) {
		int sq = popFirstBit(pcbits);
		uint64_t atk = knightMovesBB(sq);
		knightatks[side] |= atk;
		allatks2[side] |= allatks[side] & atk;
		allatks[side] |= atk;
		scr[side] += KnightMob * bitCnt(atk & mobmask);
		if (atk & kingzone[xside]) {
			katkrscnt[side] += 1;
			kzoneatks[side] += bitCnt(atk & kingzone[xside]);
			atkweights[side] += KnightAtk;
		}
		if (BitMask[sq] & outpostsqs) scr[side] += OutpostBonus * 2;
		else if (atk & outpostsqs & ~p.colorBB[side]) scr[side] += OutpostBonus;
	}
	for (uint64_t pcbits = p.pieceByColorBB(BISHOP, side); pcbits;) {
		int sq = popFirstBit(pcbits);
		uint64_t atk = bishopAttacksBB(sq, p.occupiedBB & ~p.piecesBB[BISHOP]);
		bishopatks[side] |= atk;
		allatks2[side] |= allatks[side] & atk;
		allatks[side] |= atk;
		scr[side] += BishopMob * bitCnt(atk & mobmask);
		if (atk & kingzone[xside]) {
			katkrscnt[side] += 1;
			kzoneatks[side] += bitCnt(atk & kingzone[xside]);
			atkweights[side] += BishopAtk;
		}
		if (BitMask[sq] & outpostsqs) scr[side] += OutpostBonus;
		scr[side] -= BishopPawns * bitCnt(p.pieceByColorBB(PAWN, side) & (BitMask[sq] & WhiteSquaresBB ? WhiteSquaresBB : BlackSquaresBB));
	}
	for (uint64_t pcbits = p.pieceByColorBB(ROOK, side); pcbits;) {
		int sq = popFirstBit(pcbits);
		uint64_t atk = rookAttacksBB(sq, p.occupiedBB & ~p.piecesBB[ROOK]);
		rookatks[side] |= atk;
		allatks2[side] |= allatks[side] & atk;
		allatks[side] |= atk;
		scr[side] += RookMob * bitCnt(atk & mobmask);
		if (atk & kingzone[xside]) {
			katkrscnt[side] += 1;
			kzoneatks[side] += bitCnt(atk & kingzone[xside]);
			atkweights[side] += RookAtk;
		}
		if ((BitMask[sq] & Rank7ByColorBB[side]) && (BitMask[p.kpos[xside]] & (Rank7ByColorBB[side] | Rank8ByColorBB[side]))) {
			scr[side] += RookOn7th;
		}
		if (!(FileBB[sqFile(sq)] & p.pieceByColorBB(PAWN, side))) {
			if (!(FileBB[sqFile(sq)] & p.pieceByColorBB(PAWN, xside))) scr[side] += RookOnOpenFile;
			else scr[side] += RookOnSemiOpenFile;
		}
	}
	for (uint64_t pcbits = p.pieceByColorBB(QUEEN, side); pcbits;) {
		int sq = popFirstBit(pcbits);
		uint64_t atk = queenAttacksBB(sq, p.occupiedBB);
		queenatks[side] |= atk;
		allatks2[side] |= allatks[side] & atk;
		allatks[side] |= atk;
		scr[side] += QueenMob * bitCnt(atk & mobmask);
		if (atk & kingzone[xside]) {
			katkrscnt[side] += 1;
			kzoneatks[side] += bitCnt(atk & kingzone[xside]);
			atkweights[side] += QueenAtk;
		}
	}
}

score_t eval_t::kingshelter(position_t& p, int idx, int side) {
	const int xside = side ^ 1;
	score_t shelter = KingShelter1 * bitCnt(KingShelterBB[xside][idx] & p.pieceByColorBB(PAWN, xside));
	shelter += KingShelter2 * bitCnt(KingShelter2BB[xside][idx] & p.pieceByColorBB(PAWN, xside));
	shelter -= KingStorm1 * bitCnt(KingShelter2BB[xside][idx] & p.pieceByColorBB(PAWN, side));
	shelter -= KingStorm2 * bitCnt(KingShelter3BB[xside][idx] & p.pieceByColorBB(PAWN, side));
	return shelter;
}

void eval_t::kingsafety(position_t& p, int side) {
	const int xside = side ^ 1;

	if (!p.pieceByColorBB(QUEEN, side)) return;
	if (!p.pieceByColorBB(ROOK, side) || !p.pieceByColorBB(BISHOP, side) || !p.pieceByColorBB(KNIGHT, side)) return;

	score_t curr_shelter = kingshelter(p, FileWing[sqFile(p.kpos[xside])], side);
	score_t best_shelter = curr_shelter;
	if (p.stack.castle & (xside ? BCQS : WCQS)) {
		score_t shelter = kingshelter(p, 0, side);
		if (shelter.m > best_shelter.m) best_shelter = shelter;
	}
	if (p.stack.castle & (xside ? BCKS : WCKS)) {
		score_t shelter = kingshelter(p, 2, side);
		if (shelter.m > best_shelter.m) best_shelter = shelter;
	}
	scr[xside] += (best_shelter + curr_shelter) / 2;

	if (katkrscnt[side] >= 2 && kzoneatks[side] >= 1) {
		const uint64_t king_atkmask = kingMovesBB(p.kpos[xside]);
		const uint64_t weaksqs = allatks[side] & ~allatks2[xside] & (~allatks[xside] | queenatks[xside] | king_atkmask);
		const uint64_t safesqs = ~p.colorBB[side] & (~allatks[xside] | (weaksqs & allatks2[side]));
		const uint64_t knightthreats = knightMovesBB(p.kpos[xside]);
		const uint64_t bishopthreats = bishopAttacksBB(p.kpos[xside], p.occupiedBB);
		const uint64_t rookthreats = rookAttacksBB(p.kpos[xside], p.occupiedBB);
		const uint64_t queenthreats = bishopthreats | rookthreats;
		basic_score_t penalty = atkweights[side] * katkrscnt[side];
		penalty += AttackValue * kzoneatks[side];
		penalty += WeakSquares * bitCnt(king_atkmask & weaksqs);
		penalty += NoEnemyQueens * !p.pieceByColorBB(QUEEN, xside);
		penalty -= EnemyPawns * bitCnt(p.pieceByColorBB(PAWN, xside) & king_atkmask & ~weaksqs);
		penalty += QueenSafeCheckValue * bitCnt(queenthreats & queenatks[side] & safesqs);
		penalty += RookSafeCheckValue * bitCnt(rookthreats & rookatks[side] & safesqs);
		penalty += BishopSafeCheckValue * bitCnt(bishopthreats & bishopatks[side] & safesqs);
		penalty += KnightSafeCheckValue * bitCnt(knightthreats & knightatks[side] & safesqs);
		if (penalty > 0) scr[side] += score_t(penalty * penalty / 512, penalty / 20);
	}
}

void eval_t::threats(position_t& p, int side) {
	const int xside = side ^ 1;
	const uint64_t minors = p.pieceByColorBB(KNIGHT, xside) | p.pieceByColorBB(BISHOP, xside);
	const uint64_t weak = (allatks[side] & ~allatks[xside]) | (allatks2[side] & ~allatks2[xside] & ~pawnatks[xside]);
	uint64_t push = shift8BB[side](p.pieceByColorBB(PAWN, side));
	push |= shift8BB[side](push & Rank3ByColorBB[side] & ~pawnatks[xside] & ~p.occupiedBB);
	push &= pawnAttackBB(p.colorBB[xside] & ~p.piecesBB[PAWN], xside) & ~pawnatks[xside] & ~p.occupiedBB & (allatks[side] | ~allatks[xside]);
	scr[side] += PawnPush * bitCnt(push);
	scr[side] += WeakPawns * bitCnt(p.pieceByColorBB(PAWN, xside) & weak);
	scr[side] += PawnsxMinors * bitCnt(pawnatks[side] & minors);
	scr[side] += MinorsxMinors * bitCnt((knightatks[side] | bishopatks[side]) & minors);
	scr[side] += MajorsxWeakMinors * bitCnt((rookatks[side] | queenatks[side]) & minors & weak);
	scr[side] += PawnsMinorsxMajors * bitCnt((pawnatks[side] | knightatks[side] | bishopatks[side]) & (p.pieceByColorBB(ROOK, xside) | p.pieceByColorBB(QUEEN, xside)));
	scr[side] += AllxQueens * bitCnt(allatks[side] & p.pieceByColorBB(QUEEN, xside));
}

void eval_t::passedpawns(position_t& p, int side) {
	const int xside = side ^ 1;
	uint64_t passers = p.pieceByColorBB(PAWN, side) & ~fillBBEx[xside](p.piecesBB[PAWN]) & ~pawnfillatks[xside];
	if (!passers) return;
	const uint64_t notblocked = ~shift8BB[xside](p.occupiedBB);
	const uint64_t safepush = ~shift8BB[xside](allatks[xside]);
	const uint64_t safeprom = ~fillBBEx[xside](allatks[xside] | p.colorBB[xside]);
	while (passers) {
		int sq = popFirstBit(passers);
		score_t local = PasserBonusMax;
		local += PasserDistEnemy * distance(p.kpos[xside], sq);
		local -= PasserDistOwn * distance(p.kpos[side], sq);
		if (BitMask[sq] & notblocked) local += PasserNotBlocked;
		if (BitMask[sq] & safepush) local += PasserSafePush;
		if (BitMask[sq] & safeprom) local += PasserSafeProm;
		scr[side] += fibonacci(PasserBonusMin, local, side == WHITE ? sqRank(sq) : 7 - sqRank(sq));
	}
}

basic_score_t eval_t::score(position_t& p) {
	for (int color = WHITE; color <= BLACK; ++color) {
		scr[color] = p.stack.score[color];
		katkrscnt[color] = kzoneatks[color] = atkweights[color] = 0;
		allatks2[color] = knightatks[color] = bishopatks[color] = rookatks[color] = queenatks[color] = 0;
		kingzone[color] = KingZoneBB[color][p.kpos[color]];
		allatks[color] = kingMovesBB(p.kpos[color]);
		pawnatks[color] = pawnAttackBB(p.pieceByColorBB(PAWN, color), color);
		pawnfillatks[color] = fillBB[color](pawnatks[color]);
		allatks2[color] |= allatks[color] & pawnatks[color];
		allatks[color] |= pawnatks[color];
	}
	for (int color = WHITE; color <= BLACK; ++color) {
		material(p, color);
		pawnstructure(p, color);
		pieceactivity(p, color);
	}
	for (int color = WHITE; color <= BLACK; ++color) {
		kingsafety(p, color);
		passedpawns(p, color);
		threats(p, color);
	}

	int phase = 4 * bitCnt(p.piecesBB[QUEEN]) + 2 * bitCnt(p.piecesBB[ROOK]) + 1 * bitCnt(p.piecesBB[KNIGHT] | p.piecesBB[BISHOP]);
	score_t score = scr[p.side] - scr[p.side ^ 1];
	basic_score_t scaled = (score.m*phase + (score.e*(24 - phase))) / 24;
	return scaled + Tempo;
}