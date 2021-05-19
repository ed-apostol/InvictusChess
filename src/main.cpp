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
// counter moves history
// capture moves history
// follow up move history
// Syzygy
// NNUE