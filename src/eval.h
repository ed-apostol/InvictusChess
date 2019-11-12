/**************************************************/
/*  Invictus 2019                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include "typedefs.h"
#include "position.h"

struct eval_t {
    void material(position_t& p, int side);
    void pawnstructure(position_t& p, int side);
    void pieceactivity(position_t& p, int side);
    score_t kingshelter(position_t& p, int sq, int side);
    void kingsafety(position_t& p, int side);
    void threats(position_t& p, int side);
    void passedpawns(position_t& p, int side);
    basic_score_t score(position_t& p);
    uint64_t pawnatks[2];
    uint64_t knightatks[2];
    uint64_t bishopatks[2];
    uint64_t rookatks[2];
    uint64_t queenatks[2];
    uint64_t allatks[2];
    uint64_t allatks2[2];
    uint64_t kingzone[2];
    uint64_t pawnfillatks[2];
    basic_score_t katkrscnt[2];
    basic_score_t atkweights[2];
    basic_score_t kzoneatks[2];

    score_t scr[2];
};
