/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <chrono>
#include <windows.h>
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

#define GetKnownProcAddress(hmod, F) (decltype(F)*)GetProcAddress(hmod, #F)

	int bestGroup(int index, HMODULE kernel) {
		int groupSize = 0, groups[2048];
		int nodes = 0, cores = 0, threads = 0;
		DWORD returnLength = 0, byteOffset = 0;

		auto fun1 = GetKnownProcAddress(kernel, GetLogicalProcessorInformationEx);
		if (!fun1) return -1;

		if (fun1(RelationAll, NULL, &returnLength)) return -1;

		SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, *ptr;
		ptr = buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(returnLength);
		if (!fun1(RelationAll, buffer, &returnLength)) {
			free(buffer);
			return -1;
		}

		while (ptr->Size > 0 && byteOffset + ptr->Size <= returnLength) {
			if (ptr->Relationship == RelationNumaNode) nodes++;
			else if (ptr->Relationship == RelationProcessorCore) {
				cores++;
				threads += (ptr->Processor.Flags == LTP_PC_SMT) ? 2 : 1;
			}
			byteOffset += ptr->Size;
			ptr = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)(((char*)ptr) + ptr->Size);
		}
		free(buffer);

		for (int n = 0; n < nodes; n++)
			for (int i = 0; i < cores / nodes; i++)
				groups[groupSize++] = n;
		for (int t = 0; t < threads - cores; t++)
			groups[groupSize++] = t % nodes;
		return index < groupSize ? groups[index] : -1;
	}

	void bindThisThread(int index) {
		int group;
		HMODULE kernel = GetModuleHandle("Kernel32.dll");
		if ((group = bestGroup(index, kernel)) == -1) return;

		auto fun2 = GetKnownProcAddress(kernel, GetNumaNodeProcessorMaskEx);
		auto fun3 = GetKnownProcAddress(kernel, SetThreadGroupAffinity);
		if (!fun2 || !fun3) return;

		GROUP_AFFINITY affinity;
		if (fun2(group, &affinity)) fun3(GetCurrentThread(), &affinity, NULL);
	}
}