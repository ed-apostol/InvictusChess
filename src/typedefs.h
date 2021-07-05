/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include <climits>
#include <cstdint>
#include <vector>
#include <array>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <string>

//#define TUNE
//#define DEBUG
//#define NOPOPCNT
#define USE_PEXT

#ifdef DEBUG
#define ASSERT(a) if (!(a)) \
{LogAndPrintInfo() << "file " << __FILE__ << ", line " << __LINE__  << " : " << "assertion \"" #a "\" failed";}
#else
#define ASSERT(a)
#endif

enum ColorEnum {
    WHITE,
    BLACK
};

enum PieceTypes {
    EMPTY,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
};

enum Files {
    FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH
};

enum Ranks {
    Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8
};

enum CastlingRights {
    WCKS = 1,
    WCQS = 2,
    BCKS = 4,
    BCQS = 8
};

const std::string StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

const int MAXPLY = 127;
const int MAXPLYSIZE = 128;
const int MATE = 32750;
const int NOVALUE = -32751;

enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

constexpr int sqRank(int sq) {
    return sq >> 3;
}

constexpr int  sqFile(int sq) {
    return sq & 7;
}

enum MoveFlags {
    MF_NORMAL, MF_CASTLE, MF_PAWN2, MF_ENPASSANT, MF_PROMN, MF_PROMB, MF_PROMR, MF_PROMQ
};

struct move_t {
    move_t() { }
    move_t(int _m) : m(_m) { }
    move_t(int f, int t, int fl) {
        m = f | (t << 6) | (fl << 12);
    }
    constexpr int from() const { return 63 & m; }
    constexpr int to() const { return 63 & (m >> 6); }
    constexpr int promoted() const { return isPromotion() ? flags() - 2 : 0; }
    constexpr int flags() const { return m >> 12; }
    constexpr bool isCastle() const { return MF_CASTLE == flags(); }
    constexpr bool isPawn2Forward() const { return MF_PAWN2 == flags(); }
    constexpr bool isPromotion() const { return (m >> 14); }
    constexpr bool isEnPassant() const { return MF_ENPASSANT == flags(); }
    constexpr bool isSpecial() const { return flags() ? !isPawn2Forward() : 0; }
    constexpr bool operator == (const move_t& mv) { return mv.m == m; }
    inline std::string to_str() {
        static const std::string promstr = "0pnbrqk";
        std::string str;
        str += (sqFile(from()) + 'a');
        str += ('1' + sqRank(from()));
        str += (sqFile(to()) + 'a');
        str += ('1' + sqRank(to()));
        if (promoted() != EMPTY)
            str += promstr[promoted()];
        return str;
    }
    uint16_t m;
    int16_t s;
};

template<typename T, size_t capacity>
struct container_t : public std::array<T, capacity> {
    T* begin() { return &(*this)[0]; }
    T* end() { return &(*this)[size]; }
    T const* begin() const { return &(*this)[0]; }
    T const* end() const { return &(*this)[size]; }

    container_t() : size(0) { }
    void add(const T& m) { (*this)[size] = m; ++size; }
    int size;
};

template<int N>
struct movelist_t : public container_t<move_t, N> {};

#ifdef TUNE
typedef double basic_score_t;
#else
typedef int16_t basic_score_t;
#endif

struct score_t {
    score_t() : m(0), e(0) {};
    score_t(basic_score_t mm, basic_score_t ee) : m(mm), e(ee) {}
    inline score_t operator+(const score_t &d) { return score_t(m + d.m, e + d.e); }
    inline score_t operator-(const score_t &d) { return score_t(m - d.m, e - d.e); }
    inline score_t operator/(const score_t &d) { return score_t(m / d.m, e / d.e); }
    inline score_t operator*(const score_t &d) { return score_t(m * d.m, e * d.e); }
    inline score_t operator+(const basic_score_t x) { return score_t(m + x, e + x); }
    inline score_t operator-(const basic_score_t x) { return score_t(m - x, e - x); }
    inline score_t operator/(const basic_score_t x) { return score_t(m / x, e / x); }
    inline score_t operator*(const basic_score_t x) { return score_t(m * x, e * x); }
    inline score_t& operator+=(const score_t &d) { m += d.m, e += d.e; return *this; }
    inline score_t& operator-=(const score_t &d) { m -= d.m, e -= d.e; return *this; }
    inline score_t& operator/=(const score_t &d) { m /= d.m, e /= d.e; return *this; }
    inline score_t& operator*=(const score_t &d) { m *= d.m, e *= d.e; return *this; }
    inline score_t& operator+=(const basic_score_t x) { m += x, e += x; return *this; }
    inline score_t& operator-=(const basic_score_t x) { m -= x, e -= x; return *this; }
    inline score_t& operator/=(const basic_score_t x) { m /= x, e /= x; return *this; }
    inline score_t& operator*=(const basic_score_t x) { m *= x, e *= x; return *this; }
    inline bool operator==(const score_t &d) { return (d.e == e) && (d.m == m); }
    inline std::string to_str() {
        return  "{" + std::to_string((int)std::round(m)) + ", " + std::to_string((int)std::round(e)) + "}";
    }
    basic_score_t m;
    basic_score_t e;
};

class spinlock_t {
public:
    spinlock_t() {
        mLock.clear(std::memory_order_release);
    }
    void lock() {
        while (mLock.test_and_set(std::memory_order_acquire));
    }
    void unlock() {
        mLock.clear(std::memory_order_release);
    }
private:
    std::atomic_flag mLock = ATOMIC_FLAG_INIT;
};

struct material_t {
    score_t value;
    int16_t phase;
    int16_t flags;
};

enum LogLevel {
    logIN, logOUT, logINFO
};

struct LogToFile : public std::ofstream {
    static LogToFile& Inst() {
        static LogToFile logger;
        return logger;
    }
    LogToFile(const std::string& f = "invictus.log") : std::ofstream(f.c_str(), std::ios::app) {}
};

const uint64_t A1BB = 0x0000000000000001ULL;
const uint64_t B1BB = 0x0000000000000002ULL;
const uint64_t C1BB = 0x0000000000000004ULL;
const uint64_t D1BB = 0x0000000000000008ULL;
const uint64_t E1BB = 0x0000000000000010ULL;
const uint64_t F1BB = 0x0000000000000020ULL;
const uint64_t G1BB = 0x0000000000000040ULL;
const uint64_t H1BB = 0x0000000000000080ULL;
const uint64_t A2BB = 0x0000000000000100ULL;
const uint64_t B2BB = 0x0000000000000200ULL;
const uint64_t C2BB = 0x0000000000000400ULL;
const uint64_t D2BB = 0x0000000000000800ULL;
const uint64_t E2BB = 0x0000000000001000ULL;
const uint64_t F2BB = 0x0000000000002000ULL;
const uint64_t G2BB = 0x0000000000004000ULL;
const uint64_t H2BB = 0x0000000000008000ULL;
const uint64_t A3BB = 0x0000000000010000ULL;
const uint64_t B3BB = 0x0000000000020000ULL;
const uint64_t C3BB = 0x0000000000040000ULL;
const uint64_t D3BB = 0x0000000000080000ULL;
const uint64_t E3BB = 0x0000000000100000ULL;
const uint64_t F3BB = 0x0000000000200000ULL;
const uint64_t G3BB = 0x0000000000400000ULL;
const uint64_t H3BB = 0x0000000000800000ULL;
const uint64_t A4BB = 0x0000000001000000ULL;
const uint64_t B4BB = 0x0000000002000000ULL;
const uint64_t C4BB = 0x0000000004000000ULL;
const uint64_t D4BB = 0x0000000008000000ULL;
const uint64_t E4BB = 0x0000000010000000ULL;
const uint64_t F4BB = 0x0000000020000000ULL;
const uint64_t G4BB = 0x0000000040000000ULL;
const uint64_t H4BB = 0x0000000080000000ULL;
const uint64_t A5BB = 0x0000000100000000ULL;
const uint64_t B5BB = 0x0000000200000000ULL;
const uint64_t C5BB = 0x0000000400000000ULL;
const uint64_t D5BB = 0x0000000800000000ULL;
const uint64_t E5BB = 0x0000001000000000ULL;
const uint64_t F5BB = 0x0000002000000000ULL;
const uint64_t G5BB = 0x0000004000000000ULL;
const uint64_t H5BB = 0x0000008000000000ULL;
const uint64_t A6BB = 0x0000010000000000ULL;
const uint64_t B6BB = 0x0000020000000000ULL;
const uint64_t C6BB = 0x0000040000000000ULL;
const uint64_t D6BB = 0x0000080000000000ULL;
const uint64_t E6BB = 0x0000100000000000ULL;
const uint64_t F6BB = 0x0000200000000000ULL;
const uint64_t G6BB = 0x0000400000000000ULL;
const uint64_t H6BB = 0x0000800000000000ULL;
const uint64_t A7BB = 0x0001000000000000ULL;
const uint64_t B7BB = 0x0002000000000000ULL;
const uint64_t C7BB = 0x0004000000000000ULL;
const uint64_t D7BB = 0x0008000000000000ULL;
const uint64_t E7BB = 0x0010000000000000ULL;
const uint64_t F7BB = 0x0020000000000000ULL;
const uint64_t G7BB = 0x0040000000000000ULL;
const uint64_t H7BB = 0x0080000000000000ULL;
const uint64_t A8BB = 0x0100000000000000ULL;
const uint64_t B8BB = 0x0200000000000000ULL;
const uint64_t C8BB = 0x0400000000000000ULL;
const uint64_t D8BB = 0x0800000000000000ULL;
const uint64_t E8BB = 0x1000000000000000ULL;
const uint64_t F8BB = 0x2000000000000000ULL;
const uint64_t G8BB = 0x4000000000000000ULL;
const uint64_t H8BB = 0x8000000000000000ULL;

constexpr uint64_t BitMask[64] = {
    A1BB, B1BB, C1BB, D1BB, E1BB, F1BB, G1BB, H1BB,
    A2BB, B2BB, C2BB, D2BB, E2BB, F2BB, G2BB, H2BB,
    A3BB, B3BB, C3BB, D3BB, E3BB, F3BB, G3BB, H3BB,
    A4BB, B4BB, C4BB, D4BB, E4BB, F4BB, G4BB, H4BB,
    A5BB, B5BB, C5BB, D5BB, E5BB, F5BB, G5BB, H5BB,
    A6BB, B6BB, C6BB, D6BB, E6BB, F6BB, G6BB, H6BB,
    A7BB, B7BB, C7BB, D7BB, E7BB, F7BB, G7BB, H7BB,
    A8BB, B8BB, C8BB, D8BB, E8BB, F8BB, G8BB, H8BB
};

const uint64_t FileABB = 0x0101010101010101ULL;
const uint64_t FileBBB = 0x0202020202020202ULL;
const uint64_t FileCBB = 0x0404040404040404ULL;
const uint64_t FileDBB = 0x0808080808080808ULL;
const uint64_t FileEBB = 0x1010101010101010ULL;
const uint64_t FileFBB = 0x2020202020202020ULL;
const uint64_t FileGBB = 0x4040404040404040ULL;
const uint64_t FileHBB = 0x8080808080808080ULL;

constexpr uint64_t FileBB[8] = { FileABB, FileBBB, FileCBB,FileDBB, FileEBB, FileFBB,FileGBB, FileHBB };

const uint64_t Rank1BB = 0x00000000000000FFULL;
const uint64_t Rank2BB = 0x000000000000FF00ULL;
const uint64_t Rank3BB = 0x0000000000FF0000ULL;
const uint64_t Rank4BB = 0x00000000FF000000ULL;
const uint64_t Rank5BB = 0x000000FF00000000ULL;
const uint64_t Rank6BB = 0x0000FF0000000000ULL;
const uint64_t Rank7BB = 0x00FF000000000000ULL;
const uint64_t Rank8BB = 0xFF00000000000000ULL;

constexpr uint64_t RankBB[8] = { Rank1BB, Rank2BB, Rank3BB, Rank4BB, Rank5BB,Rank6BB, Rank7BB, Rank8BB };

const uint64_t EmptyBoardBB = 0ULL;
const uint64_t FullBoardBB = 0xFFFFFFFFFFFFFFFFULL;

const uint64_t WhiteSquaresBB = 0x55AA55AA55AA55AAULL;
const uint64_t BlackSquaresBB = 0xAA55AA55AA55AA55ULL;

const uint64_t Rank8ByColorBB[2] = { Rank8BB, Rank1BB };
const uint64_t Rank7ByColorBB[2] = { Rank7BB, Rank2BB };
const uint64_t Rank6ByColorBB[2] = { Rank6BB, Rank3BB };
const uint64_t Rank5ByColorBB[2] = { Rank5BB, Rank4BB };
const uint64_t Rank4ByColorBB[2] = { Rank4BB, Rank5BB };
const uint64_t Rank3ByColorBB[2] = { Rank3BB, Rank6BB };
const uint64_t Rank2ByColorBB[2] = { Rank2BB, Rank7BB };
const uint64_t Rank1ByColorBB[2] = { Rank1BB, Rank8BB };
