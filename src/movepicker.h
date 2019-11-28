/**************************************************/
/*  Invictus 2019                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include "typedefs.h"
#include "position.h"
#include "search.h"

enum MoveGenStages {
    STG_EVASION,
    STG_HTABLE,
    STG_GENTACTICS,
    STG_WINTACTICS,
    STG_KILLER1,
    STG_KILLER2,
    STG_COUNTER,
    STG_GENQUIET,
    STG_QUIET,
    STG_BADTACTICS,
    STG_DEFERRED,
    STG_DONE
};

struct movepicker_t {
    movepicker_t(search_t& search, bool inCheck, bool skipq, int m, uint16_t hmove = 0, uint16_t k1 = 0, uint16_t k2 = 0, uint16_t cm = 0);
    move_t getBestMoveFromIdx(int idx);
    bool getMoves(move_t& move, bool skipquiets = false);
    void scoreTactical();
    void scoreNonTactical();
    void scoreEvasions();
    int stage;
    int idx;
    int margin;
    bool inQSearch;
    uint64_t pinned;
    uint16_t hashmove;
    uint16_t killer1;
    uint16_t killer2;
    uint16_t counter;
    search_t& s;
    position_t& pos;
    movelist_t<256> mvlist;
    movelist_t<32> mvlistbad;
    movelist_t<128> deferred;
};
