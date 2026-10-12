/*
    ./lib/src/keys.hh

    Maintainer: Sohail
 */

#ifndef HASH_LIB_SRC_KEYS_HH
#define HASH_LIB_SRC_KEYS_HH

/*
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
 */

class Keys
{        
    public:

        /**
         * @brief Combines two independent 64-bit hashes into a single composite hash.
         *
         * This method implements the industry-standard bit-mixing algorithm popularized by the 
         * Boost library. Instead of hashing the raw binary bytes of the two input structures via a loop, 
         * it relies on fast, single-step bitwise shifts and an irrational mathematical constant 
         * (the Golden Ratio) to securely blend the entropy of both fields.
         *
         * @note Order Sensitivity:
         * This function is strictly non-commutative. combine_hashes(A, B) will generate a vastly 
         * different signature than combine_hashes(B, A). This structural property is essential 
         * for sequential subword tokenization (e.g., Byte Pair Encoding), where the direction 
         * and pairing order of subwords dictate unique vocabularies.
         *
         * @param parent_hash The primary or left-hand hash (e.g., left_subword_representation_ptr->hash).
         * @param child_hash  The secondary or right-hand hash to fold in (e.g., right_subword_representation_ptr->hash).
         * @return A single, highly distributed size_t hash representing the ordered combination of both inputs.
         */
        static size_t combine_hashes(size_t parent_hash, size_t child_hash)
        {
            /*
             * 1. Add the right hash to the left hash.
             * 2. Add the 64-bit golden ratio magic constant to break up repeating patterns and clusters.
             * 3. Apply asymmetric bitwise shifts (left shift by 6, right shift by 2) to trigger an 
             *    "avalanche effect", scattering bit modifications across the entire 64-bit integer range.
             * 4. XOR the scrambled result back into the parent container.
             */
            parent_hash ^= child_hash + KEYS_GOLDEN_RATIO_64 + (parent_hash << 6) + (parent_hash >> 2);
                           
            return parent_hash;
        }

        static size_t generate_hash(size_t intermediate_hash, const char* ptr, size_t n)
        {
            size_t hash = intermediate_hash;

           for (size_t i = 0; i < n; i++)
           { 
               /* 
                * hash * 33 + char: unsigned overflow is defined as modulo 2^n in C++   
                */
                hash = ((hash << KEYS_BITWISE_LEFT_SHIFT_MULTIPLIER) + hash) + (unsigned char)*(ptr + i);
           }
                                
           return hash;
        }

        /*
         * @brief Generates a compressed bucket index from a (optionally) null-terminated C-string.
         *    
         * @param intermediate_hash Initial hash value.
         * @param ptr               Pointer to the (optionally) null-terminated character array to be hashed.
         * @param n                 Length of the (optionally) null-terminated character array.
         * @param array_size        Current bucket count of the destination hash table. Must be non-zero.
         * @return                  Bucket index in the range [0, array_size - 1].
         */ 
        static size_t generate_key(size_t intermediate_hash, const char* ptr, size_t n, size_t array_size)
        {
           /* Step 1. Generation: A hash function turns data into a large, unique integer */        
           size_t hash = intermediate_hash;

           for (size_t i = 0; i < n; i++)
           { 
               /* 
                * hash * 33 + char: unsigned overflow is defined as modulo 2^n in C++   
                */
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

        static size_t generate_hash(const char* ptr, size_t n)
        {
            size_t intermediate_hash = KEYS_COMMON_STARTING_SEED;

            for (size_t i = 0; i < n; i++)
            { 
                /* 
                 * hash * 33 + char: unsigned overflow is defined as modulo 2^n in C++   
                 */
                intermediate_hash = ((intermediate_hash << KEYS_BITWISE_LEFT_SHIFT_MULTIPLIER) + intermediate_hash) + (unsigned char)*(ptr + i);
            }

            return intermediate_hash;
        }
        
        /*
         * @brief Generates a compressed bucket index from a (optionally) null-terminated C-string.
         *    
         * @param ptr        Pointer to the (optionally) null-terminated character array to be hashed.
         * @param n          Length of the (optionally) null-terminated character array.
         * @param array_size Current bucket count of the destination hash table. Must be non-zero.
         * @return           Bucket index in the range [0, array_size - 1].
         */ 
        static size_t generate_key(const char* ptr, size_t n, size_t array_size)
        {   
            /* Step 1. Generation: A hash function turns data into a large, unique integer */        
           size_t hash = KEYS_COMMON_STARTING_SEED;

           for (size_t i = 0; i < n; i++)
           { 
               /* 
                * hash * 33 + char: unsigned overflow is defined as modulo 2^n in C++   
                */
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
        
        static size_t generate_hash(const char* ptr) 
        {
            size_t hash = KEYS_COMMON_STARTING_SEED;

            for (size_t i = 0; ptr[i] != '\0'; i++)
            { 
               /* 
                * hash * 33 + char: unsigned overflow is defined as modulo 2^n in C++   
                */
                hash = ((hash << KEYS_BITWISE_LEFT_SHIFT_MULTIPLIER) + hash) + (unsigned char)*(ptr + i);
            }
            
            return hash;
        }
    
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
           /* Step 1. Generation: A hash function turns data into a large, unique integer */        
           size_t hash = KEYS_COMMON_STARTING_SEED;

           for (size_t i = 0; ptr[i] != '\0'; i++)
           { 
               /* 
                * hash * 33 + char: unsigned overflow is defined as modulo 2^n in C++   
                */
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

       static size_t generate_hash(const std::string& str)
       {
           return generate_hash(str.c_str());
       }

       /**
        * @brief Overload of generate_key() for std::string.
        *
        * Extracts the underlying C-string from str and delegates to the
        * primary generate_key(const char*, size_t) implementation.
        * All hashing logic, guarantees, and caveats documented there apply here.
        *
        * @param str        Constant reference to the std::string to be hashed.
        * @param array_size Current bucket count of the destination hash table. Must be non-zero.
        * @return           Bucket index in the range [0, array_size - 1].
        */
       static size_t generate_key(const std::string& str, size_t array_size) 
       {           
           return generate_key(str.c_str(), array_size);
       }

       /*
        * @brief Generates the smallest prime number strictly greater than n.
        *
        * @note Dynamic Rehashing:
        * When expanding a hash table due to load factor overflow, pass:
        *     Keys::next_prime(bucket_count * KEYS_TABLE_GROWTH_FACTOR)
        * rather than bucket_count alone. Passing bucket_count alone will only advance
        * to the immediate next prime (e.g. 1009 -> 1013), causing continuous O(N) rehashes
        * on almost every subsequent insertion. Multiplying by KEYS_TABLE_GROWTH_FACTOR (2x)
        * roughly doubles capacity (e.g. 1009 -> 2027), halving the load factor and ensuring
        * amortized O(1) insertion time.
        *
        * @param n The number to start searching from (typically old_bucket_count * KEYS_TABLE_GROWTH_FACTOR).
        * @return The smallest prime number strictly greater than n.
        */
       static size_t next_prime(size_t n)
       {
           if (n <= 1) // 1 is not a prime number. better is (n < 2)
           {
             return 2;
           }

           /* 
            * To prevent this implementation from returning n itself if n is already prime.
            * Always start checking from the next number. 
            */
           n++;

           // If n is even, increment it to make it odd (since all even numbers > 2 are not prime)
           if (n % 2 == 0)
           {
               n++;
           }
           
           while (!is_prime(n)) // while n is not a prime number
           {
               n += 2; // increment n by 2
           }

           return n;
       }

       /*
        * Checks if a number is a prime number.
        *
        * @param n The number to check.
        * @return True if n is a prime number, false otherwise.
        */
       static bool is_prime(size_t n)
       {
           if (n <= 1) // 1 is not a prime number
           {
               return false;
           }
           if (n <= 3) // 2 and 3 are prime numbers
           {
               return true;
           }
           if (n % 2 == 0 || n % 3 == 0) // if n is divisible by 2 or 3, it is not a prime number
           {
               return false;
           }
           for (size_t i = 5; i * i <= n; i = i + 6) // check for divisibility by numbers of the form 6k ± 1
           {
               if (n % i == 0 || n % (i + 2) == 0) // if n is divisible by i or i + 2, it is not a prime number
               {
                   return false;
               }
           }

           return true;

           /*
                - For very large values of size_t, i * i <= n can technically overflow before it fails the condition. A safer check is i <= n / i.
                - If n is the largest prime representable by size_t, this will enter an infinite loop or overflow. (Rare, but worth noting for robust library code).
            */
       }    
};  
#endif // HASH_KEYS_HEADER_HH 