/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

namespace PositionData {
    extern uint64_t InBetween[64][64];
    extern int DirFromTo[64][64];
    extern const uint64_t CastleSquareMask1[2][2];
    extern const uint64_t CastleSquareMask2[2][2];
    extern const int CastleSquareFrom[2];
    extern const int CastleSquareTo[2][2];
    extern const int RookFrom[2][2];
    extern const int RookTo[2][2];
    extern void initArr(void);
}

struct undo_t {
    void init() {
        lastmove = 0;
        castle = 0;
        fifty = 0;
        epsq = -1;
        pliesfromnull = 0;
        phash = 0;
        hash = 0;
        capturedpc = EMPTY;
        movingpc = EMPTY;
        dest = 0;
        score[WHITE] = { 0, 0 };
        score[BLACK] = { 0, 0 };
    }
    move_t lastmove;
    int32_t castle;
    int32_t fifty;
    int32_t epsq;
    int32_t capturedpc;
    int32_t movingpc;
    int32_t dest;
    int32_t pliesfromnull;
    score_t score[2];
    uint64_t phash;
    uint64_t hash;
};

struct position_t {
    position_t() {}
    position_t(const std::string & fen) { setPosition(fen); }
    void initPosition();
    void undoNullMove(undo_t& undo, int& ply);
    void doNullMove(undo_t& undo, int& ply);
    void undoMove(undo_t& undo, int& ply);
    void doMove(undo_t& undo, move_t m, int& ply);
    void setPiece(bool update, int sq, int c, int pc);
    void removePiece(bool update, int sq, int c, int pc);
    void setPosition(const std::string& fenstr);
    std::string positionToFEN();
    std::string to_str();

    bool isRepeat();
    bool isMatIdxValid();
    bool isMatDrawn();
    int getPiece(int sq);
    int getSide(int sq);

    uint64_t getPieceBB(int pc, int c);
    uint64_t bishopSlidersBB(int c);
    uint64_t rookSlidersBB(int c);
    uint64_t allAttackersToSqBB(int sq, uint64_t occupied);
    uint64_t safeSqsBB(int c, uint64_t occ, uint64_t target);
    uint64_t pieceAttacksFromBB(int pc, int sq, uint64_t occ);
    uint64_t getAttacksBB(int sq, int c);
    uint64_t pinnedPiecesBB(int c);
    uint64_t discoveredPiecesBB(int c);

    bool staticExchangeEval(move_t m, int threshold);
    bool canCastleKS(int s);
    bool canCastleQS(int s);
    bool kingIsInCheck();
    bool areaIsAttacked(int c, uint64_t target);
    bool sqIsAttacked(uint64_t occ, int sq, int c);
    bool moveIsLegal(move_t move, uint64_t pinned, bool incheck);
    bool moveIsCheck(move_t m, uint64_t dcc);
    bool moveIsValid(move_t move, uint64_t pinned);
    bool moveIsTactical(move_t m);

    bool hashIsValid();
    bool phashIsValid();

    void genLegal(movelist_t<256>& mvlist);
    void genQuietMoves(movelist_t<256>& mvlist);
    void genTacticalMoves(movelist_t<256>& mvlist);
    void genCheckEvasions(movelist_t<256>& mvlist);

    uint64_t occupiedBB;
    std::vector<uint64_t> history;
    uint64_t piecesBB[7];
    uint64_t colorBB[2];
    int pieces[64];
    int kpos[2];
    int mat_idx[2];
    undo_t stack;
    int side;
};
