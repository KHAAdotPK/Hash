/*
    ./header.hh
    Q@hackers.pk
 */

 /*
    Hash Table Size (array_size)
    ----------------------------
    To build the hash table → need array_size
    To know array_size     → need vocabulary count
    To know vocabulary     → need to scan the corpus
    So you must scan TWICE → or pre-allocate blindly

    Option 1 — Two-Pass Approach (simplest fix)
    - Pass 1: Scan corpus, count unique words → get vocabulary size
    - Pass 2: Build hash table with correct array_size
    - Clean but reads corpus twice, expensive for large corpora

    Option 2 — Over-allocate with a Load Factor
    ```C++
    // If you expect ~50,000 unique words, allocate ~1.5x with a prime
    size_t array_size = 76963; // prime near 50000 * 1.5
    ```
    Industry standard is to keep load factor below 0.7 (70% full).
    Most real corpora have predictable vocabulary ranges (50k–200k words).

    Option 3 — Dynamic Rehashing (most robust)
    - Start with a small prime (e.g. 1009) 
      - It's how many buckets your hash table allocates at the start
      - When load factor is exceeded, you grow this (e.g. 1009 → 2027 → 4057...)    
    - Track how full the table gets
    - When load factor exceeds threshold (e.g. > 0.7) → allocate a new larger array, rehash everything
    - This is what std::unordered_map does internally
  */ 

#ifndef HASH_HEADER_HH
#define HASH_HEADER_HH

#include <string>

/*
 * Common starting size for the hash table.
 * Must be a prime number.
 * This is the initial number of buckets in the hash table.
 */
#ifndef KEYS_COMMON_STARTING_SIZE
#define KEYS_COMMON_STARTING_SIZE 1009 // Prime number, Option 3
#endif

/*
 * The maximum load factor before the hash table is rehashed.
 * When (buckets_used / bucket_count) exceeds this value, bucket_count
 * is grown to the next suitable prime and all keys are reindexed.
 *
 * 0.7 is the industry standard threshold — beyond it, collision chains
 * grow long enough to meaningfully degrade lookup performance.
 */
#define KEYS_LOAD_FACTOR_THRESHOLD 0.7

/*
    Bitwise Left Shift Multiplier
    ---------------------------
    This is the multiplier used in the djb2 algorithm.    
    It is a prime number that is used to multiply the hash by 33...  
    (hash << 5) + hash is equivalent to hash * 33
 */
#define KEYS_BITWISE_LEFT_SHIFT_MULTIPLIER 5

/*
 * Common starting seed. Must be a prime number.
 * 5381 is the historically proven seed for the djb2 algorithm that provides better distribution and fewer collisions.
 */
#define KEYS_COMMON_STARTING_SEED 5381

#include "./lib/src/keys.hh"

#endif // HASH_HEADER_HH 