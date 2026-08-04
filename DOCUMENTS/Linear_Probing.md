# Linear Probing — Collision Resolution in Hash Tables

## Table of Contents

1. [What Is a Hash Collision?](#1-what-is-a-hash-collision)
2. [What Is Linear Probing?](#2-what-is-linear-probing)
3. [The Probing Formula](#3-the-probing-formula)
4. [Three Scenarios Linear Probing Must Handle](#4-three-scenarios-linear-probing-must-handle)
5. [Walkthrough — Inserting a New Word (Collision Path)](#5-walkthrough--inserting-a-new-word-collision-path)
6. [Walkthrough — Finding an Existing Word (Collision Path)](#6-walkthrough--finding-an-existing-word-collision-path)
7. [Walkthrough — Rehashing with Linear Probing](#7-walkthrough--rehashing-with-linear-probing)
8. [Why the Wrap-Around (`% bucket_count`) Matters](#8-why-the-wrap-around--bucket_count-matters)
9. [The Probe Termination Condition](#9-the-probe-termination-condition)
10. [Known Limitations of Linear Probing](#10-known-limitations-of-linear-probing)
11. [Summary](#11-summary)

---

## 1. What Is a Hash Collision?

A hash table maps a word (key) to a **bucket index** using a hash function. Because the number of possible words is vastly larger than the number of buckets, two distinct words can compress to the **same index**. This is a collision.

Example using `Keys::generate_key()` with `bucket_count = 10`:

```
"hello"  →  raw hash → ... % 10  →  bucket 7
"start"  →  raw hash → ... % 10  →  bucket 7   ← collision!
```

Both words have a legitimate claim to bucket `7`. The hash table must decide what to do when the target bucket is already occupied.

---

## 2. What Is Linear Probing?

**Linear probing** is one of the simplest open-addressing collision-resolution strategies.

The rule is: if the target bucket is occupied, **step forward one bucket at a time** until you find a bucket that is either:

- **Empty** — insert the new word here, or
- **Occupied by the same word** — the word already exists; update it.

No extra memory structures (like linked lists per bucket) are needed. All entries live directly in the flat array.

```
Buckets:    [0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9]
Contents:                                       "hello"
                                                  ↑
"start" wants bucket 7, but it's taken.
Probe → bucket 8 (empty) → insert "start" there.
```

One of the main hardware advantages of linear probing over separate chaining (linked lists per bucket) is CPU cache line locality. Because adjacent buckets live in contiguous memory, scanning through probe + 1, probe + 2, etc., hits the CPU's L1/L2 cache prefetcher, drastically reducing memory latency compared to pointer-chasing across heap allocations.

---

## 3. The Probing Formula

The implementation uses this formula to advance the probe:

```cpp
size_t probe = (key + 1) % bucket_count;   // start one past the collision
// ...
probe = (probe + 1) % bucket_count;        // advance each iteration
```

The modulo (`% bucket_count`) wraps the probe back to `0` when it reaches the end of the array, turning the flat array into a logical ring.

```
Buckets:  [0]  [1]  [2]  ...  [8]  [9]
                                     ↓ wrap
                               ←←←←←[0]
```

---

## 4. Three Scenarios Linear Probing Must Handle

When a word arrives and its computed bucket `key` is occupied, the probe loop checks each slot it visits and takes one of three actions:

| Slot state | Meaning | Action |
|---|---|---|
| `hash_table[probe] == nullptr` | Empty slot found | Insert new `WordRecord` here |
| `hash_table[probe]->word == token` | Same word found | Append a new `OccurrenceNode` to its linked list |
| Otherwise | Different word, keep searching | Advance probe by 1 |

These three cases map directly to the `if / else if / (implicit continue)` structure inside the probe loop in `build_hash_table()`.

---

## 5. Walkthrough — Inserting a New Word (Collision Path)

The code first checks whether the initial bucket is empty:

```cpp
for (auto& line : *this)
{
    for (auto& token : line)                
    {
        /*
            Empty String Check
            -------------------
            The word can be an empty string, which is not a valid word for the hash table                        
         */
        if (token == "")
        {
            continue;
        }

        size_t key = Keys::generate_key(token, bucket_count); 
                    
        if (hash_table[key] == nullptr) // Case A: New Word 
        {
            try
            {
                 // Create new WordRecord for this unique token and insert into hash table at the generated key
                 // Token ID always originate at TOKEN_ID_ORIGINATE_AT_VALUE  
                 hash_table[key] = new WordRecord_new(bucket_used + TOKEN_ID_ORIGINATE_AT_VALUE, token, 1); 

                /*
                    TOKEN_ID_ORIGINATE_AT_VALUE is the base offset applied to the
                    first vocabulary word ID. In this codebase it is defined as 1,
                    so the first real token receives word_id = 1 rather than 0.

                    This keeps the compact word-id space separate from the special
                    padding slot used by the context-pair pipeline. In the current
                    Pairs implementation, missing left/right context positions are
                    filled with the literal value 0 in the context arrays, and that
                    value is interpreted as padding downstream.

                    The intent is therefore simple: reserve ID 0 for padding/special
                    handling, while all regular vocabulary tokens start at ID 1 or
                    higher. This avoids collisions between real vocabulary ids and
                    the sentinel value used for missing context positions.
                 */                                                                                                                                       
            }
            catch (const std::bad_alloc& e)
            {
                throw std::runtime_error("Parser::build_hash_table_very_new(void) Error: " + std::string(e.what()));
            }
                        
            bucket_used++; // Increment count of used buckets for load factor calculation
        }
    }
}       
```

If it is not empty and the word does **not** match (`hash_table[key]->word != token`), linear probing begins:

```cpp
for (auto& line : *this)
{
    for (auto& token : line)                
    {
        /*
            Empty String Check
            -------------------
            The word can be an empty string, which is not a valid word for the hash table                        
         */
        if (token == "")
        {
            continue;
        }

        size_t key = Keys::generate_key(token, bucket_count); 
                    
        if (hash_table[key] == nullptr) // Case A: New Word 
        {

        }
        else // Case C or D: Collision — need to probe for an empty bucket or a direct match
        {
            size_t probe = (key + 1) % bucket_count; // Linear probing

            while (probe != key) // Loop until we circle back to the original key
            {
                if (hash_table[probe] == nullptr) // Case D: New Displaced Word
                {
                    try
                    {
                        // Create new WordRecord for this unique token and insert into hash table at the probed key
                        hash_table[probe] = new WordRecord_new(bucket_used + TOKEN_ID_ORIGINATE_AT_VALUE, token, 1); 

                        /*
                            TOKEN_ID_ORIGINATE_AT_VALUE is the base offset applied to the
                            first vocabulary word ID. In this codebase it is defined as 1,
                            so the first real token receives word_id = 1 rather than 0.

                            This keeps the compact word-id space separate from the special
                            padding slot used by the context-pair pipeline. In the current
                            Pairs implementation, missing left/right context positions are
                            filled with the literal value 0 in the context arrays, and that
                            value is interpreted as padding downstream.

                            The intent is therefore simple: reserve ID 0 for padding/special
                            handling, while all regular vocabulary tokens start at ID 1 or
                            higher. This avoids collisions between real vocabulary ids and
                            the sentinel value used for missing context positions.
                         */                          
                    }
                    catch (const std::bad_alloc& e)
                    {
                        throw std::runtime_error("Parser::build_hash_table_very_new(void) Error: " + std::string(e.what()));
                    }

                    bucket_used++; // Increment count of used buckets for load factor calculation
                    break;
                }
                else if (hash_table[probe]->get_word() == token) // Case C: Probe Match
                {
                    hash_table[probe]->n++; // Increment frequency count for this word
                    break;
                }
                
                probe = (probe + 1) % bucket_count; // Move to the next bucket
                                        
                // The `% bucket_count` operation makes the probe wrap back to 0 when it
                // reaches the end of the array, so the search starts over from the beginning.
                // Each increment advances the probe one bucket at a time, and when it eventually
                // reaches the original key again, every bucket has been checked and the table
                // is considered full.
                if (probe == key)
                {
                    throw std::runtime_error("Parser::build_hash_table_very_new(void) Error: Hash table is full, cannot insert new word.");
                }

            }
        }
    }   
}       
```

**Concrete example:**

```
bucket_count = 10

Step 1: "hello" → key = 7. Bucket 7 is empty. Insert directly.
         [7] = "hello"

Step 2: "start" → key = 7. Bucket 7 is occupied by "hello". "hello" ≠ "start".
         Begin probing:
           probe = (7 + 1) % 10 = 8 → hash_table[8] == nullptr → INSERT "start" at [8]

         # To keep track of where each word actually lives, the code may also
         # maintain an index table that maps a word ID to its stored bucket.
         # In this example, "start" is stored at bucket 8, so its entry is recorded as 8,
         # not as the original home bucket 7.
         index_table[word_id_of_start] = 8
```

## 6. Walkthrough — Finding an Existing Word (Collision Path)

If `"start"` was already inserted at bucket `8`, and the corpus contains `"start"` again later:

```
"start" → key = 7. Bucket 7 is occupied by "hello". "hello" ≠ "start".
Begin probing:
  probe = 8 → hash_table[8]->word == "start" → MATCH FOUND
```

The code then appends a new `OccurrenceNode` to `"start"`'s existing linked list:

```cpp
for (auto& line : *this)
{
    for (auto& token : line)                
    {
        /*
            Empty String Check
            -------------------
            The word can be an empty string, which is not a valid word for the hash table                        
         */
        if (token == "")
        {
            continue;
        }

        size_t key = Keys::generate_key(token, bucket_count); 
                    
        if (hash_table[key] == nullptr) // Case A: New Word 
        {

        }
        else // Case C or D: Collision — need to probe for an empty bucket or a direct match
        {
            size_t probe = (key + 1) % bucket_count; // Linear probing

            while (probe != key) // Loop until we circle back to the original key
            {
                if (hash_table[probe] == nullptr) // Case D: New Displaced Word
                {
                }
                else if (hash_table[probe]->get_word() == token) // Case C: Probe Match
                {
                    hash_table[probe]->n++; // Increment frequency count for this word
                    break;
                }
                
                probe = (probe + 1) % bucket_count; // Move to the next bucket
                
            }
            
            // The `% bucket_count` operation makes the probe wrap back to 0 when it
            // reaches the end of the array, so the search starts over from the beginning.
            // Each increment advances the probe one bucket at a time, and when it eventually
            // reaches the original key again, every bucket has been checked and the table
            // is considered full.
            if (probe == key)
            {
                throw std::runtime_error("Parser::build_hash_table_very_new(void) Error: Hash table is full, cannot insert new word.");
            }
    }
}       
```

This is the same logic used for a word found at its **primary bucket** (no collision), just reached via a probe instead of directly.

---

## 7. Walkthrough — Rehashing with Linear Probing

When the load factor exceeds `KEYS_LOAD_FACTOR_THRESHOLD` (0.7):

```cpp
if ((static_cast<double>(bucket_used) / static_cast<double>(bucket_count)) > KEYS_LOAD_FACTOR_THRESHOLD)
```

...the table is grown and all entries are re-inserted into the new array. Crucially, every entry must be **re-probed** in the new table because the larger `bucket_count` changes every hash index:

```cpp
for (auto& line : *this)
{
    for (auto& token : line)                
    {
        /*
            Empty String Check
            -------------------
            The word can be an empty string, which is not a valid word for the hash table                        
         */
        if (token == "")
        {
            continue;
        }

        size_t key = Keys::generate_key(token, bucket_count); 
                    
        if (hash_table[key] == nullptr) // Case A: New Word 
        {

        }
        else // Case C or D: Collision — need to probe for an empty bucket or a direct match
        { 
        } 
        
         /*
            Check if the hash table needs to be rehashed
            Note: Integer division would truncate the result, so we cast to double
          */
         if ((static_cast<double>(bucket_used) / static_cast<double>(bucket_count)) > KEYS_LOAD_FACTOR_THRESHOLD)
         {
            /*
                Rehash all existing entries into new table
                Do NOT reset buckets_used, carry the real count forward
             */
            size_t old_bucket_count = bucket_count;
            bucket_count = Keys::next_prime(bucket_count);

            WordRecord_new** new_hash_table = nullptr; 

            size_t* new_index_table = nullptr; // New index table for the rehashed keys

            try
            {
                new_hash_table = new WordRecord_new*[bucket_count](); // Create new hash table with updated bucket count
                /*
                 * The () at the end is critical — it zero-initialises every pointer to nullptr.
                 * Without it, all bucket pointers are uninitialised garbage, and your
                 * (hash_table[key] == nullptr) check for unique words becomes undefined behaviour.
                 */

                new_index_table = new size_t[bucket_count + TOKEN_ID_ORIGINATE_AT_VALUE](); // Create new index table with updated bucket count
                                                                                                               // The + TOKEN_ID_ORIGINATE_AT_VALUE is to accommodate the offset for word IDs starting at TOKEN_ID_ORIGINATE_AT_VALUE (typically 1) rather than 0. This ensures that the index_table can store keys for all unique words, including the first one.            
                /*
                 * The () at the end is critical — it zero-initialises every entry of this array to zero.
                 */
            }
            catch (const std::bad_alloc& e)
            {
                throw std::runtime_error("Parser::build_hash_table_very_new(void) Error: " + std::string(e.what()));
            }
            // Rehash all entries from old table into new table
            for (size_t i = 0; i < old_bucket_count; ++i)
            {
                if (hash_table[i] != nullptr)  // Entry exists
                {
                    WordRecord_new* entry = hash_table[i];  // Copy the pointer
                    key = Keys::generate_key(entry->get_word(), bucket_count);

                    if (new_hash_table[key] == nullptr) // Case A: New Word 
                    {
                        new_hash_table[key] = entry;
                                    
                        *(new_index_table + entry->get_word_id()) = key; // Store the hash key for this unique word in the new index_table, indexed by word_id                                    
                    }                    
                    else // Collision — need to probe for an empty bucket or a direct match
                    {
                        size_t probe = (key + 1) % bucket_count; // Linear probing

                        while (probe != key) // Loop until we circle back to the original key
                        {
                            if (new_hash_table[probe] == nullptr) // Case D: New Displaced Word
                            {
                                new_hash_table[probe] = entry;
                                /*
                                    entry->get_word_id() returns the unique word ID assigned to this token, which is used as the index into the new index_table. The value stored at that index is the new hash key (probe) where this word now resides in the rehashed table.
                                    entry->get_word_id() returns a value in the range [TOKEN_ID_ORIGINATE_AT_VALUE, TOKEN_ID_ORIGINATE_AT_VALUE + bucket_used - 1], which is guaranteed to be within the bounds of the new index_table since bucket_used ≤ bucket_count. This ensures that we do not write out of bounds when storing the new hash key.
                                 */
                                *(new_index_table + entry->get_word_id()) = probe; // Store the hash key for this unique word in the new index_table, indexed by word_id
                                            
                                break;
                            }

                            probe = (probe + 1) % bucket_count; // Move to the next bucket
                        }

                        if (probe == key)
                        {
                            throw std::runtime_error("Parser::build_hash_table_very_new(void) Error: Hash table is full, cannot insert new word.");
                        }
                    }
                }
            }

            delete[] hash_table; // Free old table
            hash_table = new_hash_table; // Point to new table

            // index_table is a size_t** (pointer-to-pointer), so *index_table yields the size_t* array
            delete[] *index_table; // Free old index table
            *index_table = new_index_table; // Point to new index table
        }  
    }    
}       
```

**Why does rehashing help?**

With a larger `bucket_count`, the modulo compresses raw hashes into a wider range, so words that collided before are likely to land in different buckets after rehashing. The load factor drops well below `0.7`, and the average probe chain length shortens dramatically.

```
Before rehash:  bucket_count = 1009, bucket_used = 707  → load = 0.70 (threshold hit)
After rehash:   bucket_count = 1013, bucket_used = 707  → load = 0.698 (just under threshold)
```

> **Note:** The implementation calls `Keys::next_prime(bucket_count)` which returns only the next prime immediately above the current size (e.g. `1009 → 1013`). For large corpora this will trigger rehashing again very quickly. A more robust strategy is `Keys::next_prime(bucket_count * 2)` to roughly double capacity and keep the load factor low for longer.

```
`next_prime(bucket_count)` produces minimal growth, the load factor barely
drops and rehashing triggers again almost immediately:

    Before: bucket_count = 1009, bucket_used = 707  → load = 0.700
    After:  bucket_count = 1013, bucket_used = 707  → load = 0.698

`next_prime(bucket_count * 2)` amortises the cost of rehashing, the load factor drops
significantly and the table grows comfortably before the next rehash:

    Before: bucket_count = 1009, bucket_used = 707  → load = 0.700
    After:  bucket_count = 2029, bucket_used = 707  → load = 0.348

 In both cases, next_prime() is fine, but the latter would just reduce the frequency of rehashing and save the computational cost of rehashing, by keeping the load factor low for longer.   
``` 

---

## 8. Why the Wrap-Around (`% bucket_count`) Matters

Without wrapping, probing past the last bucket would access memory out of bounds. The modulo makes the array behave as a circular buffer:

```
bucket_count = 10

Probe from bucket 8:
  probe = 8 → occupied
  probe = (8+1) % 10 = 9 → occupied
  probe = (9+1) % 10 = 0 → empty → insert here ...

  Array Bounds:  [0]  [1]  ...  [8]  [9]
                  ▲                  │
                  └─── Wrap around ──┘

Without modulo:
  probe = 10 → out of bounds → undefined behaviour ✗
```

---

## 9. The Probe Termination Condition

The while loop guards against probing forever:

```cpp
while (probe != key)   // stop if probe wraps all the way back to the original bucket
{
    // ...
    probe = (probe + 1) % bucket_count;
}
```

If `probe` returns to `key`, every bucket has been visited and no empty slot was found — the table is completely full. In a well-maintained hash table (load factor ≤ 0.7), this should never happen: rehashing kicks in long before the table saturates.

---

## 10. Known Limitations of Linear Probing

### Primary Clustering

Because probing is strictly sequential, a run of occupied consecutive buckets tends to grow. Any word whose hash lands anywhere in that run extends it further. This is called **primary clustering** and it degrades lookup time from O(1) average toward O(n) worst case.

```text
Buckets: [_][_][X][X][X][X][_][_]
                 ↑─────────────┘
         Any key hashing here joins the cluster and extends it.
```

As you can see, there is no randomness, no skipping. Every displaced word lands as close to its home bucket as possible, and that closeness is exactly what causes clusters to merge. If bucket 7 is full and "start" goes to bucket 8, then any word that hashes to 8 now has to probe to 9 — even though it had no collision to begin with. Buckets 7, 8, and 9 are now a single cluster even though only bucket 7 had the original collision.

```cpp
// What it looks like in the code

size_t probe = (key + 1) % bucket_count;
while (probe != key)
{
    if (hash_table[probe] == nullptr) { /* insert here */ break; }
    else if (hash_table[probe]->word == token) { /* update here */ break; }
    probe = (probe + 1) % bucket_count;
}
```

The implementation mitigates this by keeping the load factor below `0.7`, which statistically limits average cluster length.

---

### Why 0.7 is the threshold, not 0.9 or 0.5

The relationship between load factor and average probe length for linear probing is described by the **Knuth formula** (from Donald Knuth's *The Art of Computer Programming*):

```
average probes (successful lookup)   ≈ ½ × (1 + 1 / (1 - α))
average probes (unsuccessful lookup) ≈ ½ × (1 + 1 / (1 - α)²)
```

where `α` is the load factor. Plugging in some values:

| Load factor (α) | Avg probes (hit) | Avg probes (miss) |
|---|---|---|
| 0.50 | 1.5 | 2.5 |
| 0.70 | 2.2 | 6.2 |
| 0.80 | 3.0 | 13.0 |
| 0.90 | 5.5 | 50.5 |
| 0.95 | 10.5 | 200.5 |

The table shows why 0.7 is the industry standard. From 0.5 to 0.7 the degradation is still manageable (1.5 → 2.2 probes on a hit). From 0.7 to 0.9 it explodes — a miss at 90% load costs 50× more than a miss at 50% load. The implementation in `build_hash_table()` uses `KEYS_LOAD_FACTOR_THRESHOLD = 0.7` exactly because the curve bends sharply here.

---

### Deletions Are Unsafe

Linear probing breaks if you delete a record by simply setting its slot to `nullptr`. A probe chain that passed through that slot during an earlier insert will be severed — a later lookup for the word at the end of the chain will stop prematurely at the now-empty slot and incorrectly conclude the word is absent.

The implementation avoids this problem because it never deletes individual entries during normal operation — records persist for the lifetime of the table.

When deletion genuinely needs to be supported, there are three approaches, each with different trade-offs.

**Tombstone markers** replace a deleted slot with a special sentinel value — not `nullptr`, but a distinct constant like `DELETED`. The probe loop is changed to treat tombstones as occupied (keep scanning) during lookups but as free (stop and insert) during insertions. This preserves chains for lookups while reclaiming the slot for future inserts. The cost is that tombstones accumulate over time and are never freed until a full rehash — a table with many deletions can end up mostly tombstones, with the real load factor much lower than the apparent one. This is exactly what CPython's dictionary implementation uses.

**Backward shift deletion** avoids tombstones entirely. After deleting a slot, the algorithm scans forward and finds any neighbour that is displaced from its home bucket — i.e. any word that was pushed here by probing past the now-empty slot. That neighbour is slid one step backward toward its home, filling the gap. This repeats until a truly empty slot is found. The result is a gap-free array with no sentinels, and lookups remain correct because the invariant is restored immediately. The cost is that each deletion may cascade through several shifts. This is more complex to implement correctly but more memory-efficient than tombstones.

**Robin Hood hashing** is a variant of linear probing where every entry also stores its displacement — the number of probe steps it took to reach its current slot. During insertion, if a new entry has a shorter displacement than the one already in a slot ("richer" than the incumbent), the incumbent is evicted and re-inserted elsewhere. This keeps displacement variance low across the whole table, which makes lookups uniformly fast. Deletion with Robin Hood involves filling the vacated slot from the next entry (similar to backward shifting) while maintaining the displacement invariant. It is the most sophisticated of the three and is used in many high-performance hash maps.

---

### Rehash Does Not Eliminate Collisions

The inline comment in the code is explicit about this:

```cpp
/*
 * No check of the hash collision is done here, because we are setting a threshold for the load factor.
 * Note: The load factor only reduces collision probability, it does not eliminate it.
 * At any load factor, two different words can still hash to the same bucket. This is a data-loss bug, not a probability question.
 */
```

This is why the rehash loop **also** applies linear probing — it does not blindly overwrite `new_hash_table[new_key]`, it checks first and probes if that slot is taken.

---

## 11. Summary

| Concept | Detail |
|---|---|
| **Collision** | Two words map to the same bucket index after `hash % bucket_count` |
| **Linear probe start** | `probe = (key + 1) % bucket_count` |
| **Advance** | `probe = (probe + 1) % bucket_count` |
| **Stop: empty slot** | Insert new `WordRecord` at `probe`; store `probe` (not `key`) in `index_table` |
| **Stop: word match** | Append new `OccurrenceNode` to existing word's list |
| **Stop: full circle** | `probe == key` — table is full (should not happen at load ≤ 0.7) |
| **Rehash trigger** | `bucket_used / bucket_count > 0.7` |
| **Rehash probing** | Same linear probe logic applied to the new, larger array |
| **Wrap-around** | `% bucket_count` turns the flat array into a logical ring |
| **Primary weakness** | Primary clustering degrades performance at high load factors |
