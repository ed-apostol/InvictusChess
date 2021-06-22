/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once

#include <algorithm>
#include <cstring>
#include "typedefs.h"
#include "log.h"
#include "eval.h"

template <typename T>
class hashtable_t {
public:
    hashtable_t() : table(nullptr), size(0), mask(0) {}
    ~hashtable_t() { delete[] table; }
    void clear() { memset(table, 0, size * sizeof(T)); }
    T& getEntry(const uint64_t hash) { return table[hash & mask]; }
    void init(uint64_t mb) {
        for (size = (1 << 20) / sizeof(T), mb <<= 19; size * sizeof(T) <= mb; size <<= 1);
        mask = size - 1;
        delete[] table;
        table = new T[size];
        clear();
    }
    uint32_t lock(uint64_t hash) const { return hash >> 32; }

protected:
    T *table;
    uint64_t size;
    uint64_t mask;
};

#pragma pack(push, 1)
struct eval_hash_entry_t {
    uint32_t hashlock;
    int16_t eval;
};
#pragma pack(pop)

struct eval_bucket_t {
    eval_hash_entry_t bucket[5];
    uint16_t padding;
};

class eval_table_t : public hashtable_t < eval_bucket_t > {
public:
    int retrieve(position_t& pos);
    eval_t eval;
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
    TT_LOWER = 1,
    TT_UPPER,
    TT_EXACT = TT_LOWER | TT_UPPER
};

class trans_table_t : public hashtable_t < tt_bucket_t > {
public:
    void resetAge() { currentAge = 0; }
    void updateAge() { currentAge = (currentAge + 1) % 64; }
    void store(uint64_t hash, move_t move, int depth, int bound);
    bool retrieve(uint64_t hash, tt_entry_t& ttentry);
private:
    int currentAge;
};

struct move_hash_t {
    uint32_t hashlock;
    int depth() { return hashlock & 0xff; }
};

struct movehash_bucket_t {
    move_hash_t bucket[4];
};

struct abdada_table_t : public hashtable_t < movehash_bucket_t > {
    void setBusy(uint32_t hash, uint16_t m, int d);
    void resetBusy(uint32_t hash, uint16_t m, int d);
    bool isBusy(uint32_t hash, uint16_t m, int d);
    uint32_t hashkey(uint32_t hash, uint16_t m, int d) { return (hash << 24) | (m << 8) | d; }
};