/**************************************************/
/*  Invictus 2019                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "trans.h"
#include "log.h"

bool pvhash_table_t::retrievePV(uint64_t hash, pv_hash_entry_t& pventry) {
	pv_hash_entry_t *entry = &getEntry(hash).bucket[0];
	for (int t = 3; t--; ++entry) {
		if (entry->hashlock == lock(hash)) {
			entry->age = currentAge;
			pventry = *entry;
			return true;
		}
	}
	return false;
}

void pvhash_table_t::storePV(uint64_t hash, move_t move, int depth) {
	int highest = INT_MIN;
	pv_hash_entry_t *entry = &getEntry(hash).bucket[0], *replace = entry;
	for (int t = 3; t--; ++entry) {
		if (entry->hashlock == lock(hash)) {
			if (depth >= entry->depth) {
				replace = entry;
				break;
			}
			else return;
		}
		int score = (((256 + currentAge - entry->age) % 256) << 8) - entry->depth;
		if (score > highest) {
			highest = score;
			replace = entry;
		}
	}
	replace->hashlock = lock(hash);
	replace->age = currentAge;
	replace->move = move;
	replace->depth = depth;
}

bool trans_table_t::retrieve(const uint64_t hash, tt_entry_t& ttentry) {
	tt_entry_t *entry = &getEntry(hash).bucket[0];
	for (int t = 3; t--; ++entry) {
		if (entry->hashlock == lock(hash)) {
			entry->setAge(currentAge);
			ttentry = *entry;
			return true;
		}
	}
	return false;
}

void trans_table_t::store(uint64_t hash, move_t move, int depth, int bound) {
	int highest = INT_MIN;
	tt_entry_t *entry = &getEntry(hash).bucket[0], *replace = entry;
	for (int t = 3; t--; ++entry) {
		if (entry->hashlock == lock(hash)) {
			if (bound == TT_EXACT || depth >= entry->depth) {
				replace = entry;
				break;
			}
			else return;
		}
		int score = (((64 + currentAge - entry->getAge()) % 64) << 8) - entry->depth;
		if (score > highest) {
			highest = score;
			replace = entry;
		}
	}
	replace->hashlock = lock(hash);
	replace->setAgeAndBound(currentAge, bound);
	replace->move = move;
	replace->depth = depth;
}

void abdada_table_t::setBusy(const uint32_t hash, uint16_t move, int depth) {
	int lowest = INT_MAX;
	move_hash_t *entry = &getEntry(hash).bucket[0], *replace = entry;
	for (int t = 4; t--; ++entry) {
		if (entry->move == move && entry->hashlock == mhlock(hash) && entry->depth == depth) return;
		if (entry->depth < lowest) lowest = entry->depth, replace = entry;
	}
	replace->move = move;
	replace->depth = depth;
	replace->hashlock = mhlock(hash);
}

void abdada_table_t::resetBusy(const uint32_t hash, uint16_t move, int depth) {
	move_hash_t *entry = &getEntry(hash).bucket[0];
	for (int t = 4; t--; ++entry) {
		if (entry->move == move && entry->hashlock == mhlock(hash) && entry->depth == depth) {
			entry->move = 0;
			entry->depth = 0;
			entry->hashlock = 0;
		}
	}
}

bool abdada_table_t::isBusy(const uint32_t hash, uint16_t move, int depth) {
	move_hash_t *entry = &getEntry(hash).bucket[0];
	for (int t = 4; t--; ++entry) {
		if (entry->move == move && entry->hashlock == mhlock(hash) && entry->depth == depth) return true;
	}
	return false;
}