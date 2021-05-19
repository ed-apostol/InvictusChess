/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <algorithm>
#include <cstring>

#include "typedefs.h"
#include "constants.h"
#include "position.h"
#include "utils.h"
#include "bitutils.h"
#include "attacks.h"
#include "log.h"
#include "eval.h"
#include "params.h"

namespace PositionData {
    const uint64_t  CastleSquareMask1[2][2] = { { F1BB | G1BB, B1BB | C1BB | D1BB }, { F8BB | G8BB, B8BB | C8BB | D8BB } };
    const uint64_t  CastleSquareMask2[2][2] = { { E1BB | F1BB | G1BB, C1BB | D1BB | E1BB }, { E8BB | F8BB | G8BB, C8BB | D8BB | E8BB } };
    const int CastleSquareFrom[2] = { E1, E8 };
    const int CastleSquareTo[2][2] = { { G1, C1 },{ G8, C8 } };
    const int RookFrom[2][2] = { { A1, H1 },{ A8, H8 } };
    const int RookTo[2][2] = { { D1, F1 },{ D8, F8 } };

    uint64_t DirBitmap[64][8];
    uint64_t InBetween[64][64];
    int DirFromTo[64][64];
    int CastleMask[64];

    uint64_t ZobPiece[2][8][64];
    uint64_t ZobCastle[16];
    uint64_t ZobEpsq[8];
    uint64_t ZobColor;

    static uint64_t xorshift128plus(void) {
        static uint64_t s[2] = { 4123659995ull, 9981545732273789042ull };
        uint64_t x = s[0];
        uint64_t const y = s[1];
        s[0] = y;
        x ^= x << 23;
        s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
        return s[1] + y;
    }

    void initArr(void) {
        const int kingd[8] = { -9, -1, 7, 8, 9, 1, -7, -8 };

        memset(ZobPiece, 0, sizeof(ZobPiece));
        memset(ZobCastle, 0, sizeof(ZobCastle));
        memset(ZobEpsq, 0, sizeof(ZobEpsq));

        for (int side = 0; side < 2; ++side) {
            for (int pc = 1; pc <= 6; ++pc) {
                for (int sq = 0; sq < 64; ++sq) {
                    ZobPiece[side][pc][sq] = xorshift128plus();
                }
            }
        }
        for (int sq = 1; sq < 16; ++sq) ZobCastle[sq] = xorshift128plus();
        for (int sq = 0; sq < 8; ++sq) ZobEpsq[sq] = xorshift128plus();
        ZobColor = xorshift128plus();

        for (int sq = 0; sq < 64; sq++) CastleMask[sq] = 0xF;

        CastleMask[E1] &= ~WCKS;
        CastleMask[H1] &= ~WCKS;
        CastleMask[E1] &= ~WCQS;
        CastleMask[A1] &= ~WCQS;
        CastleMask[E8] &= ~BCKS;
        CastleMask[H8] &= ~BCKS;
        CastleMask[E8] &= ~BCQS;
        CastleMask[A8] &= ~BCQS;

        for (int sq = 0; sq < 64; ++sq) {
            for (int sq2 = 0; sq2 < 64; ++sq2) DirFromTo[sq][sq2] = 8;
            for (int k = 0; k < 8; ++k) {
                DirBitmap[sq][k] = 0;
                for (int p = sq, n = sq + kingd[k]; n >= 0 && n < 64 && abs((n & 7) - (p & 7)) < 2; p = n, n += kingd[k]) {
                    DirFromTo[sq][n] = k;
                    DirBitmap[sq][k] |= BitMask[n];
                }
            }
        }
        for (int sq = 0; sq < 64; ++sq) {
            for (int sq2 = 0; sq2 < 64; ++sq2) {
                InBetween[sq][sq2] = uint64_t(0);
                int k = DirFromTo[sq][sq2];
                if (k != 8) {
                    int k2 = DirFromTo[sq2][sq];
                    InBetween[sq][sq2] = DirBitmap[sq][k] & DirBitmap[sq2][k2];
                }
            }
        }
    }
}

using namespace Attacks;
using namespace BitUtils;
using namespace PositionData;
using namespace EvalParam;

void position_t::initPosition() {
    for (auto& p : piecesBB) p = EmptyBoardBB;
    for (auto& c : colorBB) c = EmptyBoardBB;
    for (auto& pc : pieces) pc = EMPTY;
    occupiedBB = EmptyBoardBB;
    kpos[0] = kpos[1] = A1;
    mat_idx[0] = mat_idx[1] = 0;
    side = WHITE;
    history.clear();
    stack.init();
}

void position_t::undoNullMove(undo_t& undo) {
    side ^= 1;
    stack = undo;
}

void position_t::doNullMove(undo_t& undo) {
    undo = stack;

    stack.lastmove.m = 0;
    stack.epsq = -1;
    stack.fifty += 1;
    stack.pliesfromnull = 0;

    if (undo.epsq != -1) stack.hash ^= ZobEpsq[sqFile(undo.epsq)];

    stack.hash ^= ZobColor;
    side ^= 1;
}

void position_t::undoMove(undo_t& undo) {
    move_t m = stack.lastmove;
    const int xside = side;
    const int from = m.moveFrom();
    const int to = m.moveTo();
    const int pc = pieces[to];

    side = xside ^ 1;

    pieces[from] = pc;
    piecesBB[pc] ^= BitMask[from] | BitMask[to];
    colorBB[side] ^= BitMask[from] | BitMask[to];

    const int cap = stack.capturedpc;
    pieces[to] = cap;
    if (cap != EMPTY) {
        piecesBB[cap] ^= BitMask[to];
        colorBB[xside] ^= BitMask[to];
        mat_idx[xside] += MatMul[cap];
    }
    if (pc == KING) kpos[side] = from;

    switch (m.moveFlags()) {
    case MF_CASTLE: {
        int rook_from = RookFrom[to / 56][(to % 8) > 5];
        int rook_to = RookTo[to / 56][(to % 8) > 5];
        pieces[rook_from] = ROOK;
        pieces[rook_to] = EMPTY;
        piecesBB[ROOK] ^= BitMask[rook_from] | BitMask[rook_to];
        colorBB[side] ^= BitMask[rook_from] | BitMask[rook_to];
    } break;
    case MF_ENPASSANT: {
        int epsq = (sqRank(from) << 3) + sqFile(to);
        pieces[epsq] = PAWN;
        mat_idx[xside] += MatMul[PAWN];
        piecesBB[PAWN] ^= BitMask[epsq];
        colorBB[xside] ^= BitMask[epsq];
    } break;
    case MF_PROMQ: case MF_PROMR: case MF_PROMB: case MF_PROMN: {
        int prom = m.movePromote();
        pieces[from] = PAWN;
        mat_idx[side] += MatMul[PAWN];
        mat_idx[side] -= MatMul[prom];
        piecesBB[PAWN] ^= BitMask[from];
        piecesBB[prom] ^= BitMask[from];
    } break;
    }
    occupiedBB = colorBB[side] | colorBB[xside];
    stack = undo;
    history.pop_back();
}

void position_t::doMove(undo_t& undo, move_t m) {
    const int from = m.moveFrom();
    const int to = m.moveTo();
    const int xside = side ^ 1;

    ASSERT(pieces[to] != KING);

    undo = stack;

    if (undo.epsq != -1) stack.hash ^= ZobEpsq[sqFile(undo.epsq)];
    stack.hash ^= ZobCastle[undo.castle];

    stack.lastmove = m;
    stack.epsq = -1;
    stack.castle &= CastleMask[from] & CastleMask[to];
    stack.hash ^= ZobCastle[stack.castle];
    stack.fifty += 1;
    stack.pliesfromnull += 1;
    stack.capturedpc = pieces[to];
    stack.hash ^= ZobColor;

    const int pc = pieces[from];
    pieces[to] = pieces[from];
    pieces[from] = EMPTY;
    piecesBB[pc] ^= BitMask[from] | BitMask[to];
    colorBB[side] ^= BitMask[from] | BitMask[to];
    stack.score[side] += PcSqTab[side][pc][to];
    stack.score[side] -= PcSqTab[side][pc][from];
    stack.hash ^= ZobPiece[side][pc][from] ^ ZobPiece[side][pc][to];
    if (pc == PAWN) {
        stack.phash ^= ZobPiece[side][pc][from] ^ ZobPiece[side][pc][to];
        stack.fifty = 0;
    }
    if (pc == KING) kpos[side] = to;

    const int cap = stack.capturedpc;
    if (cap != EMPTY) {
        piecesBB[cap] ^= BitMask[to];
        colorBB[xside] ^= BitMask[to];
        stack.score[xside] -= PcSqTab[xside][cap][to];
        stack.fifty = 0;
        stack.hash ^= ZobPiece[xside][cap][to];
        if (cap == PAWN)
            stack.phash ^= ZobPiece[xside][cap][to];
        mat_idx[xside] -= MatMul[cap];
    }
    switch (m.moveFlags()) {
    case MF_PAWN2: {
        stack.epsq = (from + to) / 2;
        if (piecesBB[PAWN] & colorBB[xside] & pawnAttacksBB(stack.epsq, side))
            stack.hash ^= ZobEpsq[sqFile(stack.epsq)];
        else
            stack.epsq = -1;
        stack.fifty = 0;
    } break;
    case MF_CASTLE: {
        const int rook_from = RookFrom[to / 56][(to % 8) > 5];
        const int rook_to = RookTo[to / 56][(to % 8) > 5];
        pieces[rook_to] = ROOK;
        pieces[rook_from] = EMPTY;
        piecesBB[ROOK] ^= BitMask[rook_from] | BitMask[rook_to];
        colorBB[side] ^= BitMask[rook_from] | BitMask[rook_to];
        stack.score[side] += PcSqTab[side][ROOK][rook_to];
        stack.score[side] -= PcSqTab[side][ROOK][rook_from];
        stack.hash ^= ZobPiece[side][ROOK][rook_from] ^ ZobPiece[side][ROOK][rook_to];
    } break;
    case MF_ENPASSANT: {
        const int epsq = (sqRank(from) << 3) + sqFile(to);
        pieces[epsq] = EMPTY;
        mat_idx[xside] -= MatMul[PAWN];
        piecesBB[PAWN] ^= BitMask[epsq];
        colorBB[xside] ^= BitMask[epsq];
        stack.score[xside] -= PcSqTab[xside][PAWN][epsq];
        stack.hash ^= ZobPiece[xside][PAWN][epsq];
        stack.phash ^= ZobPiece[xside][PAWN][epsq];
        stack.fifty = 0;
    } break;
    case MF_PROMQ: case MF_PROMR: case MF_PROMB: case MF_PROMN: {
        const int prom = m.movePromote();
        piecesBB[PAWN] ^= BitMask[to]; // remove pawn
        stack.hash ^= ZobPiece[side][PAWN][to];
        stack.phash ^= ZobPiece[side][PAWN][to];
        pieces[to] = prom;
        mat_idx[side] -= MatMul[PAWN];
        mat_idx[side] += MatMul[prom];
        piecesBB[prom] ^= BitMask[to];
        stack.score[side] += PcSqTab[side][prom][to];
        stack.score[side] -= PcSqTab[side][PAWN][to];
        stack.hash ^= ZobPiece[side][prom][to];
        stack.fifty = 0;
    } break;
    }
    occupiedBB = colorBB[side] | colorBB[xside];
    side = xside;
    history.push_back(stack.hash);

    //ASSERT(hashIsValid());
    //ASSERT(phashIsValid());
}

void position_t::setPiece(int sq, int c, int pc) {
    pieces[sq] = pc;
    piecesBB[pc] |= BitMask[sq];
    colorBB[c] |= BitMask[sq];
    occupiedBB |= BitMask[sq];
    stack.score[c] += PcSqTab[c][pc][sq];
    stack.hash ^= ZobPiece[c][pc][sq];
    if (pc == PAWN) stack.phash ^= ZobPiece[c][pc][sq];
    if (pc == KING) kpos[c] = sq;
    mat_idx[c] += MatMul[pc];
}

void position_t::setPosition(const std::string& fenStr) {
    static const std::string pc2char("PNBRQKpnbrqk");
    static const std::vector<int> char2piece = { PAWN,KNIGHT,BISHOP,ROOK,QUEEN,KING };
    char col, row, token;
    size_t p;
    int sq = A8;
    std::istringstream ss(fenStr);
    initPosition();
    ss >> std::noskipws;
    while ((ss >> token) && !isspace(token)) {
        if (isdigit(token)) sq += (token - '0');
        else if (token == '/') sq -= 16;
        else if ((p = pc2char.find(token)) != std::string::npos)
            setPiece(sq++, (islower(token) ? BLACK : WHITE), char2piece[p % 6]);
    }
    ss >> token;
    side = (token == 'w' ? WHITE : BLACK);
    ss >> token;
    while ((ss >> token) && !isspace(token)) {
        if (token == 'K') stack.castle |= WCKS;
        else if (token == 'Q') stack.castle |= WCQS;
        else if (token == 'k') stack.castle |= BCKS;
        else if (token == 'q') stack.castle |= BCQS;
    }
    if (((ss >> col) && (col >= 'a' && col <= 'h')) && ((ss >> row) && (row == '3' || row == '6')))
        stack.epsq = (8 * (row - '1')) + (col - 'a');
    ss >> std::skipws >> stack.fifty;
    if (stack.epsq != -1) stack.hash ^= ZobEpsq[sqFile(stack.epsq)];
    if (side == WHITE) stack.hash ^= ZobColor;
    stack.hash ^= ZobCastle[stack.castle];

    //ASSERT(hashIsValid());
    //ASSERT(phashIsValid());
}

std::string position_t::positionToFEN() {
    static const std::string pcstr = ".PNBRQK.pnbrqk";
    std::string fen;
    for (int rank = 56; rank >= 0; rank -= 8) {
        int empty = 0;
        for (int file = FileA; file <= FileH; file++) {
            if (getPiece(rank + file) != EMPTY) {
                if (empty > 0) fen += (char)empty + '0';
                fen += pcstr[getPiece(rank + file) + (getSide(rank + file) == WHITE ? 0 : 7)];
                empty = 0;
            }
            else empty++;
        }
        if (empty > 0) fen += (char)empty + '0';
        fen += (rank > Rank1) ? '/' : ' ';
    }
    fen += (side == WHITE) ? 'w' : 'b';
    fen += ' ';
    if (stack.castle == 0) fen += '-';
    if (stack.castle & WCKS) fen += 'K';
    if (stack.castle & WCQS) fen += 'Q';
    if (stack.castle & BCKS) fen += 'k';
    if (stack.castle & BCQS) fen += 'q';
    fen += ' ';
    if (stack.epsq == -1) fen += '-';
    else {
        fen += sqFile(stack.epsq) + 'a';
        fen += '1' + sqRank(stack.epsq);
    }
    fen += " " + std::to_string(stack.fifty) + " " + std::to_string(history.size() + 1);
    return fen;
}

std::string position_t::to_str() {
    std::string str;
    static const std::string piecestr = " PNBRQK pnbrqk";
    static const std::string board = "\n  +---+---+---+---+---+---+---+---+\n";
    str += positionToFEN() + "\n";
    str += "incheck: " + std::to_string(kingIsInCheck());
    str += " capt: " + std::to_string(stack.capturedpc);
    str += " lastmove: " + stack.lastmove.to_str() + "\n";
    str += board;
    for (int j = 7; j >= 0; --j) {
        str += std::to_string(j + 1);
        str += " | ";
        for (int i = 0; i <= 7; ++i) {
            int sq = (j * 8) + i;
            int pc = pieces[sq] + (getSide(sq) ? 7 : 0);
            str += piecestr[pc];
            str += " | ";
        }
        str += board;
    }
    str += "    a   b   c   d   e   f   g   h \n";
    return str;
}

bool position_t::isRepeat() {
    int idx_limit = std::max(history.size() - stack.fifty, history.size() - stack.pliesfromnull);
    idx_limit = std::max(idx_limit, 0);
    for (int idx = history.size() - 5; idx >= idx_limit; idx -= 2) {
        if (history[idx] == stack.hash)
            return true;
    }
    return false;
}

bool position_t::isMatIdxValid() {
#ifdef TUNE
    return false;
#else
    return mat_idx[0] < 486 && mat_idx[1] < 486;
#endif
}

bool position_t::isMatDrawn() {
    if (isMatIdxValid() && getMaterial(mat_idx[0], mat_idx[1]).flags & 1) return true;
    return false;
}

int position_t::getPiece(int sq) {
    return pieces[sq];
}

int position_t::getSide(int sq) {
    return (BitMask[sq] & colorBB[WHITE]) ? WHITE : BLACK;
}

uint64_t  position_t::getPieceBB(int pc, int c) {
    return piecesBB[pc] & colorBB[c];
}

uint64_t position_t::bishopSlidersBB(int c) {
    return (piecesBB[QUEEN] | piecesBB[BISHOP]) & colorBB[c];
}
uint64_t position_t::rookSlidersBB(int c) {
    return (piecesBB[QUEEN] | piecesBB[ROOK]) & colorBB[c];
}

uint64_t position_t::allAttackersToSqBB(int sq, uint64_t occupied) {
    return
        (colorBB[WHITE] & pawnAttacksBB(sq, BLACK) & piecesBB[PAWN]) |
        (colorBB[BLACK] & pawnAttacksBB(sq, WHITE) & piecesBB[PAWN]) |
        (knightMovesBB(sq) & piecesBB[KNIGHT]) |
        (kingMovesBB(sq) & piecesBB[KING]) |
        (bishopAttacksBB(sq, occupied) & (piecesBB[BISHOP] | piecesBB[QUEEN])) |
        (rookAttacksBB(sq, occupied) & (piecesBB[ROOK] | piecesBB[QUEEN]));
}

uint64_t position_t::safeSqsBB(int c, uint64_t occ, uint64_t target) {
    uint64_t bits = target;
    while (target) {
        int sq = popFirstBit(target);
        if (sqIsAttacked(occ, sq, c))
            bits ^= BitMask[sq];
    }
    return bits;
}

uint64_t position_t::pieceAttacksFromBB(int pc, int sq, uint64_t occ) {
    switch (pc) {
    case PAWN: return pawnAttacksBB(sq, side);
    case KNIGHT: return knightMovesBB(sq);
    case BISHOP: return bishopAttacksBB(sq, occ);
    case ROOK: return rookAttacksBB(sq, occ);
    case QUEEN: return queenAttacksBB(sq, occ);
    case KING: return kingMovesBB(sq);
    }
    return 0;
}

uint64_t position_t::getAttacksBB(int sq, int c) {
    return colorBB[c] & ((pawnAttacksBB(sq, c ^ 1) & piecesBB[PAWN]) |
        (knightMovesBB(sq) & piecesBB[KNIGHT]) |
        (kingMovesBB(sq) & piecesBB[KING]) |
        (bishopAttacksBB(sq, occupiedBB) & (piecesBB[BISHOP] | piecesBB[QUEEN])) |
        (rookAttacksBB(sq, occupiedBB) & (piecesBB[ROOK] | piecesBB[QUEEN])));
}

uint64_t position_t::pinnedPiecesBB(int c) {
    uint64_t pinned = 0;
    const int ksq = kpos[c];

    uint64_t pinners = rookSlidersBB(c ^ 1);
    pinners &= pinners ? rookAttacksBBX(ksq, occupiedBB) : 0;
    while (pinners) pinned |= InBetween[popFirstBit(pinners)][ksq] & colorBB[c];

    pinners = bishopSlidersBB(c ^ 1);
    pinners &= pinners ? bishopAttacksBBX(ksq, occupiedBB) : 0;
    while (pinners) pinned |= InBetween[popFirstBit(pinners)][ksq] & colorBB[c];

    return pinned;
}

uint64_t position_t::discoveredPiecesBB(int c) {
    uint64_t pinned = 0;
    const int ksq = kpos[c ^ 1];

    uint64_t pinners = rookSlidersBB(c);
    pinners &= pinners ? rookAttacksBBX(ksq, occupiedBB) : 0;
    while (pinners) pinned |= InBetween[popFirstBit(pinners)][ksq] & colorBB[c] & ~rookSlidersBB(c);

    pinners = bishopSlidersBB(c);
    pinners &= pinners ? bishopAttacksBBX(ksq, occupiedBB) : 0;
    while (pinners) pinned |= InBetween[popFirstBit(pinners)][ksq] & colorBB[c] & ~bishopSlidersBB(c);

    return pinned;
}

bool position_t::staticExchangeEval(move_t m, int threshold) {
    static const int StatExEvPcVals[] = { 0, 100,  450,  450,  675, 1300, 0 };
    const int from = m.moveFrom();
    const int to = m.moveTo();
    const int prom = m.movePromote();
    const bool isEnPassant = m.isEnPassant();

    int piece = prom ? prom : pieces[from];
    int currval = StatExEvPcVals[pieces[to]] - threshold;
    if (prom) currval += StatExEvPcVals[prom] - StatExEvPcVals[PAWN];
    else if (isEnPassant) currval += StatExEvPcVals[PAWN];
    if (currval < 0) return false;
    currval -= StatExEvPcVals[piece];
    if (currval >= 0) return true;

    const uint64_t bishops = piecesBB[BISHOP] | piecesBB[QUEEN];
    const uint64_t rooks = piecesBB[ROOK] | piecesBB[QUEEN];
    uint64_t occupied = (occupiedBB ^ BitMask[from]) | BitMask[to];
    if (isEnPassant) occupied ^= BitMask[stack.epsq];
    uint64_t allAttackers = allAttackersToSqBB(to, occupied) & occupied;
    int color = side ^ 1;
    for (uint64_t mask, attackers; attackers = allAttackers & colorBB[color];) {
        for (piece = PAWN; !(mask = attackers & piecesBB[piece]); ++piece);
        occupied ^= BitMask[getFirstBit(mask)];
        if (piece == PAWN || piece == BISHOP || piece == QUEEN)
            allAttackers |= bishopAttacksBB(to, occupied) & bishops;
        if (piece == ROOK || piece == QUEEN)
            allAttackers |= rookAttacksBB(to, occupied) & rooks;
        allAttackers &= occupied;
        color ^= 1;
        currval = -currval - 1 - StatExEvPcVals[piece];
        if (currval >= 0) {
            if (piece == KING && (allAttackers & colorBB[color])) color ^= 1;
            break;
        }
    }
    return side != color;
}

bool position_t::canCastleKS(int s) {
    return stack.castle & (s ? BCKS : WCKS);
}
bool position_t::canCastleQS(int s) {
    return stack.castle & (s ? BCQS : WCQS);
}

bool position_t::kingIsInCheck() {
    return sqIsAttacked(occupiedBB, kpos[side], side ^ 1);
}

bool position_t::areaIsAttacked(int c, uint64_t target) {
    while (target)
        if (sqIsAttacked(occupiedBB, popFirstBit(target), c))
            return true;
    return false;
}

bool position_t::sqIsAttacked(uint64_t occ, int sq, int c) {
    return
        (rookAttacksBB(sq, occ) & rookSlidersBB(c)) ||
        (bishopAttacksBB(sq, occ) & bishopSlidersBB(c)) ||
        (knightMovesBB(sq) & piecesBB[KNIGHT] & colorBB[c]) ||
        ((pawnAttacksBB(sq, c ^ 1) & piecesBB[PAWN] & colorBB[c])) ||
        (kingMovesBB(sq) & piecesBB[KING] & colorBB[c]);
}

bool position_t::moveIsLegal(move_t move, uint64_t pinned, bool incheck) {
    if (incheck) return true;

    const int xside = side ^ 1;
    const int from = move.moveFrom();
    const int to = move.moveTo();
    const int ksq = kpos[side];

    if (move.isEnPassant()) {
        uint64_t b = occupiedBB ^ BitMask[from] ^ BitMask[(sqRank(from) << 3) + sqFile(to)] ^ BitMask[to];
        return !(rookAttacksBB(ksq, b) & rookSlidersBB(xside)) && !(bishopAttacksBB(ksq, b) & bishopSlidersBB(xside));
    }
    if (move.isCastle()) {
        return !areaIsAttacked(xside, CastleSquareMask2[side][to > from ? 0 : 1]);
    }
    if (from == ksq) return !(sqIsAttacked(occupiedBB ^ BitMask[ksq], to, xside));
    if (!(pinned & BitMask[from])) return true;
    if (DirFromTo[from][ksq] == DirFromTo[to][ksq]) return true;
    return false;
}

bool position_t::moveIsCheck(move_t move, uint64_t dcc) {
    const int xside = side ^ 1;
    const int from = move.moveFrom();
    const int to = move.moveTo();
    const int enemy_ksq = kpos[xside];
    const int pc = pieces[from];
    const int prom = move.movePromote();

    if ((dcc & BitMask[from]) && DirFromTo[from][enemy_ksq] != DirFromTo[to][enemy_ksq]) return true;
    if (pc != PAWN && pc != KING && pieceAttacksFromBB(pc, enemy_ksq, occupiedBB)  & BitMask[to]) return true;
    if (pc == PAWN && pawnAttacksBB(enemy_ksq, xside) & BitMask[to]) return true;
    uint64_t tempOccBB = occupiedBB ^ BitMask[from] ^ BitMask[to];
    if (move.isPromote() && pieceAttacksFromBB(prom, enemy_ksq, tempOccBB)  & BitMask[to]) return true;
    if (move.isEnPassant() && sqIsAttacked(tempOccBB ^ BitMask[(sqRank(from) << 3) + sqFile(to)], enemy_ksq, side)) return true;
    if (move.isCastle() && rookAttacksBB(enemy_ksq, tempOccBB) & BitMask[RookTo[to / 56][(to % 8) > 5]]) return true;
    return false;
}

bool position_t::moveIsValid(move_t m, uint64_t pinned) {
    const int from = m.moveFrom();
    const int to = m.moveTo();
    const int pc = pieces[from];
    const int cap = pieces[to];
    const int absdiff = abs(from - to);
    const int flag = m.moveFlags();
    const int prom = m.movePromote();

    if (pc == EMPTY) return false;
    if (cap == KING) return false;
    if (prom != EMPTY && (prom < KNIGHT || prom > QUEEN)) return false;
    if (getSide(from) != side) return false;
    if (cap != EMPTY && getSide(to) == side) return false;
    if ((pinned & BitMask[from]) && (DirFromTo[from][kpos[side]] != DirFromTo[to][kpos[side]])) return false;
    if (pc != PAWN && (pc != KING || absdiff != 2) && !(pieceAttacksFromBB(pc, from, occupiedBB) & BitMask[to])) return false;
    if (pc == KING) {
        if (BitMask[to] & kingMovesBB(kpos[side ^ 1])) return false;
        if (absdiff == 2 && flag != MF_CASTLE) return false;
    }
    if (pc == PAWN) {
        if (cap != EMPTY && !(pawnAttacksBB(from, side) & BitMask[to])) return false;
        if (cap == EMPTY && to != stack.epsq && !((pawnMovesBB(from, side) | pawnMoves2BB(from, side)) & BitMask[to])) return false;
        if (absdiff == 16 && flag != MF_PAWN2) return false;
        if (to == stack.epsq && flag != MF_ENPASSANT) return false;
    }
    switch (flag) {
    case MF_PAWN2: {
        if (pc != PAWN) return false;
        if (!(Rank2ByColorBB[side] & BitMask[from])) return false;
        if (absdiff != 16) return false;
        if (cap != EMPTY) return false;
        if (pieces[(from + to) / 2] != EMPTY) return false;
    } break;
    case MF_ENPASSANT: {
        if (pc != PAWN) return false;
        if (cap != EMPTY) return false;
        if (to != stack.epsq) return false;
        if (pieces[(sqRank(from) << 3) + sqFile(to)] != PAWN) return false;
        if (absdiff != 9 && absdiff != 7) return false;
    } break;
    case MF_CASTLE: {
        if (pc != KING) return false;
        if (absdiff != 2) return false;
        if (from != E1 && from != E8) return false;
        if (pieces[RookFrom[to / 56][(to % 8) > 5]] != ROOK) return false;
        if (to > from) {
            if (!canCastleKS(side)) return false;
            if (occupiedBB & CastleSquareMask1[side][0]) return false;
            if (areaIsAttacked(side ^ 1, CastleSquareMask2[side][0])) return false;
        }
        if (to < from) {
            if (!canCastleQS(side)) return false;
            if (occupiedBB & CastleSquareMask1[side][1]) return false;
            if (areaIsAttacked(side ^ 1, CastleSquareMask2[side][1])) return false;
        }
    } break;
    case MF_PROMN: case MF_PROMB: case MF_PROMR: case MF_PROMQ: {
        if (pc != PAWN) return false;
        if (prom == EMPTY) return false;
        if (!(Rank7ByColorBB[side] & BitMask[from])) return false;
    } break;
    }
    return true;
}

bool position_t::moveIsTactical(move_t m) {
    return pieces[m.moveTo()] != EMPTY || m.isPromote() || m.isEnPassant();
}

// TEST UTILS
bool position_t::hashIsValid() {
    uint64_t hash = 0;
    for (int sq = A1; sq <= H8; ++sq) {
        if (getPiece(sq) != EMPTY) hash ^= ZobPiece[getSide(sq)][getPiece(sq)][sq];
    }
    if (stack.epsq != -1) hash ^= ZobEpsq[sqFile(stack.epsq)];
    if (side == WHITE) hash ^= ZobColor;
    hash ^= ZobCastle[stack.castle];
    return stack.hash == hash;
}

bool position_t::phashIsValid() {
    uint64_t hash = 0;
    for (int sq = A1; sq <= H8; ++sq) {
        if (getPiece(sq) == PAWN)
            hash ^= ZobPiece[getSide(sq)][PAWN][sq];
    }
    return stack.phash == hash;
}