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
//-board is drawn
//-history moves and other new history ideas
//-LMP
//-king safety eval
//-threats
//-passed pawn eval
//-mobility
//-pawn structure
//-razoring, delta pruning, futility,

//DONE:
//-iterative deepening
//-proper display of pv moves
//-proper display of UCI output and behavior
//-support of timed games
//-aspiration window
//-mate distance pruning
//-investigate king PST values
//-PVS search
//-add way to interrupt input/maybe do threads instead
//-repetition detection
//-hash tables
//-hash move and killers
//-write code to test for valid hash per position
//-move sorting thorugh swap?
//-SEE
//-move ordering of bad captures
//-put back mate pruning
//-rename classes
//-add storing PV at qsearch
//-fix mate in 1 issue
//-do not stop incomplete iteration
//-null move
//-LMR
//-fix mate scores in hash table
//-precalculate PST and material
//-lazy SMP
//-support UCI options
//-IID
//-pondering
//-replace hash numbers by mersenne?
//-replace PST?
//-ponder
//-ABDADA??? -sync at root, sync aspiration window,