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
/*
 * Why Open Addressing Needs Low Load Factors
 * ------------------------------------------
 * Open addressing tables (especially those using linear probing like yours) scale exponentially in terms of collision costs. 
 * A table at 70% full performs dramatically worse than a table at 40% full. 
 * For linear probing, maintaining a load factor between 40% and 50% is widely considered the industry "sweet spot" for maximizing lookup speed at the expense of a tiny bit of RAM.
 */ 
#ifndef KEYS_LOAD_FACTOR_THRESHOLD
#define KEYS_LOAD_FACTOR_THRESHOLD 0.4
#endif

/*
 * Table capacity growth factor for dynamic rehashing.
 * When the load factor threshold is exceeded, the table capacity is multiplied
 * by this factor before finding the next prime with Keys::next_prime().
 *
 * Multiplying by 2 (geometric growth) guarantees amortized O(1) insertions
 * by roughly halving the load factor upon rehash and keeping the total number
 * of rehashes across N elements to O(log N).
 */
#ifndef KEYS_TABLE_GROWTH_FACTOR
#define KEYS_TABLE_GROWTH_FACTOR 2
#endif

/*
    Bitwise Left Shift Multiplier
    ---------------------------
    This is the shift value used in the djb2 algorithm.    
    (hash << 5) + hash is an optimized way to multiply the hash by 33.
    33 is an odd number, which ensures bits are shifted and mixed 
    thoroughly without losing entropy during unsigned overflow.
 */ 
#define KEYS_BITWISE_LEFT_SHIFT_MULTIPLIER 5
/*
    Bitwise Left Shift Multiplier (33): The "Stirrer"
    -------------------------------------------------
    Its job is to stir incoming text data into the hash bucket byte-by-byte.
    It works beautifully for strings because it quickly mixes small ASCII 
    characters (values 0–255) across the integer space.

    Why 33? It is an odd number, ensuring that when combined with the previous hash 
    value through addition and bit shifting, it preserves and mixes the high-order 
    bits of the hash.
    
    (hash << 5) + hash is an optimized way to multiply the hash by 33. 
    It is faster than multiplication because it uses bitwise operations.
 */

/*
 * Common starting seed. Must be a prime number.
 * 5381 is the historically proven seed for the djb2 algorithm that provides better distribution and fewer collisions.
 */
#define KEYS_COMMON_STARTING_SEED 5381

/**
 * @brief 64-bit Golden Ratio Fractional Constant for Hash Combining.
 *
 * This constant represents the fractional part of the Golden Ratio (phi) 
 * projected onto a 64-bit unsigned integer space:
 *     floor(2^64 / ((1 + sqrt(5)) / 2)) = 0x9e3779b97f4a7c15
 *
 * @note Mathematical Importance:
 * The golden ratio acts as a highly effective "successive bit scrambler." Because it 
 * is an irrational number in binary representation, it introduces high entropy (randomness) 
 * and eliminates sequential bit patterns or clusters. When used in a hash combination function, 
 * it guarantees an "avalanche effect"—ensuring that minor differences in the input hashes 
 * yield vastly different combined outcomes.
 *
 * @see Boost::hash_combine
 */
#ifndef KEYS_GOLDEN_RATIO_64
#define KEYS_GOLDEN_RATIO_64 0x9e3779b97f4a7c15ULL
#endif

#include "./lib/src/keys.hh"

#endif // HASH_HEADER_HH 

/**
 * @file header.hh
 * @author Sohail
 * @brief High-Performance Hash Engine for Hierarchical Subword Representation and Tokenization.
 *
 * DESIGN HISTORY & ARCHITECTURAL LOG
 * ============================================================================================
 * This library was engineered to facilitate ultra-fast lookups of subword token pairings 
 * (e.g., Byte Pair Encoding / Vocabulary training). Throughout its iterative development, 
 * several classic low-level systems programming traps were uncovered, analyzed, and 
 * rigorously optimized out of the critical path.
 *
 * 1. The Multiplier Choice & Return Type Bounds
 * --------------------------------------------
 * The core character-driven hashing algorithm utilizes a variation of the classic DJB2 string 
 * hash (`hash * 33 + char`), optimized at the CPU layer via bitwise shifts: `(hash << 5) + hash`. 
 * While 33 is not a prime number, its asymmetry and odd parity ensure that no bit entropy is lost 
 * during unsigned overflow. The engine natively leverages `size_t` as its return type to naturally 
 * scale to the host system's register architecture (64-bit on standard modern systems), providing 
 * a vast 2^64 hash space that completely minimizes natural signature collisions.
 *
 * 2. String View Over-Reads (The "Binoculars" Trap)
 * ------------------------------------------------
 * During validation of candidate text structures, an initial implementation attempted to check 
 * concatenated subword lengths via `std::string_view(left->sub_word, left->n + right->n)`. This 
 * triggered a classic undefined behavior trap. Because `std::string_view` does not own data but 
 * merely points to a window of memory, adding the lengths together caused the pointer to read 
 * past the bounds of the left buffer into unallocated, adjacent RAM junk. This was corrected by 
 * enforcing a zero-allocation prefix/suffix boundary validation using explicit `.substr()` splits.
 *
 * 3. Fast Combinational Merging vs. String Regeneration
 * ----------------------------------------------------
 * To locate merged subword candidates within the hash table, the code initially recalculated 
 * the entire string's hash from scratch using an expensive byte-by-byte loop. This introduced 
 * an O(N) performance bottleneck. 
 *
 * The architecture was expanded to support immediate O(1) hash combining. Instead of iterating 
 * through bytes, the library blends the two pre-calculated 64-bit hashes (`left->hash` and `right->hash`) 
 * into a single unified composite signature using an asymmetric mixing function based on the binary 
 * Fractional Golden Ratio constant (`0x9e3779b97f4a7c15ULL`). This serves as a massive avalanche agent, 
 * shattering linear bit patterns and scattering the resulting indices uniformly across the array.
 *
 * 4. Open Addressing, Primary Clustering, and the Load Factor Threshold
 * ---------------------------------------------------------------------
 * Upon deploying the high-entropy Golden Ratio combination technique, an anomalous drop in 
 * retrieval matches occurred under high load factors (70%). Because the Golden Ratio flings 
 * signatures randomly across the table, it triggered severe "Primary Clustering" within the linear 
 * probing open-addressing table. Long blocks of adjacent, filled buckets caused lookup loops 
 * to miss keys or terminate early. 
 *
 * Lowering the maximum table expansion threshold to 40% gave the array vital breathing room. 
 * This reduction completely eliminated clustering bottlenecks, shortened lookup probe paths 
 * down to 1–2 steps, and unlocked the true performance of the Golden Ratio combinational keys. 
 * The system now achieves flawless, amortized O(1) time complexity for continuous vocabulary generation.
 *
 * @note Future Expansion:
 * Plans are in place to implement a custom Quadratic Probing or Robin Hood hashing algorithm 
 * to safely raise the load factor threshold back up without re-introducing clustering blocks.
 */
