/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "trans.h"
#include "log.h"

bool pvhash_table_t::retrievePV(uint64_t hash, pv_hash_entry_t& pventry) {
	pv_bucket_t *B = getEntry(hash);
	pv_hash_entry_t *entry = &B->bucket[0];
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
	int lowest = INT_MAX;
	pv_bucket_t *B = getEntry(hash);
	pv_hash_entry_t *entry = &B->bucket[0], *replace = entry;
	for (int t = 3; t--; ++entry) {
		if (entry->hashlock == lock(hash)) {
			if (depth >= entry->depth) {
				replace = entry;
				break;
			}
			else return;
		}
		int score = (entry->age << 8) + entry->depth;
		if (score < lowest) {
			lowest = score;
			replace = entry;
		}
	}
	replace->hashlock = lock(hash);
	replace->age = currentAge;
	replace->move = move;
	replace->depth = depth;
}

bool trans_table_t::retrieve(const uint64_t hash, tt_entry_t& ttentry) {
	tt_bucket_t *B = getEntry(hash);
	tt_entry_t *entry = &B->bucket[0];
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
	int lowest = INT_MAX;
	tt_bucket_t *B = getEntry(hash);
	tt_entry_t *entry = &B->bucket[0], *replace = entry;
	for (int t = 3; t--; ++entry) {
		if (entry->hashlock == lock(hash)) {
			if (bound == TT_EXACT || depth >= entry->depth) {
				replace = entry;
				break;
			}
			else return;
		}
		int score = (entry->getAge() << 8) + entry->depth;
		if (score < lowest) {
			lowest = score;
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
	movehash_bucket_t *B = getEntry(hash);
	move_hash_t *entry = &B->bucket[0], *replace = entry;
	for (int t = 4; t--; ++entry) {
		if (entry->move == move && entry->hashlock == mhlock(hash)) return;
		if (entry->depth < lowest) lowest = entry->depth, replace = entry;
	}
	replace->move = move;
	replace->depth = depth;
	replace->hashlock = mhlock(hash);
}

void abdada_table_t::resetBusy(const uint32_t hash, uint16_t move) {
	movehash_bucket_t *B = getEntry(hash);
	move_hash_t *entry = &B->bucket[0];
	for (int t = 4; t--; ++entry) {
		if (entry->move == move && entry->hashlock == mhlock(hash)) {
			entry->move = 0;
			entry->depth = 0;
			entry->hashlock = 0;
		}
	}
}

bool abdada_table_t::isBusy(const uint32_t hash, uint16_t move) {
	movehash_bucket_t *B = getEntry(hash);
	move_hash_t *entry = &B->bucket[0];
	for (int t = 4; t--; ++entry) {
		if (entry->move == move && entry->hashlock == mhlock(hash)) return true;
	}
	return false;
}