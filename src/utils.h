/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once
#include "typedefs.h"
#include "position.h"

namespace Utils {
    extern void printBitBoard(uint64_t n);
    extern uint64_t getTime(void);
    extern void bindThisThread(int index);
}
