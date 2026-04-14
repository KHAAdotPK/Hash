## Hash

A lightweight, header-only C++ utility for generating bucket indices from string data.
Implements a variant of the **djb2 algorithm** optimized for speed and uniform distribution
in hash-based data structures.

---

### Features

- **Fast String Hashing** — djb2 bit-shifting recurrence (`hash * 33 + c`) runs in O(n) time relative to string length.
- **Automatic Index Compression** — built-in modulo step maps any raw hash into a valid bucket index for a given array size.
- **Dual Interface** — accepts both null-terminated C-strings (`const char*`) and `std::string` objects via overloading.
- **Header-Only** — no compilation step required; include and use.

---

### Directory Structure

```text
Hash/
├── header.hh           # Public interface — include this in your project
├── usage/
│   └── main.cpp        # Collision demonstration example
└── lib/
    └── src/
        └── keys.hh     # Keys class — djb2 implementation and compression logic
```

---

### Installation

Clone the repository into your project's `lib/` directory:

```bash
git clone ttps://github.com/KHAAdotPK/Hash.git ./lib
```

Then add the include path to your compiler invocation:

```bash
# GCC / Clang
g++ -I./lib/Hash main.cpp -o my_program
```

---

### Usage

`Keys` exposes a single static method `generate_key()` with two overloads.
Pass your string and the current bucket count — it returns a valid index in `[0, array_size - 1]`.

```cpp
#include <iostream>
#include "./lib/Hash/header.hh"

int main()
{
    // Bucket count should be a prime number for best distribution.
    // KEYS_COMMON_STARTING_SIZE (1009) is the recommended starting value
    // for dynamic hash tables before any rehashing occurs.
    const size_t BUCKET_COUNT = KEYS_COMMON_STARTING_SIZE;

    // Works with std::string
    std::string word = "hello";
    size_t index = Keys::generate_key(word, BUCKET_COUNT);
    std::cout << "index: " << index << std::endl;

    // Also works with raw C-strings
    size_t index2 = Keys::generate_key("world", BUCKET_COUNT);
    std::cout << "index: " << index2 << std::endl;

    return 0;
}
```

---

### Algorithm

Hashing is a two-step process:

**Step 1 — Generation**

The djb2 recurrence walks the string and accumulates a raw hash:

```
hash = (hash << 5) + hash + c       // equivalent to hash * 33 + c
```

Starting seed is `5381` (`KEYS_COMMON_STARTING_SEED`) — a historically proven prime
that provides strong initial entropy and minimizes early collisions. This value is
unrelated to array size.

**Step 2 — Compression**

The raw hash is mapped to a valid bucket index:

```
index = hash % array_size
```

When the hash table is **rehashed** (i.e. `array_size` grows), all keys must be
reindexed by calling `generate_key()` again with the new array size.

---

### Collision Handling

No hash function eliminates collisions entirely. Two distinct strings may compress
to the same index. This class does not handle collisions — that responsibility
belongs to the hash table that calls it. Two standard strategies are:

| Strategy | Behaviour |
|---|---|
| **Chaining** | Each bucket holds a linked list of all entries that map to it |
| **Linear Probing** | On collision, scan forward until an empty bucket is found |

### Collision Example

A working demonstration is provided in `usage/main.cpp`. It deliberately selects two
strings — `"hello"` and `"start"` — that compress to the same bucket index when
`array_size` is `10`, producing a detectable collision at runtime.

To compile and run:

```bash
g++ -I./.. usage/main.cpp -o collision_demo
./collision_demo
```

Expected output:

```text
"hello"  -> bucket 7
"start"  -> bucket 7

Collision detected: both strings map to bucket 7
```

This example exists to confirm that `Keys` correctly and consistently maps distinct
strings to the same index under the right conditions — a prerequisite for testing
any collision-handling strategy built on top of it.

---

### License

This project is governed by a license, the details of which can be located in the accompanying file named 'LICENSE.' Please refer to this file for comprehensive information.
