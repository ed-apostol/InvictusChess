/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <chrono>
#include <iostream>
#include "typedefs.h"
#include "constants.h"
#include "utils.h"

namespace Utils {
	void printBitBoard(uint64_t b) {
		for (int j = 7; j >= 0; --j) {
			for (int i = 0; i <= 7; ++i) {
				int sq = (j * 8) + i;
				if (BitMask[sq] & b) std::cout << "X";
				else std::cout << ".";
				std::cout << " ";
			}
			std::cout << "\n";
		}
	}

	uint64_t getTime(void) {
		using namespace std::chrono;
		return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	}
}