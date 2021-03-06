/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <chrono>
#include <iostream>
#include "typedefs.h"
#include "utils.h"

namespace Utils {
#ifdef _MSC_VER

#include <intrin.h>

#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)

    int getFirstBit(uint64_t bb) {
        unsigned long index = 0;
        _BitScanForward64(&index, bb);
        return index;
    }

    int popFirstBit(uint64_t& b) {
        unsigned long index = 0;
        _BitScanForward64(&index, b);
        b &= (b - 1);
        return index;
    }

#ifdef NOPOPCNT
    int bitCnt(uint64_t x) {
        x -= (x >> 1) & 0x5555555555555555ULL;
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
        return (x * 0x0101010101010101ULL) >> 56;
    }
#else
    int bitCnt(uint64_t x) {
        return __popcnt64(x);
    }
#endif
#else
    int getFirstBit(uint64_t bb) {
        return __builtin_ctzll(bb);
    }

    int popFirstBit(uint64_t& b) {
        int index = __builtin_ctzll(b);
        b &= (b - 1);
        return index;
    }

    int bitCnt(uint64_t x) {
        return __builtin_popcountll(x);
    }
#endif

    std::string printBitBoard(uint64_t b) {
        std::string str;
        for (int j = 7; j >= 0; --j) {
            str += '1' + j;
            str += " ";
            for (int i = 0; i <= 7; ++i) {
                int sq = (j * 8) + i;
                if (BitMask[sq] & b) str += "1";
                else str += ".";
                str += " ";
            }
            str += "\n";
        }
        str += "  a b c d e f g h\n";
        return str;
    }

    uint64_t getTime(void) {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }
#ifndef _WIN32
    void bindThisThread(int index) { (void)index; };
#else

#include <windows.h>

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
        HMODULE kernel = GetModuleHandle("kernel32.dll");
        if ((group = bestGroup(index, kernel)) == -1) return;

        auto getNumaProcMask = GetKnownProcAddress(kernel, GetNumaNodeProcessorMaskEx);
        auto setThreadAffinity = GetKnownProcAddress(kernel, SetThreadGroupAffinity);
        if (!getNumaProcMask || !setThreadAffinity) return;

        GROUP_AFFINITY affinity;
        if (getNumaProcMask(group, &affinity)) setThreadAffinity(GetCurrentThread(), &affinity, NULL);
    }
#endif
}