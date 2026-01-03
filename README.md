## Hash

A lightweight, high-performance C++ utility for generating unique integer keys and array indices from string data. This package implements a robust variant of the **djb2 algorithm**, optimized for speed and uniform distribution in hash-based data structures.

### Features
*   **Fast String Hashing:** Utilizes bit-shifting multipliers and prime seeds for O(n) performance.
*   **Automatic Compression:** Includes built-in modulo mapping to transform large hashes into valid array indices.
*   **Type Safe:** Optimized for `cc_tokenizer::String` containers while remaining compatible with standard C-style strings.
*   **Memory Efficient:** Header-only design with minimal overhead.

### Directory Structure
```text
Hash/
├── header.hh         # Main public interface
├── lib/
|   └── PULL.cmd      # Pulls the latest versions of dependencies from GitHub
└── src/
    └── keys.hh       # Implementation of the hashing logic

### Installation
To use this package in your project, simply include the Hash directory in your project's include path.

### Example include path addition for cl/g++
```bash
g++ -I./path/to/Hash_Package main.cpp -o my_program
```

### Usage

The keys class provides a static method generate_keys that accepts both high-level String objects and raw character pointers.

```cpp
#include <iostream> 
#include "Hash/header.hh"

int main() {   
    // Define your capacity (array size)
    const size_t BUCKET_COUNT = 100;
    
    // Initialize string data
    cc_tokenizer::String<char> str("hello");
    
    // Generate a valid index between 0 and 99
    size_t index = keys::generate_keys(str, BUCKET_COUNT);
    
    std::cout << "The calculated index is: " << index << std::endl;
   
    return 0;
}
```

### Algorithm Details

The core logic utilizes the djb2 hash algorithm variant:

1. Seed Initialization: Starts with a prime value (5381) to ensure a high degree of entropy.
2. Bitwise Processing: Uses a bitwise left-shift multiplier (5) to efficiently compute hash * 33 + char.
3. Index Mapping: Applies a modulo operation against the provided array_size to guarantee a safe displacement for data structures like Hash Tables or Sparse Arrays.

> Note: While this algorithm is highly resistant to collisions, ensure your implementation includes a collision-handling strategy (such as Linear Probing or Chaining) if you require absolute data integrity.

### License

This project is governed by a license, the details of which can be located in the accompanying file named 'LICENSE.' Please refer to this file for comprehensive information.
