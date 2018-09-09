/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/
#pragma once
#include <functional>
#include <thread>
#include "typedefs.h"
#include "trans.h"
#include "utils.h"
#include "log.h"
#include "eval.h"

namespace Search {
	void initArr();
}

class thread_t {
public:
	thread_t(int _thread_id) : thread_id(_thread_id) {
		exit_flag = false;
		doSleep = true;
	}
	~thread_t() {
		exit_flag = true;
		wakeup();
		nativeThread.join();
	}
	void wait() {
		std::unique_lock<std::mutex> lk(threadLock);
		sleepCondition.wait(lk);
	}
	void wakeup() {
		doSleep = false;
		sleepCondition.notify_one();
	}
	bool doSleep;
	int thread_id;
protected:
	bool exit_flag;
	std::thread nativeThread;
	std::condition_variable sleepCondition;
	std::mutex threadLock;
};

struct engine_t;

struct search_t : public thread_t {
	static const int MAXPLY = 127;
	static const int MAXPLYSIZE = 128;
	static const int MATE = 32750;

	search_t(int _thread_id, engine_t& _e) : e(_e), thread_t(_thread_id) {
		nativeThread = std::thread(&search_t::idleloop, this);
	}

	void idleloop();
	uint64_t perft(int depth);
	uint64_t perft2(int depth);
	void extractPV(move_t rmove, bool fillhash);
	void updateInfo();
	void displayInfo(move_t bestmove, int depth, int alpha, int beta);
	void start();
	bool stopSearch();
	int search(bool root, bool inPv, int alpha, int beta, int depth, int ply, bool inCheck);
	int qsearch(int alpha, int beta, int ply, bool inCheck);
	void updateHistory(position_t& p, move_t bm, int depth, int ply);

	position_t pos;
	engine_t& e;

	int maxplysearched;
	int depth;
	std::atomic<uint64_t> nodecnt;
	std::atomic<bool> stop_iter;
	std::atomic<bool> resolve_iter;

	eval_t eval;
	move_t rootmove;
	movelist_t<128> pvlist;
	movelist_t<64> playedmoves[MAXPLYSIZE];
	uint16_t killer1[MAXPLYSIZE];
	uint16_t killer2[MAXPLYSIZE];
	int history[2][8][64];
};
