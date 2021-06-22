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
    inline score_t operator+(const int x) { return score_t(m + x, e + x); }
    inline score_t operator-(const int x) { return score_t(m - x, e - x); }
    inline score_t operator/(const int x) { return score_t(m / x, e / x); }
    inline score_t operator*(const int x) { return score_t(m * x, e * x); }
    inline score_t& operator+=(const score_t &d) { m += d.m, e += d.e; return *this; }
    inline score_t& operator-=(const score_t &d) { m -= d.m, e -= d.e; return *this; }
    inline score_t& operator/=(const score_t &d) { m /= d.m, e /= d.e; return *this; }
    inline score_t& operator*=(const score_t &d) { m *= d.m, e *= d.e; return *this; }
    inline score_t& operator+=(const int x) { m += x, e += x; return *this; }
    inline score_t& operator-=(const int x) { m -= x, e -= x; return *this; }
    inline score_t& operator/=(const int x) { m /= x, e /= x; return *this; }
    inline score_t& operator*=(const int x) { m *= x, e *= x; return *this; }
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
