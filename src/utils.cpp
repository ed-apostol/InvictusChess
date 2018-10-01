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
        std::vector<int> groups;
        int nodes = 0, cores = 0, threads = 0;
        DWORD returnLength = 0, byteOffset = 0;

        auto getLogicalProcInfo = GetKnownProcAddress(kernel, GetLogicalProcessorInformationEx);
        if (!getLogicalProcInfo) return -1;

        if (getLogicalProcInfo(RelationAll, NULL, &returnLength)) return -1;

        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, *ptr;
        ptr = buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(returnLength);
        if (!getLogicalProcInfo(RelationAll, buffer, &returnLength)) {
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
                groups.push_back(n);
        for (int t = 0; t < threads - cores; t++)
            groups.push_back(t % nodes);
        return index < groups.size() ? groups[index] : -1;
    }

    void bindThisThread(int index) {
        int group;
        HMODULE kernel = GetModuleHandle("Kernel32.dll");
        if ((group = bestGroup(index, kernel)) == -1) return;

        auto getNumaProcMask = GetKnownProcAddress(kernel, GetNumaNodeProcessorMaskEx);
        auto setThreadAffinity = GetKnownProcAddress(kernel, SetThreadGroupAffinity);
        if (!getNumaProcMask || !setThreadAffinity) return;

        GROUP_AFFINITY affinity;
        if (getNumaProcMask(group, &affinity)) setThreadAffinity(GetCurrentThread(), &affinity, NULL);
    }
}