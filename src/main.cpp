/**************************************************/
/*  Invictus 2019						          */
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
//-threats
//-passed pawn eval
//-pawn structure
//-razoring, delta pruning, futility,
//-better time management
//-outposts, open files, 7th rank, double bishop,

//DONE:
//-sync iter depth
//-king safety eval
//-mobility