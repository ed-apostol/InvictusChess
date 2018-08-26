/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include "typedefs.h"
#include "position.h"

enum MoveGenStages {
	STG_EVASION,
	STG_HTABLE,
	STG_GENTACTICS,
	STG_WINTACTICS,
	STG_KILLER1,
	STG_KILLER2,
	STG_GENQUIET,
	STG_QUIET,
	STG_BADTACTICS,
	STG_DEFERRED,
	STG_DONE
};

struct movepicker_t {
	movepicker_t(position_t& pos, bool inCheck, bool skipq, uint16_t hmove = 0, uint16_t k1 = 0, uint16_t k2 = 0);
	move_t getBestMoveFromIdx(int idx);
	bool getMoves(position_t& pos, move_t& move);
	void scoreTactical(position_t& pos);
	void scoreNonTactical(position_t& pos);
	void scoreEvasions(position_t& pos);
	int stage;
	int idx;
	bool skipquiet;
	uint64_t pinned;
	uint16_t hashmove;
	uint16_t killer1;
	uint16_t killer2;
	uint16_t counter;
	movelist_t mvlist;
	movelist_t mvlistbad;
	movelist_t deferred;
};
