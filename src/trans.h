/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include <algorithm>
#include "typedefs.h"
#include "log.h"

template <typename hash_entity_t>
class hashtable_t {
public:
	hashtable_t() : table(nullptr), tabsize(0), mask(0), bucket(0) {}
	~hashtable_t() { delete[] table; }
	virtual void clear() { memset(table, 0, tabsize * sizeof(hash_entity_t)); }
	hash_entity_t* getEntry(const uint64_t hash) const { return &table[key(hash) & mask]; }
	void init(uint64_t mb, int bucketsize) {
		bucket = bucketsize;
		tabsize = 1024;
		for (mb <<= 19; tabsize * sizeof(hash_entity_t) <= mb; tabsize <<= 1);
		mask = tabsize - 1;
		tabsize += bucket - 1;
		delete[] table;
		table = new hash_entity_t[tabsize];
		clear();
	}
	uint64_t key(uint64_t hash) const { return hash & 0xffff; }
	uint64_t lock(uint64_t hash) const { return hash >> 48; }

protected:
	hash_entity_t * table;
	uint64_t tabsize;
	uint64_t mask;
	int bucket;
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
	void updateAge() { currentAge = ++currentAge % 256; }
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
	TT_EXACT = 1,
	TT_LOWER,
	TT_UPPER
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
