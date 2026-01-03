/*
 * src/keys.hh
 * Q@hacker.pk
 */

#ifndef HASH_FOR_KEYS_OF_ALICIA_HEADER_HH
#define HASH_FOR_KEYS_OF_ALICIA_HEADER_HH

#define KEYS_BITWISE_LEFT_SHIFT_MULTIPLIER 5
/*
 * Common starting seed. Must be a prime number.
 * 5381 is the historically proven seed for the djb2 algorithm that provides better distribution and fewer collisions.
 */
#define KEYS_COMMON_STARTING_SEED 5381 

class keys
{        
    public:
        
       /**
        * @brief Generates a compressed hash index from a null-terminated character string.
        * 
        * This function implements a variant of the djb2 algorithm to transform a C-style string 
        * into a unique integer hash. The resulting hash is then compressed using a modulo 
        * operation to ensure it serves as a valid displacement or index within the specified 
        * array bounds. 
        * 
        * @note This implementation relies on unsigned overflow behavior (modulo 2^n) to maintain 
        * performance and distribution efficiency.
        * 
        * @param ptr A pointer to the null-terminated character array to be hashed.
        * @param array_size The total capacity of the destination data structure, used for index compression.
        * @return A size_type representing the calculated index (bucket) within the range [0, array_size - 1].
        */
       static cc_tokenizer::string_character_traits<char>::size_type generate_key(const char* ptr, cc_tokenizer::string_character_traits<char>::size_type array_size) 
       {
           // Step 1. Generation: A hash function turns data into a large, unique integer
           cc_tokenizer::string_character_traits<char>::size_type hash = KEYS_COMMON_STARTING_SEED;

           for (cc_tokenizer::string_character_traits<char>::size_type i = 0; ptr[i] != '\0'; i++)
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
        * @brief Generates a compressed hash index from a high-level String object.
        * 
        * This overloaded method provides an interface for the cc_tokenizer::String container. 
        * It extracts the underlying character buffer and delegates the hashing logic to the 
        * primary C-string implementation.
        * 
        * @param str A reference to the cc_tokenizer::String object to be processed.
        * @param array_size The total capacity of the destination data structure for index mapping.
        * @return A size_type representing the calculated index (bucket) within the range [0, array_size - 1].
        */
       static cc_tokenizer::string_character_traits<char>::size_type generate_key(cc_tokenizer::String<char>& str, cc_tokenizer::string_character_traits<char>::size_type array_size) 
       {
           
           return generate_key(str.c_str(), array_size);
       }

};

#endif //HASH_FOR_KEYS_OF_ALICIA_HEADER_HH

