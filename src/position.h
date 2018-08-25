/**************************************************/
/*  Invictus 2018						          */
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
		castle = 0;
		fifty = 0;
		epsq = -1;
		pliesfromnull = 0;
		phash = 0;
		hash = 0;
		capturedpc = EMPTY;
		score[WHITE] = { 0, 0 };
		score[BLACK] = { 0, 0 };
	}
	move_t lastmove;
	int32_t castle;
	int32_t fifty;
	int32_t epsq;
	int32_t capturedpc;
	int64_t pliesfromnull;
	score_t score[2];
	uint64_t phash;
	uint64_t hash;
};

struct position_t {
	void initPosition();
	void undoNullMove(undo_t& undo);
	void doNullMove(undo_t& undo);
	void undoMove(undo_t& undo);
	void doMove(undo_t& undo, move_t m);
	void setPiece(int ss, int c, int pc);
	void setPosition(const std::string& fenstr);
	std::string positionToFEN();
	std::string to_str();

	bool isRepeat();
	bool isMatDrawn();
	inline int getPiece(int sq);
	inline int getSide(int sq);
	uint64_t getPieceBB(int pc, int c);
	uint64_t getBishopSlidersBB(int c);
	uint64_t getRookSlidersBB(int c);
	bool colorIsDifferent(int sq, int c);

	uint64_t allAttackersToSquare(int sq, uint64_t occupied);
	bool statExEval(move_t m, int threshold);

	//movegen
	void genLegal(movelist_t& mvlist);
	void genQuietMoves(movelist_t& mvlist);
	void genTacticalMoves(movelist_t& mvlist);
	void genCheckEvasions(movelist_t& mvlist);
	bool areaIsAttacked(int c, uint64_t area);
	bool sqIsAttacked(uint64_t occ, int sq, int c);
	bool kingIsInCheck();
	uint64_t pieceAttacksFromBB(int pc, int sq, uint64_t occ);
	uint64_t getAttacksBB(int sq, int c);
	uint64_t pinnedPieces(int c);
	uint64_t discoveredCheckCandidates(int c);
	bool moveIsLegal(move_t move, uint64_t pinned, bool incheck);
	bool moveIsCheck(move_t m, uint64_t dcc);
	bool moveIsValid(move_t move, uint64_t pinned);
	bool moveIsTactical(move_t m);

	bool hashIsValid();
	bool phashIsValid();

	uint64_t occupiedBB;
	uint64_t history[1024];
	uint64_t piecesBB[7];
	uint64_t colorBB[2];
	int pieces[64];
	int kpos[2];
	undo_t stack;
	int side;
	int num_moves;
};
