/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "uci.h"

int main(void) {
	uci_t interface;
	interface.info();
	interface.run();

	return 0;
}

// TODO:
//-history moves and other new history ideas
//-LMP
//-king safety eval
//-threats
//-passed pawn eval
//-mobility
//-pawn structure
//-razoring, delta pruning, futility,

//DONE: