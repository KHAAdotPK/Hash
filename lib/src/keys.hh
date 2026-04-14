/*
    ./lib/src/keys.hh
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

#ifndef HASH_KEYS_HEADER_HH
#define HASH_KEYS_HEADER_HH

#ifndef KEYS_COMMON_STARTING_SIZE
#define KEYS_COMMON_STARTING_SIZE 1009 // Prime number, Option 3
#endif

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

class Keys
{        
    public:            
       /**
        * @brief Generates a compressed bucket index from a null-terminated C-string using the djb2 algorithm.
        *
        * Hashing is a two-step process:
        *
        * Step 1 - Generation: The djb2 algorithm walks the string character by character,
        * accumulating a large integer via the recurrence:
        *
        *     hash = (hash << 5) + hash + c
        *
        * which is equivalent to hash * 33 + c. The seed KEYS_COMMON_STARTING_SEED (5381)
        * is the historically proven starting value for this recurrence — it is NOT the
        * array size. Unsigned integer overflow during this step is intentional; C++ defines
        * it as modulo 2^n, which djb2 relies on for speed and distribution.
        *
        * Step 2 - Compression: The raw hash is reduced to a valid bucket index via modulo:
        *
        *     index = hash % array_size
        *
        * This maps any large hash value into the range [0, array_size - 1].
        *
        * @note array_size is expected to be a prime number. Prime-sized tables reduce
        * clustering after the modulo compression step and improve bucket distribution.
        *
        * @note This function does not handle collisions. Two distinct strings may compress
        * to the same index. The caller is responsible for a collision strategy at the
        * bucket level, either Linear Probing (scan forward for an empty slot) or
        * Chaining (maintain a linked list of entries per bucket).
        *
        * @note When the hash table is rehashed (array_size changes), all keys must be
        * reindexed by calling this function again with the new array_size. The raw djb2
        * hash of a string does not change, but its compressed index will.
        *
        * @param ptr        Pointer to the null-terminated character array to be hashed.
        * @param array_size Current bucket count of the destination hash table. Must be non-zero.
        * @return           Bucket index in the range [0, array_size - 1].
        */
       static size_t generate_key(const char* ptr, size_t array_size) 
       {
           // Step 1. Generation: A hash function turns data into a large, unique integer
           size_t hash = KEYS_COMMON_STARTING_SEED;

           for (size_t i = 0; ptr[i] != '\0'; i++)
           { 
               // hash * 33 + char: unsigned overflow is defined as modulo 2^n in C++   
               hash = ((hash << KEYS_BITWISE_LEFT_SHIFT_MULTIPLIER) + hash) + (unsigned char)*(ptr + i);
           }
                     
           /*
            * In high-performance data structures,
            * throwing an exception for a hash overflow is usually unnecessary.
            * Standard algorithms like djb2 or FNV-1a rely on bit-wrapping to maintain speed and distribution.
            */ 
            
           /* 
            * Step 2. Compression (The Modulo Step): This integer is reduced to fit within the actual size of your storage array. 
            * Compress the hash into a valid array index (bucket indexing),          
            * the modulo operation is used to ensure the index is within the array bounds
            */
           hash = hash % array_size;

           return hash;

           /*   
            * Collision Handling: We need to remember that no matter how good the hash function is,
            * different strings can still result in the same index (a collision) (e.g., if array_size is 10, both 12 % 10 and 22 % 10 result in 2)
            * Ensure that caller has a plan for this, such as Linear Probing or Chaining. 
            * 1. Linear Probing: If the index is full, move to the next available spot in the array.
            * 2. Chaining: If the index is full, create a linked list of values at that index.
            */
       }

       /**
        * @brief Overload of generate_key() for std::string.
        *
        * Extracts the underlying C-string from str and delegates to the
        * primary generate_key(const char*, size_t) implementation.
        * All hashing logic, guarantees, and caveats documented there apply here.
        *
        * @param str        Reference to the std::string to be hashed.
        * @param array_size Current bucket count of the destination hash table. Must be non-zero.
        * @return           Bucket index in the range [0, array_size - 1].
        */
       static size_t generate_key(std::string& str, size_t array_size) 
       {
           
           return generate_key(str.c_str(), array_size);
       }
};

#endif // HASH_HEADER_HH 