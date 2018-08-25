/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include "typedefs.h"
#include "position.h"

namespace EvalPar {
	extern const int phasevals[7];
	extern const score_t mat_values[7];
	extern score_t pst[2][8][64];
	extern void initArr();
	extern void displayPST();
}

struct eval_t {
	score_t evalMaterial(position_t& p);
	int score(position_t& p);
	int phase;
};
