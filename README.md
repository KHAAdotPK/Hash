## Hash

A lightweight, header-only C++ utility for generating bucket indices from string data.
Implements a variant of the **djb2 algorithm** optimized for speed and uniform distribution
in hash-based data structures.

---

### Features

- **Fast String Hashing** — djb2 bit-shifting recurrence (`hash * 33 + c`) runs in O(n) time relative to string length.
- **Automatic Index Compression** — built-in modulo step maps any raw hash into a valid bucket index for a given array size.
- **Dual Interface** — accepts both null-terminated C-strings (`const char*`) and `const std::string&` objects via overloading.
- **Dynamic Rehashing Support** — built-in `next_prime()` and `is_prime()` utilities enable hash tables to grow at runtime when the load factor threshold is exceeded.
- **Configurable Load Factor** — `KEYS_LOAD_FACTOR_THRESHOLD` controls when rehashing is triggered; defaults to the industry standard `0.7`.
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
        └── keys.hh     # Keys class — djb2, compression, and prime utilities
```

---

### Installation

Clone the repository into your project's `lib/` directory:

```bash
git clone https://github.com/KHAAdotPK/Hash.git ./lib
```

Then add the include path to your compiler invocation:

```bash
# GCC / Clang
g++ -I./lib/Hash main.cpp -o my_program
```

---

### Usage

`Keys` exposes static methods for hashing, index compression, and prime number utilities.

```cpp
#include <iostream>
#include "./lib/Hash/header.hh"

int main()
{
    // Bucket count should be a prime number for best distribution.
    // KEYS_COMMON_STARTING_SIZE (1009) is the recommended starting value
    // for dynamic hash tables before any rehashing occurs.
    size_t bucket_count = KEYS_COMMON_STARTING_SIZE;

    // Works with std::string (const reference — temporaries and const variables both accepted).
    std::string word = "hello";
    size_t index = Keys::generate_key(word, bucket_count);
    std::cout << "index: " << index << std::endl;

    // Also works with raw C-strings
    size_t index2 = Keys::generate_key("world", bucket_count);
    std::cout << "index: " << index2 << std::endl;

    // When load factor is exceeded, grow to the next prime roughly double
    // the current bucket count to amortise rehashing cost.
    size_t buckets_used = 750;
    if ((double)buckets_used / (double)bucket_count > KEYS_LOAD_FACTOR_THRESHOLD)
    {
        bucket_count = Keys::next_prime(bucket_count * 2);
        // rehash all existing entries with new bucket_count
    }

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

### Dynamic Rehashing

The `Keys` class provides two utilities to support runtime table growth:

- `Keys::is_prime(n)` — returns `true` if `n` is prime, using the `6k ± 1` trial division optimisation.
- `Keys::next_prime(n)` — returns the smallest prime **strictly greater than** `n`, used to compute the new bucket count after a rehash.

The load factor threshold is controlled by:

```cpp
#define KEYS_LOAD_FACTOR_THRESHOLD 0.7
```

This can be overridden before including `header.hh` if your use case requires a different threshold. Always cast to `double` when evaluating the load factor to avoid integer division truncating the result to zero:

```cpp
if ((double)buckets_used / (double)bucket_count > KEYS_LOAD_FACTOR_THRESHOLD)
{
    // Grow to roughly 2x current size, landing on the nearest prime.
    // Calling next_prime(bucket_count) without doubling first produces
    // only a tiny increment (e.g. 1009 → 1013) and triggers rehashing
    // again almost immediately.
    bucket_count = Keys::next_prime(bucket_count * 2);
}
```

> **Note:** `next_prime()` is not bounded against `size_t` overflow. If `n` is near
> the largest prime representable by `size_t`, the internal loop will overflow and
> behave incorrectly. This edge case is unlikely in practice but is worth keeping in
> mind for long-running or memory-intensive workloads.

---

### Collision Handling

No hash function eliminates collisions entirely. Two distinct strings may compress
to the same index. This class does not handle collisions — that responsibility
belongs to the hash table that calls it. Two standard strategies are:

| Strategy | Behaviour |
|---|---|
| **Chaining** | Each bucket holds a linked list of all entries that map to it |
| **Linear Probing** | On collision, scan forward until an empty bucket is found |

#### Linear Probing — detailed documentation

A full technical reference on linear probing as it is implemented in this project is
available in [`LINEAR_PROBING.md`](https://github.com/KHAAdotPK/Parser/blob/main/DOCUMENTS/LINEAR_PROBING.md).

It covers:

- What a hash collision is and why it is unavoidable
- The probing formula — `probe = (key + 1) % bucket_count` — and wrap-around behaviour
- The three slot states the probe loop must handle (empty, matching word, different word)
- Step-by-step insertion and lookup walkthroughs using the `"hello"` / `"start"` collision pair from `usage/main.cpp`
- How linear probing is re-applied during rehashing when the table grows
- Primary clustering — why sequential probing causes runs of occupied buckets to grow faster than the load factor alone predicts, and how the 0.7 threshold is derived from Knuth's formula
- Why naive deletion (`slot = nullptr`) silently breaks lookup chains, and the three standard fixes: tombstone markers, backward shift deletion, and Robin Hood hashing
- A summary reference table covering every key concept

---

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

### Known Issues

- **`next_prime()` is unbounded near `size_t` maximum.**
  See the note in the Dynamic Rehashing section above.

---

### License

This project is governed by a license, the details of which can be located in the accompanying file named 'LICENSE.' Please refer to this file for comprehensive information.
