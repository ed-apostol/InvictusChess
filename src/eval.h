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
    void pawnstructure(position_t& p, int side);
    void mobility(position_t& p, int side);
    void kingsafety(position_t& p, int side);
    void threats(position_t& p, int side);
    void passedpawns(position_t& p, int side);
    int score(position_t& p);
    uint64_t pawnatks[2];
    uint64_t knightatks[2];
    uint64_t bishopatks[2];
    uint64_t rookatks[2];
    uint64_t queenatks[2];
    uint64_t allatks[2];
    uint64_t allatks2[2];
    uint64_t kingzone[2];
    uint64_t kingzoneatks[2];
    score_t scr[2];
};
