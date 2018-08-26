/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <algorithm>
#include "movepicker.h"
#include "log.h"

movepicker_t::movepicker_t(position_t& pos, bool inCheck, bool skipq, uint16_t hmove, uint16_t k1, uint16_t k2) {
	idx = 0;
	hashmove = hmove;
	killer1 = k1;
	killer2 = k2;
	pinned = pos.pinnedPieces(pos.side);
	skipquiet = skipq;
	if (inCheck) {
		pos.genCheckEvasions(mvlist);
		scoreEvasions(pos);
		stage = STG_EVASION;
	}
	else if (skipquiet) stage = STG_GENTACTICS;
	else stage = STG_HTABLE;
}

move_t movepicker_t::getBestMoveFromIdx(int idx) {
	int best_idx = idx;
	for (int i = idx + 1; i < mvlist.size; ++i) {
		if (mvlist.mv(i).s > mvlist.mv(best_idx).s)
			best_idx = i;
	}
	if (best_idx != idx)
		std::swap(mvlist.mv(best_idx), mvlist.mv(idx));
	return mvlist.mv(idx);
}

bool movepicker_t::getMoves(position_t& pos, move_t& move) {
	switch (stage) {
	case STG_EVASION:
		if (idx < mvlist.size) {
			move = getBestMoveFromIdx(idx++);
			return true;
		}
		else return false;
		break;
	case STG_HTABLE:
		++stage;
		if (!skipquiet && hashmove != 0) {
			move.m = hashmove;
			if (pos.moveIsValid(move, pinned) && pos.moveIsLegal(move, pinned, false))
				return true;
		}
	case STG_GENTACTICS:
		pos.genTacticalMoves(mvlist);
		scoreTactical(pos);
		++stage;
	case STG_WINTACTICS:
		while (idx < mvlist.size) {
			move = getBestMoveFromIdx(idx++);
			if (move.m == hashmove) continue;
			if (!pos.statExEval(move, 1)) {
				if (!skipquiet)
					mvlistbad.add(move);
				continue;
			}
			if (pos.moveIsLegal(move, pinned, false))
				return true;
		}
		if (skipquiet) return false;
		++stage;
	case STG_KILLER1:
		++stage;
		if (killer1 != hashmove && killer1 != 0) {
			move.m = killer1;
			if (pos.moveIsValid(move, pinned) && pos.moveIsLegal(move, pinned, false))
				return true;
		}
	case STG_KILLER2:
		++stage;
		if (killer2 != hashmove && killer2 != 0) {
			move.m = killer2;
			if (pos.moveIsValid(move, pinned) && pos.moveIsLegal(move, pinned, false))
				return true;
		}
	case STG_GENQUIET:
		pos.genQuietMoves(mvlist);
		scoreNonTactical(pos);
		++stage;
	case STG_QUIET:
		while (idx < mvlist.size) { // order when history scoring is done
			move = mvlist.mv(idx++);
			if (move.m == hashmove) continue;
			if (move.m == killer1) continue;
			if (move.m == killer2) continue;
			if (pos.moveIsLegal(move, pinned, false))
				return true;
		}
		++stage;
		idx = 0;
	case STG_BADTACTICS:
		while (idx < mvlistbad.size) { // no need to order, ordered already
			move = mvlistbad.mv(idx++);
			if (move.m == hashmove) continue;
			if (pos.moveIsLegal(move, pinned, false))
				return true;
		}
		++stage;
		idx = 0;
		//if (deferred.size) PrintOutput() << "deferred size: " << deferred.size;
	case STG_DEFERRED:
		while (idx < deferred.size) {
			move = deferred.mv(idx++);
			return true;
		}
		++stage;
		break;
	case STG_DONE:
		return false;
	}
	return false;
}

void movepicker_t::scoreTactical(position_t& pos) {
	for (int x = 0; x < mvlist.size; ++x) {
		mvlist.mv(x).s = (pos.pieces[mvlist.mv(x).moveTo()] * 6) + mvlist.mv(x).movePromote() - pos.pieces[mvlist.mv(x).moveFrom()];
	}
}

void movepicker_t::scoreNonTactical(position_t& pos) {
}

void movepicker_t::scoreEvasions(position_t& pos) {
	for (int x = 0; x < mvlist.size; ++x) {
		if (mvlist.mv(x).m == hashmove) mvlist.mv(x).s = 10000;
		else if (mvlist.mv(x).m == killer1) mvlist.mv(x).s = 5000;
		else if (mvlist.mv(x).m == killer2) mvlist.mv(x).s = 4999;
		else mvlist.mv(x).s = 5000 + (pos.pieces[mvlist.mv(x).moveTo()] * 6) + mvlist.mv(x).movePromote() - pos.pieces[mvlist.mv(x).moveFrom()];
	}
}