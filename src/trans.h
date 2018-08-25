/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include <algorithm>
#include "typedefs.h"

template <typename hash_entity_t>
class hashtable_t {
public:
	hashtable_t() : table(nullptr), tabsize(0), mask(0), bucket(0) {}
	~hashtable_t() { delete[] table; }
	virtual void clear() { memset(table, 0, tabsize * sizeof(hash_entity_t)); }
	hash_entity_t* getEntry(const uint64_t hash) const { return &table[key(hash) & mask]; }
	void init(const int targetMB, const size_t bucket_size) {
		size_t size = 2;
		bucket = bucket_size;
		size_t half = std::max(1, targetMB) << 19;
		while (size * sizeof(hash_entity_t) <= half) size *= 2;
		if (size + bucket_size - 1 == tabsize) {
			clear();
		}
		else {
			tabsize = size + bucket_size - 1;
			mask = size - 1;
			delete table;
			table = new hash_entity_t[tabsize];
		}
	}
	size_t key(uint64_t hash) const { return hash & 0xffff; }
	size_t lock(uint64_t hash) const { return hash >> 48; }

protected:
	hash_entity_t * table;
	size_t tabsize;
	size_t mask;
	size_t bucket;
};

struct pv_hash_entry_t {
	uint16_t hashlock;
	move_t move;
	uint8_t depth;
	uint8_t age;
};

class pvhash_table_t : public hashtable_t < pv_hash_entry_t > {
public:
	void resetAge() { currentAge = 0; }
	void updateAge() { ++currentAge; }
	void storePV(uint64_t hash, move_t move, int depth);
	bool retrievePV(const uint64_t hash, pv_hash_entry_t& pventry) const;
private:
	int currentAge;
};

struct tt_entry_t {
	uint16_t hashlock;
	move_t move;
	uint8_t depth;
	uint8_t age;
	int getAge() { return age & 63; }
	int bound() { return age >> 6; }
};

enum TTBounds {
	EXACT = 1,
	LOWERBOUND,
	UPPERBOUND
};

class trans_table_t : public hashtable_t < tt_entry_t > {
public:
	void resetAge() { currentAge = 0; }
	void updateAge() { currentAge = ++currentAge % 64; }
	void store(uint64_t hash, move_t move, int depth, int bound);
	bool retrieve(const uint64_t hash, tt_entry_t& ttentry) const;
private:
	int currentAge;
};
