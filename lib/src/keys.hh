/*
    ./lib/src/keys.hh
    Q@hackers.pk
 */

#ifndef HASH_KEYS_HEADER_HH
#define HASH_KEYS_HEADER_HH

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

       static size_t next_prime(size_t n)
       {
           if (n <= 1) // 1 is not a prime number
           {
             return 2;
           }
           if (n % 2 == 0) // if n is divisible by 2, it is not a prime number
           {
               n++;
           }
           while (!is_prime(n)) // while n is not a prime number
           {
               n += 2; // increment n by 2
           }

           return n;
       }

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
       }
};

#endif // HASH_HEADER_HH 