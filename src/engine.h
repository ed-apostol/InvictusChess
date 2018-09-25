/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/
#pragma once

#include <functional>
#include <thread>
#include <unordered_map>
#include "typedefs.h"
#include "trans.h"
#include "utils.h"
#include "log.h"
#include "eval.h"
#include "search.h"

struct uci_limits_t {
    void init() {
        wtime = 0;
        btime = 0;
        winc = 0;
        binc = 0;
        movestogo = 0;
        depth = 0;
        movetime = 0;
        mate = 0;
        infinite = false;
        ponder = false;
        nodes = 0;
    };
    int wtime;
    int btime;
    int winc;
    int binc;
    int movestogo;
    int depth;
    int movetime;
    int mate;
    bool infinite;
    bool ponder;
    uint64_t nodes;
};

struct uci_options_t {
    using callback = std::function<void()>;
    uci_options_t() {}
    uci_options_t(std::string v, callback f) : type("string"), min(0), max(0), onChange(f) {
        defaultval = currval = v;
    }
    uci_options_t(bool v, callback f) : type("check"), min(0), max(0), onChange(f) {
        defaultval = currval = (v ? "true" : "false");
    }
    uci_options_t(callback f) : type("button"), min(0), max(0), onChange(f) {
    }
    uci_options_t(int v, int minv, int maxv, callback f) : type("spin"), min(minv), max(maxv), onChange(f) {
        defaultval = currval = std::to_string(v);
    }
    int getIntVal() const {
        return (type == "spin" ? atoi(currval.c_str()) : currval == "true");
    }
    std::string getStrVal() {
        return currval;
    }
    uci_options_t& operator=(const std::string& val) {
        if (type != "button") currval = val;
        onChange();
        return *this;
    }
    std::string defaultval, currval, type;
    int min, max;
    callback onChange;
};

struct uci_options_map : public std::unordered_map<std::string, uci_options_t> {
    uci_options_t & operator[] (std::string const& key) {
        auto opt = find(key);
        if (opt != end()) return opt->second;
        else {
            mKeys.push_back(key);
            return (insert(std::make_pair(key, uci_options_t())).first)->second;
        }
    }
    void print() const {
        for (auto key : mKeys) {
            auto itr = find(key);
            if (itr != end()) {
                const uci_options_t& opt = itr->second;
                LogAndPrintOutput log;
                log << "option name " << itr->first << " type " << opt.type;
                if (opt.type != "button") log << " default " << opt.defaultval;
                if (opt.type == "spin") log << " min " << opt.min << " max " << opt.max;
            }
        }
    }
    std::vector<std::string> mKeys;
};

struct engine_t : public std::vector<search_t*> {
    engine_t();
    ~engine_t();
    void initSearch();
    void newgame();
    void stopthreads();
    void initUCIoptions();
    void printUCIoptions();
    void waitForThreads();
    void ponderhit();

    void onHashChange();
    void onThreadsChange();

    uint64_t nodesearched();
    void stopIteration();
    void resolveIteration();

    abdada_table_t mht;
    pvhash_table_t pvt;
    trans_table_t tt;
    uci_options_map options;
    uci_limits_t limits;
    position_t origpos;

    std::atomic<bool> use_time;
    std::atomic<bool> stop;
    bool doSMP;
    int defer_depth;
    int cutoffcheck_depth;

    std::atomic<int> alpha;
    std::atomic<int> beta;
    std::atomic<int> rootbestdepth;
    move_t rootbestmove;
    move_t rootponder;

    int64_t start_time;
    int64_t time_limit_max;
    int64_t time_limit_abs;
};