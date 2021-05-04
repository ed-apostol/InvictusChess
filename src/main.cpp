/**************************************************/
/*  Invictus 2021                                 */
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
// history moves and other new history ideas
// pawn hash
// material imbalance
// checks in qsearch at first level