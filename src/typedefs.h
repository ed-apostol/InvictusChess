/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>

//#define ASSERT(a)
#define ASSERT(a) if (!(a)) \
{LogAndPrintDebug() << "file " << __FILE__ << ", line " << __LINE__  << " : " << "assertion \"" #a "\" failed";}

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

inline int sqRank(int sq) {
	return sq >> 3;
}

inline int  sqFile(int sq) {
	return sq & 7;
}

enum MoveFlags {
	MF_NORMAL, MF_CASTLE, MF_PAWN2, MF_ENPASSANT, MF_PROMN, MF_PROMB, MF_PROMR, MF_PROMQ
};

struct move_t {
	move_t() { }
	move_t(int v) : m(v) { }
	move_t(const move_t& mv) : m(mv.m), s(mv.s) { }
	move_t(int f, int t, MoveFlags l) {
		m = f | (t << 6) | (l << 12);
	}
	inline void init() { m = 0;  s = SHRT_MIN; }
	inline int moveFrom() const { return 63 & m; }
	inline int moveTo() const { return 63 & (m >> 6); }
	inline int movePromote() const { return (m >> 14) ? (m >> 12) - 2 : 0; }
	inline int moveFlags() const { return m >> 12; }
	inline bool isCastle() const { return MF_CASTLE == (m >> 12); }
	inline bool isPawn2Forward() const { return MF_PAWN2 == (m >> 12); }
	inline bool isPromote() const { return (m >> 14); }
	inline bool isEnPassant() const { return MF_ENPASSANT == (m >> 12); }
	inline bool operator == (const move_t& mv) { return mv.m == m; }
	inline std::string to_str() {
		static const std::string promstr = "0pnbrqk";
		std::string str;
		str += (sqFile(moveFrom()) + 'a');
		str += ('1' + sqRank(moveFrom()));
		str += (sqFile(moveTo()) + 'a');
		str += ('1' + sqRank(moveTo()));
		if (movePromote() != EMPTY)
			str += promstr[movePromote()];
		return str;
	}
	uint16_t m;
	int16_t s;
};

template<int N>
struct movelist_t {
public:
	movelist_t() : size(0) {}
	void add(const move_t& m) { tab[size] = m; ++size; }
	move_t& mv(int idx) { return tab[idx]; }
	int size;
private:
	move_t tab[N];
};

struct score_t {
	score_t() : m(0), e(0) {};
	score_t(int mm, int ee) : m(mm), e(ee) {}
	inline score_t operator+(const score_t d) { return score_t(m + d.m, e + d.e); }
	inline score_t operator-(const score_t d) { return score_t(m - d.m, e - d.e); }
	inline score_t operator/(const score_t d) { return score_t(m / d.m, e / d.e); }
	inline score_t operator*(const score_t d) { return score_t(m * d.m, e * d.e); }
	inline score_t operator+(const int x) { return score_t(m + x, e + x); }
	inline score_t operator-(const int x) { return score_t(m - x, e - x); }
	inline score_t operator/(const int x) { return score_t(m / x, e / x); }
	inline score_t operator*(const int x) { return score_t(m * x, e * x); }
	inline score_t& operator+=(const score_t d) { m += d.m, e += d.e; return *this; }
	inline score_t& operator-=(const score_t d) { m -= d.m, e -= d.e; return *this; }
	inline score_t& operator/=(const score_t d) { m /= d.m, e /= d.e; return *this; }
	inline score_t& operator*=(const score_t d) { m *= d.m, e *= d.e; return *this; }
	inline score_t& operator+=(const int x) { m += x, e += x; return *this; }
	inline score_t& operator-=(const int x) { m -= x, e -= x; return *this; }
	inline score_t& operator/=(const int x) { m /= x, e /= x; return *this; }
	inline score_t& operator*=(const int x) { m *= x, e *= x; return *this; }
	inline bool operator==(const score_t d) { return (d.e == e) && (d.m == m); }

	int16_t m;
	int16_t e;
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
	std::atomic_flag mLock;
};
