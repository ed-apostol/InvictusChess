/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "typedefs.h"
#include "attacks.h"
#include "utils.h"

#include <functional>
#include <vector>
#ifdef USE_PEXT
#include <immintrin.h>
#endif

namespace {
    const uint64_t BMagic[64] =
    {
        0x0048610528020080ULL, 0x00c4100212410004ULL, 0x0004180181002010ULL, 0x0004040188108502ULL,
        0x0012021008003040ULL, 0x0002900420228000ULL, 0x0080808410c00100ULL, 0x000600410c500622ULL,
        0x00c0056084140184ULL, 0x0080608816830050ULL, 0x00a010050200b0c0ULL, 0x0000510400800181ULL,
        0x0000431040064009ULL, 0x0000008820890a06ULL, 0x0050028488184008ULL, 0x00214a0104068200ULL,
        0x004090100c080081ULL, 0x000a002014012604ULL, 0x0020402409002200ULL, 0x008400c240128100ULL,
        0x0001000820084200ULL, 0x0024c02201101144ULL, 0x002401008088a800ULL, 0x0003001045009000ULL,
        0x0084200040981549ULL, 0x0001188120080100ULL, 0x0048050048044300ULL, 0x0008080000820012ULL,
        0x0001001181004003ULL, 0x0090038000445000ULL, 0x0010820800a21000ULL, 0x0044010108210110ULL,
        0x0090241008204e30ULL, 0x000c04204004c305ULL, 0x0080804303300400ULL, 0x00a0020080080080ULL,
        0x0000408020220200ULL, 0x0000c08200010100ULL, 0x0010008102022104ULL, 0x0008148118008140ULL,
        0x0008080414809028ULL, 0x0005031010004318ULL, 0x0000603048001008ULL, 0x0008012018000100ULL,
        0x0000202028802901ULL, 0x004011004b049180ULL, 0x0022240b42081400ULL, 0x00c4840c00400020ULL,
        0x0084009219204000ULL, 0x000080c802104000ULL, 0x0002602201100282ULL, 0x0002040821880020ULL,
        0x0002014008320080ULL, 0x0002082078208004ULL, 0x0009094800840082ULL, 0x0020080200b1a010ULL,
        0x0003440407051000ULL, 0x000000220e100440ULL, 0x00480220a4041204ULL, 0x00c1800011084800ULL,
        0x000008021020a200ULL, 0x0000414128092100ULL, 0x0000042002024200ULL, 0x0002081204004200ULL
    };

    const uint64_t RMagic[64] =
    {
        0x00800011400080a6ULL, 0x004000100120004eULL, 0x0080100008600082ULL, 0x0080080016500080ULL,
        0x0080040008000280ULL, 0x0080020005040080ULL, 0x0080108046000100ULL, 0x0080010000204080ULL,
        0x0010800424400082ULL, 0x00004002c8201000ULL, 0x000c802000100080ULL, 0x00810010002100b8ULL,
        0x00ca808014000800ULL, 0x0002002884900200ULL, 0x0042002148041200ULL, 0x00010000c200a100ULL,
        0x00008580004002a0ULL, 0x0020004001403008ULL, 0x0000820020411600ULL, 0x0002120021401a00ULL,
        0x0024808044010800ULL, 0x0022008100040080ULL, 0x00004400094a8810ULL, 0x0000020002814c21ULL,
        0x0011400280082080ULL, 0x004a050e002080c0ULL, 0x00101103002002c0ULL, 0x0025020900201000ULL,
        0x0001001100042800ULL, 0x0002008080022400ULL, 0x000830440021081aULL, 0x0080004200010084ULL,
        0x00008000c9002104ULL, 0x0090400081002900ULL, 0x0080220082004010ULL, 0x0001100101000820ULL,
        0x0000080011001500ULL, 0x0010020080800400ULL, 0x0034010224009048ULL, 0x0002208412000841ULL,
        0x000040008020800cULL, 0x001000c460094000ULL, 0x0020006101330040ULL, 0x0000a30010010028ULL,
        0x0004080004008080ULL, 0x0024000201004040ULL, 0x0000300802440041ULL, 0x00120400c08a0011ULL,
        0x0080006085004100ULL, 0x0028600040100040ULL, 0x00a0082110018080ULL, 0x0010184200221200ULL,
        0x0040080005001100ULL, 0x0004200440104801ULL, 0x0080800900220080ULL, 0x000a01140081c200ULL,
        0x0080044180110021ULL, 0x0008804001001225ULL, 0x00a00c4020010011ULL, 0x00001000a0050009ULL,
        0x0011001800021025ULL, 0x00c9000400620811ULL, 0x0032009001080224ULL, 0x001400810044086aULL
    };

    uint64_t KnightMoves[64];
    uint64_t KingMoves[64];
    uint64_t PawnCaps[2][64];
    uint64_t PawnMoves[2][64];
    uint64_t PawnMoves2[2][64];
    uint64_t RMagicAttacks[0x19000];
    uint64_t BMagicAttacks[0x1480];

    struct Magic {
        uint64_t* offset;
        uint64_t mask;
        uint64_t magic;
        uint64_t shift;
    };
    Magic RookMagic[64];
    Magic BishopMagic[64];

    uint64_t slideAttacks(int sq, uint64_t occ, const std::vector<int>& D) {
        uint64_t att = 0;
        for (int d : D) {
            for (int p = sq, n = sq + d; n >= 0 && n < 64 && abs((n & 7) - (p & 7)) < 2; p = n, n += d) {
                att |= BitMask[n];
                if (BitMask[n] & occ) break;
            }
        }
        return att;
    }

    size_t sliderIndex(uint64_t occ, Magic& m) {
#ifdef USE_PEXT
        return _pext_u64(occ, m.mask);
#else
        return ((occ & m.mask) * m.magic) >> m.shift;
#endif
    }

    void initSliderTable(uint64_t atktable[], const uint64_t magics[], Magic mtable[], const std::vector<int>& dir) {
        mtable[0].offset = atktable;
        for (int s = 0; s < 0x40; s++) {
            Magic& m = mtable[s];
            m.magic = magics[s];
            m.mask = slideAttacks(s, 0, dir) & ~(((Rank1BB | Rank8BB) & ~RankBB[sqRank(s)]) | ((FileABB | FileHBB) & ~FileBB[sqFile(s)]));
            m.shift = 64 - Utils::bitCnt(m.mask);
            if (s < 63) mtable[s + 1].offset = m.offset + (1ull << Utils::bitCnt(m.mask));
            uint64_t occ = 0;
            do {
                m.offset[sliderIndex(occ, m)] = slideAttacks(s, occ, dir);
                occ = (occ - m.mask) & m.mask;
            } while (occ);
        }
    }

    void initMovesTable(const std::vector<int>& D, uint64_t A[]) {
        for (int i = 0; i < 0x40; i++) {
            for (int j : D) {
                int n = i + j;
                if (n >= 0 && n < 64 && abs((n & 7) - (i & 7)) <= 2)
                    A[i] |= BitMask[n];
            }
        }
    }
    uint64_t shiftLeft(uint64_t b, int i) {
        return (b << i);
    }
    uint64_t shiftRight(uint64_t b, int i) {
        return (b >> i);
    }
    uint64_t shiftLeft8(uint64_t b) {
        return (b << 8);
    }
    uint64_t shiftRight8(uint64_t b) {
        return (b >> 8);
    }
    uint64_t shiftLeft16(uint64_t b) {
        return (b << 16);
    }
    uint64_t shiftRight16(uint64_t b) {
        return (b >> 16);
    }
    uint64_t fillUp(uint64_t b) {
        return b |= b << 8, b |= b << 16, b |= b << 32;
    }
    uint64_t fillDown(uint64_t b) {
        return b |= b >> 8, b |= b >> 16, b |= b >> 32;
    }
    uint64_t fillUpEx(uint64_t b) {
        return b |= b << 8, b |= b << 16, (b |= b << 32) << 8;
    }
    uint64_t fillDownEx(uint64_t b) {
        return b |= b >> 8, b |= b >> 16, (b |= b >> 32) >> 8;
    }
}

namespace Attacks {
    void initArr(void) {
        initMovesTable({ -17, -10, 6, 15, 17, 10, -6, -15 }, KnightMoves);
        initMovesTable({ -9, -1, 7, 8, 9, 1, -7, -8 }, KingMoves);
        initMovesTable({ 8 }, PawnMoves[WHITE]);
        initMovesTable({ -8 }, PawnMoves[BLACK]);
        initMovesTable({ 7, 9 }, PawnCaps[WHITE]);
        initMovesTable({ -7, -9 }, PawnCaps[BLACK]);
        initMovesTable({ 16 }, PawnMoves2[WHITE]);
        initMovesTable({ -16 }, PawnMoves2[BLACK]);

        initSliderTable(RMagicAttacks, RMagic, RookMagic, { -1, 8, 1, -8 });
        initSliderTable(BMagicAttacks, BMagic, BishopMagic, { -9, 7, 9, -7 });
    }

    uint64_t pawnMovesBB(int from, uint64_t s) {
        return PawnMoves[s][from];
    }
    uint64_t pawnMoves2BB(int from, uint64_t s) {
        return PawnMoves2[s][from];
    }
    uint64_t pawnAttacksBB(int from, uint64_t s) {
        return PawnCaps[s][from];
    }
    uint64_t knightAttacksBB(int from, uint64_t occ) {
        return KnightMoves[from];
    }
    uint64_t bishopAttacksBB(int from, uint64_t occ) {
        return BishopMagic[from].offset[sliderIndex(occ, BishopMagic[from])];
    }
    uint64_t rookAttacksBB(int from, uint64_t occ) {
        return RookMagic[from].offset[sliderIndex(occ, RookMagic[from])];
    }
    uint64_t queenAttacksBB(int from, uint64_t occ) {
        return bishopAttacksBB(from, occ) | rookAttacksBB(from, occ);
    }
    uint64_t bishopAttacksBBX(int from, uint64_t occ) {
        return bishopAttacksBB(from, occ & ~(bishopAttacksBB(from, occ) & occ));
    }
    uint64_t rookAttacksBBX(int from, uint64_t occ) {
        return rookAttacksBB(from, occ & ~(rookAttacksBB(from, occ) & occ));
    }
    uint64_t kingAttacksBB(int from, uint64_t occ) {
        return KingMoves[from];
    }
    uint64_t knightMovesBB(int from) {
        return KnightMoves[from];
    }
    uint64_t kingMovesBB(int from) {
        return KingMoves[from];
    }
    std::function<uint64_t(uint64_t, int i)> shiftBB[2] = { shiftLeft, shiftRight };
    std::function<uint64_t(uint64_t)> shift8BB[2] = { shiftLeft8, shiftRight8 };
    std::function<uint64_t(uint64_t)> shift16BB[2] = { shiftLeft16, shiftRight16 };
    std::function<uint64_t(uint64_t)> fillBB[2] = { fillUp, fillDown };
    std::function<uint64_t(uint64_t)> fillBBEx[2] = { fillUpEx, fillDownEx };

    uint64_t pawnAttackBB(uint64_t pawns, size_t color) {
        const int Shift[] = { 9, 7 };
        uint64_t pawnAttackLeft = shiftBB[color](pawns, Shift[color ^ 1]) & ~FileHBB;
        uint64_t pawnAttackright = shiftBB[color](pawns, Shift[color]) & ~FileABB;
        return (pawnAttackLeft | pawnAttackright);
    }
}