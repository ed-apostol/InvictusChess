/**************************************************/
/*  Invictus 2019						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include "typedefs.h"
#include "position.h"

namespace EvalPar {
    extern const score_t mat_values[7];
    extern score_t pst[2][8][64];
    extern void initArr();
    extern void displayPST();
}

struct eval_t {
    void mobility(position_t& p, score_t& scr, int side);
    void kingsafety(position_t& p, score_t& scr, int side);
    int score(position_t& p);
    uint64_t pawnatks[2];
    uint64_t knightatks[2];
    uint64_t bishopatks[2];
    uint64_t rookatks[2];
    uint64_t queenatks[2];
    uint64_t kingzone[2];
    uint64_t kingzoneatks[2];
};
