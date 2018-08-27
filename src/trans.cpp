/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "trans.h"
#include "log.h"

bool pvhash_table_t::retrievePV(uint64_t hash, pv_hash_entry_t& pventry) const {
	pv_hash_entry_t* entry = getEntry(hash);
	for (int t = 0; t < bucket; ++t, ++entry) {
		if (entry->hashlock == lock(hash)) {
			entry->age = currentAge;
			pventry = *entry;
			return true;
		}
	}
	return false;;
}

void pvhash_table_t::storePV(uint64_t hash, move_t move, int depth) {
	int lowest = INT_MAX;
	pv_hash_entry_t *replace, *entry;

	replace = entry = getEntry(hash);
	for (int t = 0; t < bucket; ++t, ++entry) {
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

bool trans_table_t::retrieve(const uint64_t hash, tt_entry_t& ttentry) const {
	tt_entry_t* entry = getEntry(hash);
	for (int t = 0; t < bucket; ++t, ++entry) {
		if (entry->hashlock == lock(hash)) {
			entry->age = entry->age & 192;
			entry->age |= currentAge;
			ttentry = *entry;
			return true;
		}
	}
	return false;
}

void trans_table_t::store(uint64_t hash, move_t move, int depth, int bound) {
	int lowest = INT_MAX;
	tt_entry_t *replace, *entry;

	replace = entry = getEntry(hash);
	for (int t = 0; t < bucket; ++t, ++entry) {
		if (entry->hashlock == lock(hash)) {
			if (depth >= entry->depth) {
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
	replace->age = currentAge | (bound << 6);
	replace->move = move;
	replace->depth = depth;
}