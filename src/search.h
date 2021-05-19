/**************************************************/
/*  Invictus 2021                                 */
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
        do_sleep = true;
    }
    ~thread_t() {
        exit_flag = true;
        wakeup();
        native_thread.join();
    }
    void wait() {
        std::unique_lock<std::mutex> lk(thread_lock);
        sleep_condition.wait(lk);
    }
    void wakeup() {
        do_sleep = false;
        sleep_condition.notify_one();
    }
    bool do_sleep;
    int thread_id;
protected:
    bool exit_flag;
    std::thread native_thread;
    std::condition_variable sleep_condition;
    std::mutex thread_lock;
};

struct engine_t;

struct search_t : public thread_t {
    search_t(int _thread_id, engine_t& _e) : e(_e), thread_t(_thread_id) {
        native_thread = std::thread(&search_t::idleloop, this);
        et.init(1);
    }

    void idleloop();
    uint64_t perft(size_t depth);
    uint64_t perft2(int depth);
    void updateInfo();
    void displayInfo(move_t bestmove, int depth, int alpha, int beta);
    void start();
    bool stopSearch();
    int search(bool inRoot, bool inPv, int alpha, int beta, int depth, int ply, bool inCheck);
    int qsearch(bool inPv, int alpha, int beta, int ply, bool inCheck);
    void updateHistory(position_t& p, move_t bm, int depth, int ply);

    position_t pos;
    engine_t& e;
    eval_table_t et;

    int maxplysearched;
    int rdepth;
    std::atomic<uint64_t> nodecnt;
    std::atomic<bool> stop_iter;

    move_t rootmove;
    movelist_t<128> pvlist[MAXPLYSIZE];
    movelist_t<64> playedmoves[MAXPLYSIZE];
    int evalvalue[MAXPLYSIZE];
    uint16_t killer1[MAXPLYSIZE];
    uint16_t killer2[MAXPLYSIZE];
    uint16_t countermove[2][7][64];
    int history[2][64][64];
};
