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
	hashtable_t() : table(nullptr), tabsize(0), mask(0) {}
	~hashtable_t() { delete[] table; }
	virtual void clear() { memset(table, 0, tabsize * sizeof(hash_entity_t)); }
	hash_entity_t* getEntry(const uint64_t hash) const { return &table[hash & mask]; }
	void init(uint64_t mb) {
		//PrintOutput() << sizeof(hash_entity_t);
		tabsize = (1 << 20) / sizeof(hash_entity_t); // at least 1MB
		for (mb <<= 19; tabsize * sizeof(hash_entity_t) <= mb; tabsize <<= 1);
		mask = tabsize - 1;
		delete[] table;
		table = new hash_entity_t[tabsize];
		clear();
	}
	uint32_t lock(uint64_t hash) const { return hash >> 32; }

protected:
	hash_entity_t * table;
	uint64_t tabsize;
	uint64_t mask;
};

#pragma pack(push, 1)
struct pv_hash_entry_t {
	uint32_t hashlock;
	move_t move;
	uint8_t depth;
	uint8_t age;
};
#pragma pack(pop)

struct pv_bucket_t {
	pv_hash_entry_t bucket[3];
	uint16_t padding;
};

class pvhash_table_t : public hashtable_t < pv_bucket_t > {
public:
	void resetAge() { currentAge = 0; }
	void updateAge() { currentAge = ++currentAge % 256; }
	void storePV(uint64_t hash, move_t move, int depth);
	bool retrievePV(const uint64_t hash, pv_hash_entry_t& pventry);
private:
	int currentAge;
};

#pragma pack(push, 1)
struct tt_entry_t {
	uint32_t hashlock;
	move_t move;
	uint8_t depth;
	int getAge() { return age & 63; }
	int getBound() { return age >> 6; }
	void setAgeAndBound(int a, int b) { age = a | (b << 6); }
	void setAge(int a) { age &= 192; age |= a; }
private:
	uint8_t age;
};
#pragma pack(pop)

struct tt_bucket_t {
	tt_entry_t bucket[3];
	uint16_t padding;
};

enum TTBounds {
	TT_EXACT = 1,
	TT_LOWER,
	TT_UPPER
};

class trans_table_t : public hashtable_t < tt_bucket_t > {
public:
	void resetAge() { currentAge = 0; }
	void updateAge() { currentAge = ++currentAge % 64; }
	void store(uint64_t hash, move_t move, int depth, int bound);
	bool retrieve(const uint64_t hash, tt_entry_t& ttentry);
private:
	int currentAge;
};

struct move_hash_t {
	uint32_t movehash;
};

struct movehash_bucket_t {
	move_hash_t bucket[4];
};

struct abdada_table_t : public hashtable_t < movehash_bucket_t > {
	void setBusy(const uint32_t hash);
	void resetBusy(const uint32_t hash);
	bool isBusy(const uint32_t hash);
};