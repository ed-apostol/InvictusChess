/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <algorithm>
#include <cmath>
#include "typedefs.h"
#include "search.h"
#include "engine.h"
#include "position.h"
#include "utils.h"
#include "bitutils.h"
#include "eval.h"
#include "log.h"
#include "movepicker.h"
#include "params.h"

namespace Search {
    const int CMHistDepth[2] = { 3, 2 };
    const int CMHistLimit[2] = { 0, -100 };
    const int FMHistDepth[2] = { 3, 2 };
    const int FMHistLimit[2] = { -100, -200 };
    const int FPHistLimit[2] = { 600, 500 };
    const int FMHistReduction = 600;

    int LMRTable[64][64];
    int LMPTable[2][9];
    void initArr() {
        //LogAndPrintOutput logger;
        for (int d = 1; d < 64; d++) {
            for (int p = 1; p < 64; p++) {
                LMRTable[d][p] = 0.75 + log(d) * log(p) / 2.1;
                //logger << " " << LMRTable[d][p];
            }
            //logger << "\n";
        }
        for (int depth = 1; depth < 9; depth++) {
            LMPTable[0][depth] = 3 + depth * depth;
            LMPTable[1][depth] = 3 + depth * depth;
            //logger << " " << LMPTable[0][depth];
        }
        //logger << "\n";
    }
    inline int scoreFromTrans(int score, int ply, int mate) {
        return (score >= mate) ? (score - ply) : ((score <= -mate) ? (score + ply) : score);
    }
    inline int scoreToTrans(int score, int ply, int mate) {
        return (score >= mate) ? (score + ply) : ((score <= -mate) ? (score - ply) : score);
    }
    inline void updatePV(movelist_t<MAXPLYSIZE> pv[], move_t& m, int ply) {
        pv[ply].size = 0;
        pv[ply].add(m);
        for (auto mm : pv[ply + 1]) pv[ply].add(mm);
    }
}

using namespace Search;
using namespace EvalParam;

void search_t::idleloop() {
    while (!exit_flag) {
        if (do_sleep) wait();
        else {
            start();
            do_sleep = true;
        }
    }
}

// use this for checking position routines: doMove and undoMove
uint64_t search_t::perft(size_t depth) {
    undo_t undo;
    uint64_t cnt = 0ull;
    if (depth == 0) return 1ull;
    movelist_t<256> mvlist;
    bool inCheck = pos.kingIsInCheck();
    if (inCheck) pos.genCheckEvasions(mvlist);
    else {
        pos.genTacticalMoves(mvlist);
        pos.genQuietMoves(mvlist);
    }
    uint64_t pinned = pos.pinnedPiecesBB(pos.side);
    for (move_t m : mvlist) {
        if (!pos.moveIsLegal(m, pinned, inCheck)) continue;
        pos.doMove(undo, m, ply);
        cnt += perft(depth - 1);
        pos.undoMove(undo, ply);
    }
    return cnt;
}

// use this for checking move generation: faster
uint64_t search_t::perft2(int depth) {
    movelist_t<256> mvlist;
    pos.genLegal(mvlist);
    if (depth == 1) return mvlist.size;
    undo_t undo;
    uint64_t cnt = 0ull;
    for (move_t m : mvlist) {
        pos.doMove(undo, m, ply);
        cnt += perft2(depth - 1);
        pos.undoMove(undo, ply);
    }
    return cnt;
}

void search_t::updateInfo() {
    uint64_t currtime = Utils::getTime() - e.start_time + 1;
    uint64_t totalnodes = e.nodesearched();
    PrintOutput() << "info time " << currtime << " nodes " << totalnodes << " nps " << (totalnodes * 1000 / currtime);
}

void search_t::displayInfo(move_t bestmove, int depth, int alpha, int beta) {
    PrintOutput logger;
    uint64_t currtime = Utils::getTime() - e.start_time + 1;
    logger << "info depth " << depth << " seldepth " << maxplysearched;
    if (abs(bestmove.s) < MATE - MAXPLY) {
        if (bestmove.s <= alpha) logger << " score cp " << bestmove.s << " upperbound";
        else if (bestmove.s >= beta) logger << " score cp " << bestmove.s << " lowerbound";
        else logger << " score cp " << bestmove.s;
    }
    else
        logger << " score mate " << ((bestmove.s > 0) ? (MATE - bestmove.s + 1) / 2 : -(MATE + bestmove.s) / 2);
    uint64_t totalnodes = e.nodesearched();
    logger << " time " << currtime << " nodes " << totalnodes << " nps " << (totalnodes * 1000 / currtime) << " pv";
    for (move_t m : pvlist[0]) logger << " " << m.to_str();
}

void search_t::start() {
    if (e.doNUMA) Utils::bindThisThread(thread_id); // NUMA bindings

    memset(history, 0, sizeof(history));
    memset(caphistory, 0, sizeof(caphistory));
    memset(countermove, 0, sizeof(countermove));
    memset(killer1, 0, sizeof(killer1));
    memset(killer2, 0, sizeof(killer2));
    memset(cmh, 0, sizeof(cmh));
    memset(fmh, 0, sizeof(fmh));

    ply = 0;
    nodecnt = 0;
    bool inCheck = pos.kingIsInCheck();
    int last_score = 0;
    int mate_count = 0;

    for (rdepth = e.rdepth; rdepth <= e.limits.depth; rdepth = e.rdepth) {
        int delta = 10;
        maxplysearched = 0;
        while (true) {
            stop_iter = false;
            search(true, true, e.alpha, e.beta, rdepth, inCheck);
            if (e.stop || e.plysearched[rdepth - 1]) break;
            else if (stop_iter) {
                if (e.resolve_iter) continue;
                else break;
            }
            else {
                std::lock_guard<spinlock_t> lock(e.updatelock);
                if (e.stop || e.plysearched[rdepth - 1]) break;
                else if (stop_iter) {
                    if (e.resolve_iter) continue;
                    else break;
                }
                if (rootmove.s <= e.alpha)
                    e.beta = (e.alpha + e.beta) / 2,
                    e.alpha = std::max(-MATE, rootmove.s - delta);
                else if (rootmove.s >= e.beta)
                    e.beta = std::min(MATE, rootmove.s + delta);
                else {
                    e.plysearched[rdepth - 1] = true;
                    e.resolve_iter = false;
                    e.rootbestmove = rootmove;
                    if (pvlist[0].size > 1) e.rootponder = pvlist[0].mv(1);
                    if (rdepth >= 12) displayInfo(rootmove, rdepth, e.alpha, e.beta);
                    e.rdepth = ++rdepth;
                    if (rdepth >= 5)
                        e.alpha = std::max(-MATE, e.rootbestmove.s - delta),
                        e.beta = std::min(MATE, e.rootbestmove.s + delta);
                    else
                        e.alpha = -MATE,
                        e.beta = MATE;
                    e.stopIteration();
                    break;
                }
                delta += delta / 2;
                e.resolve_iter = true;
                e.stopIteration();
            }
        }
        if (e.stop) break;
        if (thread_id == 0 && e.use_time) {
            int64_t currtime = Utils::getTime();
            if (currtime - e.start_time >= ((e.time_limit_max - e.start_time) * 65) / 100) {
                if (e.rootbestmove.s + 20 <= last_score)
                    e.time_limit_max = std::min(e.time_limit_max + (e.time_range * (last_score - e.rootbestmove.s)) / 40, e.time_limit_abs);
                else
                    break;
            }
            last_score = e.rootbestmove.s;
            if (rdepth >= 30 && abs(last_score) > MATE - MAXPLY) ++mate_count;
            if (mate_count >= 4) break;
        }
    }

    if (!e.stop && (e.limits.ponder || e.limits.infinite)) {
        while (!e.use_time && !e.stop) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    else e.stopthreads();

    if (thread_id == 0) {
        updateInfo();
        LogAndPrintOutput logger;
        logger << "bestmove " << e.rootbestmove.to_str();
        if (pvlist[0].size > 1) logger << " ponder " << e.rootponder.to_str();
    }
}

bool search_t::stopSearch() {
    ++nodecnt;
    if (thread_id == 0 && e.use_time && (nodecnt & 0x3fff) == 0) {
        int64_t currtime = Utils::getTime();
        if ((currtime >= e.time_limit_max && !e.resolve_iter) || (currtime >= e.time_limit_abs)) {
            if (e.rootbestmove.m == 0)
                e.time_limit_max = std::min(e.time_limit_max + e.time_range / 2, e.time_limit_abs);
            else
                e.stop = true;
        }
    }
    if (thread_id == 0 && (nodecnt & 0x3fffff) == 0) updateInfo();
    return e.stop;
}

int search_t::search(bool inRoot, bool inPv, int alpha, int beta, int depth, bool inCheck) {
    if (depth <= 0) return qsearch(inPv, alpha, beta, inCheck);

    ASSERT(alpha < beta);
    ASSERT(!(pos.colorBB[WHITE] & pos.colorBB[BLACK]));

    pvlist[ply].size = 0;

    if (!inRoot) {
        if (stopSearch()) return 0;
        if (inPv && ply > maxplysearched) maxplysearched = ply;
        if (pos.stack.fifty > 99 || pos.isRepeat() || pos.isMatDrawn()) return 0;
        if (ply >= MAXPLY) return et.retrieve(pos);
        alpha = std::max(alpha, -MATE + ply);
        beta = std::min(beta, MATE - ply - 1);
        if (alpha >= beta) return alpha;
    }

    tt_entry_t tte;
    tte.move.m = 0;
    int tscore = NOVALUE;
    if (e.tt.retrieve(pos.stack.hash, tte)) {
        tscore = scoreFromTrans(tte.move.s, ply, MATE - MAXPLY);
        if (!inPv && tte.depth >= depth && ((tscore >= beta)
            ? (tte.getBound() & TT_LOWER)
            : (tte.getBound() & TT_UPPER)))
            return tscore;
    }

    int evalscore = evalvalue[ply] = (tscore != NOVALUE) ? tscore : et.retrieve(pos);
    const bool nonpawnpcs = pos.colorBB[pos.side] & ~(pos.piecesBB[PAWN] | pos.piecesBB[KING]);
    const uint64_t dcc = pos.discoveredPiecesBB(pos.side);
    undo_t& undo = stack[ply];

    if (!inPv && !inCheck) {
        if (depth < 9 && evalscore - 85 * depth > beta) // TODO: test
            return evalscore;
        if (depth >= 2 && evalscore >= beta && nonpawnpcs && pos.stack.lastmove.m != 0) {
            int R = ((13 + depth) >> 2) + std::min(3, (evalscore - beta) / 185); // TODO: test
            pos.doNullMove(undo, ply);
            int score = -search(false, false, -beta, -beta + 1, depth - R, false);
            pos.undoNullMove(undo, ply);
            if (e.stop || stop_iter) return 0;
            if (score >= beta) {
                if (score >= MATE - MAXPLY) score = beta;
                if (depth < 12 && abs(beta) < MATE - MAXPLY) return score;
                int score2 = search(false, false, alpha, beta, depth - R, inCheck);
                if (e.stop || stop_iter) return 0;
                if (score2 >= beta) return score;
            }
        }
        if (depth >= 5 && std::abs(beta) < MATE - MAXPLY && evalscore >= beta) {
            int rbeta = std::min(beta + 100, MATE - MAXPLY);
            movepicker_t mp(*this, inCheck, true, rbeta - evalscore);
            for (move_t m; mp.getMoves(m);) {
                bool moveGivesCheck = pos.moveIsCheck(m, dcc);
                pos.doMove(undo, m, ply);
                int score = -qsearch(inPv, -rbeta, -rbeta + 1, moveGivesCheck);
                if (score >= rbeta) score = -search(false, false, -rbeta, -rbeta + 1, depth - 4, moveGivesCheck);
                pos.undoMove(undo, ply);
                if (e.stop || stop_iter) return 0;
                if (score >= rbeta) return score;
            }
        }
    }

    const int futility = evalscore + (90 * depth) + 250; // TODO: test
    const int futilityHistory = evalscore + (90 * depth);
    const int improving = ply >= 2 && evalvalue[ply] > evalvalue[ply - 2];
    const int old_alpha = alpha;
    int best_score = NOVALUE;
    int movestried = 0;
    move_t best_move(0);
    int score;
    move_t lm = pos.stack.lastmove;
    uint16_t cm = countermove[pos.side ^ 1][pos.stack.movingpc][pos.stack.dest];
    movepicker_t mp(*this, inCheck, false, 1, tte.move.m, killer1[ply], killer2[ply], cm);
    bool skipquiets = false;
    playedmoves[ply].size = 0;
    playedcaps[ply].size = 0;
    for (move_t m; mp.getMoves(m, skipquiets);) {
        if (e.doSMP && mp.stage == STG_DEFERRED) movestried = m.s;
        else ++movestried;

        bool moveGivesCheck = pos.moveIsCheck(m, dcc);
        bool isTactical = pos.moveIsTactical(m);

        if (best_score == NOVALUE) {
            int extension = 0;
            if (moveGivesCheck) extension = 1;
            else if (inCheck && mp.mvlist.size == 1) extension = 1;
            else if (!inRoot && depth >= 8 && tscore != NOVALUE && tte.move.m == m.m && tte.depth >= depth - 2 && tte.getBound() & TT_LOWER) {
                int xbeta = std::max(tscore - depth * 2, -MATE), xscore = NOVALUE, quiets = 0, tactical = 0;
                bool skipqx = false;
                movepicker_t mpx(*this, inCheck, false, 1, tte.move.m, killer1[ply], killer2[ply], cm);
                for (move_t mx; mpx.getMoves(mx, skipqx);) {
                    if (mx.m == tte.move.m) continue;
                    bool givesCheck = pos.moveIsCheck(mx, dcc);
                    pos.doMove(undo, mx, ply);
                    xscore = -search(false, inPv, -xbeta - 1, -xbeta, depth / 2 - 1, givesCheck);
                    pos.undoMove(undo, ply);
                    if (e.stop || stop_iter) return 0;
                    if (xscore >= xbeta) break;
                    pos.moveIsTactical(mx) ? ++tactical : ++quiets;
                    skipqx = quiets >= 6;
                    if (skipqx && tactical >= 3) break;
                }
                if (xscore != NOVALUE && xscore < xbeta) extension = 1;
                else if (xbeta >= beta) return xbeta;
            }
            pos.doMove(undo, m, ply);
            score = -search(false, inPv, -beta, -alpha, depth - 1 + extension, moveGivesCheck);
            pos.undoMove(undo, ply);
        }
        else {
            uint32_t move_hash = 0;
            const bool doABDADA = (e.doSMP && mp.stage != STG_DEFERRED && depth >= e.defer_depth && !inCheck);
            if (doABDADA) {
                if (!inPv && mp.deferred.size > 0 && depth >= e.cutoffcheck_depth && e.tt.retrieve(pos.stack.hash, tte)) {
                    tscore = scoreFromTrans(tte.move.s, ply, MATE - MAXPLY);
                    if (tte.depth >= depth && ((tscore >= beta)
                        ? (tte.getBound() & TT_LOWER)
                        : (tte.getBound() & TT_UPPER)))
                        return tscore;
                }
                move_hash = pos.stack.hash >> 32;
                move_hash ^= (m.m * 1664525) + 1013904223;
                if (e.mht.isBusy(move_hash, m.m, depth)) {
                    m.s = movestried;
                    mp.deferred.add(m);
                    continue;
                }
            }

            int R = LMRTable[std::min(depth, 63)][std::min(movestried, 63)];
            int h, ch, fh;
            getHistoryValues(h, ch, fh, m);

            if (!inPv && !inCheck && !moveGivesCheck && !isTactical && depth < 9 && mp.stage != STG_DEFERRED) {
                if (movestried >= LMPTable[improving][depth]) { skipquiets = true; continue; }
                if (futility <= alpha) { skipquiets = true; continue; }
                if (futilityHistory <= alpha && (h + ch + fh) < FPHistLimit[improving]) { skipquiets = true; continue; }
                if (depth - R <= CMHistDepth[improving] && ch < CMHistLimit[improving]) continue;
                if (depth - R <= FMHistDepth[improving] && fh < FMHistLimit[improving]) continue;
                if (!pos.staticExchangeEval(m, -10 * depth * depth)) continue;
            }
            if (!inPv && !inCheck && !moveGivesCheck && depth < 9 && mp.stage == STG_BADTACTICS) {
                if (!pos.staticExchangeEval(m, -100 * depth)) continue;
            }

            pos.doMove(undo, m, ply);

            int reduction = 1;
            if (!inCheck && !moveGivesCheck && !isTactical && depth > 2) {
                reduction = R;
                reduction += !inPv + !improving;
                reduction -= (m.m == mp.killer1) || (m.m == mp.killer2) || (m.m == mp.counter);
                reduction -= std::max(-2, std::min(2, (h + ch + fh) / FMHistReduction));
                reduction = std::min(depth - 1, std::max(reduction, 1));
            }

            if (doABDADA) e.mht.setBusy(move_hash, m.m, depth);
            score = -search(false, false, -alpha - 1, -alpha, depth - reduction, moveGivesCheck);
            if (doABDADA) e.mht.resetBusy(move_hash, m.m, depth);

            if (reduction > 1 && !e.stop && !stop_iter && score > alpha)
                score = -search(false, false, -alpha - 1, -alpha, depth - 1, moveGivesCheck);

            if (inPv && !e.stop && !stop_iter && score > alpha)
                score = -search(false, inPv, -beta, -alpha, depth - 1, moveGivesCheck);

            pos.undoMove(undo, ply);
        }
        if (e.stop || stop_iter) return 0;

        isTactical ? playedcaps[ply].add(m) : playedmoves[ply].add(m);

        if (score > best_score) {
            best_score = score;
            if (inRoot) {
                rootmove.m = m.m;
                rootmove.s = best_score;
                updatePV(pvlist, m, ply);
            }
            if (score > alpha) {
                best_move.m = m.m;
                best_move.s = score;
                if (!inRoot && inPv) updatePV(pvlist, m, ply);
                if (score >= beta) break;
                alpha = score;
            }
        }
    }
    if (movestried == 0) {
        if (inCheck) return -MATE + ply;
        else return 0;
    }
    if (!inCheck && best_move.m != 0) {
        if (!pos.moveIsTactical(best_move)) updateHistory(best_move, depth);
        updateCapHistory(best_move, depth);
    }
    best_move.s = scoreToTrans(best_score, ply, MATE - MAXPLY);
    e.tt.store(pos.stack.hash, best_move, depth, (best_score >= beta) ? TT_LOWER : ((inPv && best_score > old_alpha) ? TT_EXACT : TT_UPPER));
    return best_score;
}

int search_t::qsearch(bool inPv, int alpha, int beta, bool inCheck) {
    ASSERT(alpha < beta);
    ASSERT(!(pos.colorBB[WHITE] & pos.colorBB[BLACK]));

    pvlist[ply].size = 0;
    if (stopSearch()) return 0;

    if (inPv && ply > maxplysearched) maxplysearched = ply;
    if (pos.stack.fifty > 99 || pos.isRepeat() || pos.isMatDrawn()) return 0;
    if (ply >= MAXPLY) return et.retrieve(pos);

    tt_entry_t tte;
    tte.move.m = 0;
    int tscore = NOVALUE;
    if (e.tt.retrieve(pos.stack.hash, tte)) {
        tscore = scoreFromTrans(tte.move.s, ply, MATE - MAXPLY);
        if (!inPv && ((tscore >= beta)
            ? (tte.getBound() & TT_LOWER)
            : (tte.getBound() & TT_UPPER)))
            return tscore;
    }

    const int old_alpha = alpha;
    int best_score = NOVALUE;
    if (!inCheck) {
        best_score = (tscore != NOVALUE) ? tscore : et.retrieve(pos);
        if (best_score >= beta) return best_score;
        alpha = std::max(alpha, best_score);
    }

    undo_t& undo = stack[ply];
    move_t best_move(0);
    int movestried = 0;
    movepicker_t mp(*this, inCheck, true, std::max(1, alpha - best_score - 100), tte.move.m);
    uint64_t dcc = pos.discoveredPiecesBB(pos.side);
    for (move_t m; mp.getMoves(m);) {
        ++movestried;
        bool moveGivesCheck = pos.moveIsCheck(m, dcc);
        pos.doMove(undo, m, ply);
        int score = -qsearch(inPv, -beta, -alpha, moveGivesCheck);
        pos.undoMove(undo, ply);
        if (e.stop || stop_iter) return 0;
        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                best_move.m = m.m;
                best_move.s = best_score;
                if (inPv) updatePV(pvlist, m, ply);
                if (score >= beta) break;
                alpha = score;
            }
        }
    }
    if (movestried == 0 && inCheck) return -MATE + ply;
    best_move.s = scoreToTrans(best_score, ply, MATE - MAXPLY);
    e.tt.store(pos.stack.hash, best_move, 0, (best_score >= beta) ? TT_LOWER : ((inPv && best_score > old_alpha) ? TT_EXACT : TT_UPPER));
    return best_score;
}

void search_t::updateHistoryValues(int& sc, int delta) {
    sc += delta - (sc * abs(delta)) / 800;
}

void search_t::updateHistory(move_t bm, int depth) {
    if (killer1[ply] != bm.m) {
        killer2[ply] = killer1[ply];
        killer1[ply] = bm.m;
    }
    int cm_pc = pos.stack.movingpc, cm_to = pos.stack.dest, fm_pc = 0, fm_to = 0;
    countermove[pos.side ^ 1][cm_pc][cm_to] = bm.m;
    if (ply > 1) fm_pc = stack[ply - 1].movingpc, fm_to = stack[ply - 1].dest;
    int bonus = std::min(depth * depth, 400);
    for (move_t m : playedmoves[ply]) {
        int delta = (m.m == bm.m) ? bonus : -bonus;
        int from = m.moveFrom();
        int to = m.moveTo();
        int piece = pos.getPiece(from);
        updateHistoryValues(history[pos.side][from][to], delta);
        updateHistoryValues(cmh[cm_pc][cm_to][piece][to], delta);
        updateHistoryValues(fmh[fm_pc][fm_to][piece][to], delta);
    }
}

void search_t::getHistoryValues(int& h, int& ch, int& fh, move_t m) {
    int from = m.moveFrom();
    int to = m.moveTo();
    int piece = pos.getPiece(from);
    h = history[pos.side][from][to];
    ch = cmh[pos.stack.movingpc][pos.stack.dest][piece][to];
    fh = (ply > 1) ? fmh[stack[ply - 1].movingpc][stack[ply - 1].dest][piece][to] : 0;
}

void search_t::updateCapHistory(move_t bm, int depth) {
    int bonus = std::min(depth * depth, 400);
    for (move_t m : playedcaps[ply]) {
        int delta = (m.m == bm.m) ? bonus : -bonus;
        int to = m.moveTo();
        updateHistoryValues(caphistory[pos.getPiece(m.moveFrom())][pos.getPiece(to)][to], delta);
    }
}