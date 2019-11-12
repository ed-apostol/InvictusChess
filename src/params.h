/**************************************************/
/*  Invictus 2019                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include "typedefs.h"

namespace EvalParam {
	extern score_t MaterialValues[7];
	extern score_t BishopPair;

	extern score_t PawnConnected;
	extern score_t PawnDoubled;
	extern score_t PawnIsolated;
	extern score_t PawnBackward;
	extern score_t PawnIsolatedOpen;
	extern score_t PawnBackwardOpen;

	extern score_t PasserBonusMin;
	extern score_t PasserBonusMax;
	extern score_t PasserDistOwn;
	extern score_t PasserDistEnemy;
	extern score_t PasserNotBlocked;
	extern score_t PasserSafePush;
	extern score_t PasserSafeProm;

	extern score_t KnightMob;
	extern score_t BishopMob;
	extern score_t RookMob;
	extern score_t QueenMob;
	extern score_t RookOn7th;
	extern score_t RookOnSemiOpenFile;
	extern score_t RookOnOpenFile;
	extern score_t OutpostBonus;
	extern score_t BishopPawns;
	extern basic_score_t Tempo;

	extern score_t PawnPush;
	extern score_t WeakPawns;
	extern score_t PawnsxMinors;
	extern score_t MinorsxMinors;
	extern score_t MajorsxWeakMinors;
	extern score_t PawnsMinorsxMajors;
	extern score_t AllxQueens;

	extern score_t KingShelter1;
	extern score_t KingShelter2;
	extern score_t KingStorm1;
	extern score_t KingStorm2;

	extern basic_score_t KnightAtk;
	extern basic_score_t BishopAtk;
	extern basic_score_t RookAtk;
	extern basic_score_t QueenAtk;
	extern basic_score_t AttackValue;
	extern basic_score_t WeakSquares;
	extern basic_score_t NoEnemyQueens;
	extern basic_score_t EnemyPawns;
	extern basic_score_t QueenSafeCheckValue;
	extern basic_score_t RookSafeCheckValue;
	extern basic_score_t BishopSafeCheckValue;
	extern basic_score_t KnightSafeCheckValue;

	extern score_t PcSqTab[2][8][64];
	extern uint64_t KingZoneBB[2][64];
	extern uint64_t KingShelterBB[2][3];
	extern uint64_t KingShelter2BB[2][3];
	extern uint64_t KingShelter3BB[2][3];

	extern void initArr();
	extern void displayPST();
}