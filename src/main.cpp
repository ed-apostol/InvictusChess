/**************************************************/
/*  Invictus 2019                                 */
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
// counter move
// singular move
//-better time management
//-hash qsearch