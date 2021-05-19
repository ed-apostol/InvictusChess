/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <algorithm>
#include "movepicker.h"
#include "log.h"

movepicker_t::movepicker_t(search_t& search, bool inCheck, bool inQS, int marg, uint16_t hmove, uint16_t k1, uint16_t k2, uint16_t cm)
    : s(search), pos(s.pos), idx(0), hashmove(hmove), killer1(k1), killer2(k2), counter(cm), inQSearch(inQS), margin(marg) {
    pinned = pos.pinnedPiecesBB(pos.side);
    if (inCheck) {
        pos.genCheckEvasions(mvlist);
        scoreEvasions();
        stage = STG_EVASION;
    }
    else stage = STG_HTABLE;
}

move_t movepicker_t::getBestMoveFromIdx(int idx) {
    int best_idx = idx;
    for (int i = idx + 1; i < mvlist.size; ++i) {
        if (mvlist.mv(i).s > mvlist.mv(best_idx).s)
            best_idx = i;
    }
    if (best_idx != idx)
        std::swap(mvlist.mv(best_idx), mvlist.mv(idx));
    return mvlist.mv(idx);
}

bool movepicker_t::getMoves(move_t& move, bool skipquiets) {
    switch (stage) {
    case STG_EVASION:
        if (idx < mvlist.size) {
            move = getBestMoveFromIdx(idx++);
            return true;
        }
        else return false;
        break;
    case STG_HTABLE:
        ++stage;
        if (hashmove != 0) {
            move.m = hashmove;
            if (pos.moveIsValid(move, pinned) && pos.moveIsLegal(move, pinned, false))
                return true;
            else {
                LogAndPrintOutput() << "Hashmove not valid!";
                LogAndPrintOutput() << move.to_str();
                LogAndPrintOutput() << pos.to_str();
            }
        }
    case STG_GENTACTICS:
        pos.genTacticalMoves(mvlist);
        scoreTactical();
        ++stage;
    case STG_WINTACTICS:
        while (idx < mvlist.size) {
            move = getBestMoveFromIdx(idx++);
            if (move.m == hashmove) continue;
            if (!pos.staticExchangeEval(move, margin)) {
                if (!inQSearch)
                    mvlistbad.add(move);
                continue;
            }
            if (pos.moveIsLegal(move, pinned, false))
                return true;
        }
        if (inQSearch) return false;
        ++stage;
    case STG_KILLER1:
        ++stage;
        if (!skipquiets && killer1 != 0 && killer1 != hashmove) {
            move.m = killer1;
            if (pos.moveIsValid(move, pinned) && pos.moveIsLegal(move, pinned, false))
                return true;
        }
    case STG_KILLER2:
        ++stage;
        if (!skipquiets  && killer2 != 0 && killer2 != hashmove) {
            move.m = killer2;
            if (pos.moveIsValid(move, pinned) && pos.moveIsLegal(move, pinned, false))
                return true;
        }
    case STG_COUNTER:
        ++stage;
        if (!skipquiets && counter != 0 && counter != hashmove && counter != killer1 && counter != killer2) {
            move.m = counter;
            if (pos.moveIsValid(move, pinned) && pos.moveIsLegal(move, pinned, false))
                return true;
        }
    case STG_GENQUIET:
        if (!skipquiets) {
            pos.genQuietMoves(mvlist);
            scoreNonTactical();
        }
        ++stage;
    case STG_QUIET:
        if (!skipquiets) {
            while (idx < mvlist.size) {
                move = getBestMoveFromIdx(idx++);
                if (move.m == hashmove) continue;
                if (move.m == killer1) continue;
                if (move.m == killer2) continue;
                if (move.m == counter) continue;
                if (pos.moveIsLegal(move, pinned, false))
                    return true;
            }
        }
        ++stage;
        idx = 0;
    case STG_BADTACTICS:
        while (idx < mvlistbad.size) { // no need to order, ordered already
            move = mvlistbad.mv(idx++);
            if (move.m == hashmove) continue;
            if (pos.moveIsLegal(move, pinned, false))
                return true;
        }
        ++stage;
        idx = 0;
        //if (deferred.size) PrintOutput() << "deferred size: " << deferred.size;
    case STG_DEFERRED:
        while (idx < deferred.size) {
            move = deferred.mv(idx++);
            if (skipquiets && !pos.moveIsTactical(move)) continue;
            return true;
        }
        ++stage;
        break;
    case STG_DONE:
        return false;
    }
    return false;
}

void movepicker_t::scoreTactical() {
    for (move_t& m : mvlist) {
        m.s = (pos.pieces[m.moveTo()] * 6) + m.movePromote() - pos.pieces[m.moveFrom()];
    }
}

void movepicker_t::scoreNonTactical() {
    for (move_t& m : mvlist) {
        m.s = s.history[pos.side][m.moveFrom()][m.moveTo()];
        //PrintOutput() << m.to_str() << " " << m.s;
    }
}

void movepicker_t::scoreEvasions() {
    for (move_t& m : mvlist) {
        if (m.m == hashmove) m.s = 10000;
        else if (m.m == killer1) m.s = 5000;
        else if (m.m == killer2) m.s = 4999;
        else if (m.m == counter) m.s = 4998;
        else m.s = 7000 + (pos.pieces[m.moveTo()] * 10) + m.movePromote() - pos.pieces[m.moveFrom()];
    }
}