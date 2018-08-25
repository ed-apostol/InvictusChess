/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include "typedefs.h"
#include "position.h"

enum MoveGenStages {
	STAGE_EVASION,
	STAGE_HTABLE,
	STAGE_GENTACTICAL,
	STAGE_WINTACTICAL,
	STAGE_KILLER1,
	STAGE_KILLER2,
	STAGE_GENQUIETS,
	STAGE_QUIETS,
	STAGE_BADTACTICAL,
	STAGE_DEFERRED,
	STAGE_DONE
};

struct movepicker_t {
	movepicker_t(position_t& pos, bool skipq, uint16_t hmove = 0, uint16_t k1 = 0, uint16_t k2 = 0);
	move_t getBestMoveFromIdx(int idx);
	bool getMoves(position_t& pos, move_t& move);
	void scoreTactical(position_t& pos);
	void scoreNonTactical(position_t& pos);
	void scoreEvasions(position_t& pos);
	int stage;
	int idx;
	bool inCheck;
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
