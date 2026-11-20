#pragma once

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

#include "fnv.h"
#include <esp_err.h>
#include <stdint.h>
#include <stdlib.h>
#include <new>
#include <type_traits>

// -----------------------------------------------------------------------------

namespace lightstd {

// Default hash using FNV-1a on raw bytes (works for POD types)
template <typename K>
struct static_hash_map_default_hash
{
    // Hashes the key as a raw byte sequence.
    uint32_t operator()(const K& key) const noexcept
    {
        return fnv1a32(&key, sizeof(key));
    }
};

// Stores key-value pairs in a fixed-size open-addressed hash table.
template <typename K, typename V, typename HashFn = static_hash_map_default_hash<K>>
class static_hash_map
{
    static_assert(std::is_nothrow_invocable_r_v<uint32_t, HashFn&, const K&>,
                  "static_hash_map requires HashFn(const K&) to return uint32_t without throwing.");

private:
    typedef enum State_e : uint8_t {
        EMPTY = 0,
        OCCUPIED = 1,
        TOMBSTONE = 2
    } State_t;

    typedef struct Entry_s {
        alignas(K) uint8_t keyStorage[sizeof(K)];
        alignas(V) uint8_t valueStorage[sizeof(V)];
        State_t state;

        K* key() noexcept
        {
            return reinterpret_cast<K*>(keyStorage);
        }

        const K* key() const noexcept
        {
            return reinterpret_cast<const K*>(keyStorage);
        }

        V* value() noexcept
        {
            return reinterpret_cast<V*>(valueStorage);
        }

        const V* value() const noexcept
        {
            return reinterpret_cast<const V*>(valueStorage);
        }
    } Entry_t;

public:
    // Creates an empty map with no allocated table.
    static_hash_map() noexcept = default;
    static_hash_map(const static_hash_map&) = delete;
    // Transfers ownership of the allocated hash table.
    static_hash_map(static_hash_map&& other) noexcept
        : table(other.table), tableSize(other.tableSize), count(other.count), tombstones(other.tombstones)
    {
        other.table = nullptr;
        other.tableSize = 0;
        other.count = 0;
        other.tombstones = 0;
    }

    // Releases the allocated hash table.
    ~static_hash_map()
    {
        deinit();
    }

    static_hash_map& operator=(const static_hash_map&) = delete;
    // Transfers ownership of the allocated hash table.
    static_hash_map& operator=(static_hash_map&& other) noexcept
    {
        if (this != &other)
        {
            deinit();

            table = other.table;
            tableSize = other.tableSize;
            count = other.count;
            tombstones = other.tombstones;

            other.table = nullptr;
            other.tableSize = 0;
            other.count = 0;
            other.tombstones = 0;
        }
        return *this;
    }

    // Allocates a fixed-size table for the requested number of slots.
    esp_err_t init(size_t _tableSize) noexcept
    {
        Entry_t* newTable;

        if (_tableSize < 1)
        {
            return ESP_ERR_INVALID_ARG;
        }
        if (_tableSize > SIZE_MAX / sizeof(Entry_t))
        {
            return ESP_ERR_INVALID_ARG;
        }

        newTable = static_cast<Entry_t*>(malloc(_tableSize * sizeof(Entry_t)));
        if (!newTable)
        {
            return ESP_ERR_NO_MEM;
        }

        for (size_t i = 0; i < _tableSize; i++)
        {
            newTable[i].state = EMPTY;
        }

        deinit();
        table = newTable;
        tableSize = _tableSize;

        // Done
        return ESP_OK;
    }

    // Releases the allocated table and resets the map state.
    void deinit() noexcept
    {
        if (table)
        {
            clear();
            free(table);
            table = nullptr;
        }
        tableSize = 0;
        count = 0;
        tombstones = 0;
    }

    // Releases the allocated table and resets the map state.
    void done() noexcept
    {
        deinit();
    }

    // Inserts a new key or updates the value of an existing key.
    V* insert(const K& key, const V& value, bool* inserted = nullptr) noexcept
    {
        if (!table)
        {
            if (inserted)
            {
                *inserted = false;
            }
            return nullptr;
        }

        size_t idx = getHash(key);
        size_t start = idx;
        size_t firstTombstone = tableSize;

        if (inserted)
        {
            *inserted = false;
        }

        do
        {
            if (table[idx].state == EMPTY)
            {
                // EMPTY terminates the probe chain, so a remembered tombstone is now safe to reuse.
                if (firstTombstone != tableSize)
                {
                    idx = firstTombstone;
                    tombstones -= 1;
                }
                constructEntry(table[idx], key, value);
                table[idx].state = OCCUPIED;
                count += 1;
                if (inserted)
                {
                    *inserted = true;
                }
                return table[idx].value();
            }
            if (table[idx].state == TOMBSTONE && firstTombstone == tableSize)
            {
                firstTombstone = idx;
            }
            if (table[idx].state == OCCUPIED && *table[idx].key() == key)
            {
                *table[idx].value() = value; // Update existing
                return table[idx].value();
            }
            if ((++idx) >= tableSize)
            {
                idx = 0;
            }
        }
        while (idx != start);

        if (firstTombstone != tableSize)
        {
            // A tombstone remains probe-visible but is available for insertion.
            constructEntry(table[firstTombstone], key, value);
            table[firstTombstone].state = OCCUPIED;
            tombstones -= 1;
            count += 1;
            if (inserted)
            {
                *inserted = true;
            }
            return table[firstTombstone].value();
        }

        // Done
        return nullptr;
    }

    // Returns the value pointer for a key or nullptr if not found.
    V* find(const K& key) noexcept
    {
        if (!table)
        {
            return nullptr;
        }

        size_t idx = getHash(key);
        size_t start = idx;

        do
        {
            if (table[idx].state == EMPTY)
            {
                return nullptr;
            }
            if (table[idx].state == OCCUPIED && *table[idx].key() == key)
            {
                return table[idx].value();
            }
            if ((++idx) >= tableSize)
            {
                idx = 0;
            }
        }
        while (idx != start);

        // Not found
        return nullptr;
    }

    // Reports whether the requested key exists in the map.
    bool contains(const K& key) noexcept
    {
        return find(key) != nullptr;
    }

    // Removes a key and leaves a tombstone for probing continuity.
    bool erase(const K& key) noexcept
    {
        if (!table)
        {
            return false;
        }

        size_t idx = getHash(key);
        size_t start = idx;

        do
        {
            if (table[idx].state == EMPTY)
            {
                return false;
            }
            if (table[idx].state == OCCUPIED && *table[idx].key() == key)
            {
                destroyEntry(table[idx]);
                count -= 1;
                table[idx].state = TOMBSTONE;
                tombstones += 1;
                rehashIfNeeded(false);
                return true;
            }
            if ((++idx) >= tableSize)
            {
                idx = 0;
            }
        }
        while (idx != start);

        // Not found
        return false;
    }

    // Returns the number of occupied entries.
    size_t size() const noexcept
    {
        return count;
    }

    // Reports whether the map contains no entries.
    bool empty() const noexcept
    {
        return count == 0;
    }

    // Removes all entries without releasing the table allocation.
    void clear() noexcept
    {
        for (size_t i = 0; i < tableSize; i += 1)
        {
            if (table[i].state == OCCUPIED)
            {
                destroyEntry(table[i]);
            }
            table[i].state = EMPTY;
        }
        count = 0;
        tombstones = 0;
    }

    // Rebuilds the table to remove accumulated tombstones.
    void compact() noexcept
    {
        if (tombstones > 0)
        {
            rehashIfNeeded(true);
        }
    }

public:
    typedef struct key_value_s
    {
        K& key;
        V& value;
    } key_value_t;

    // Exposes a read-only key-value view when iterating a const map.
    typedef struct const_key_value_s
    {
        const K& key;
        const V& value;
    } const_key_value_t;

public:
    class iterator
    {
    public:
        // Creates an iterator over the allocated table range.
        iterator(Entry_t* p, Entry_t* e) noexcept : ptr(p), end(e)
        {
            // Skip to first occupied slot
            while (ptr != end && ptr->state != OCCUPIED)
            {
                ptr += 1;
            }
        }
        iterator(const iterator&) noexcept = default;
        iterator(iterator&&) noexcept = default;

        ~iterator() noexcept = default;

        iterator& operator=(const iterator&) noexcept = default;
        iterator& operator=(iterator&&) noexcept = default;

        // Advances to the next occupied slot.
        iterator& operator++() noexcept
        {
            if (ptr != end)
            {
                ptr += 1;
                // Skip non-occupied slots
                while (ptr != end && ptr->state != OCCUPIED)
                {
                    ptr += 1;
                }
            }
            return *this;
        }

        // Compares iterator positions.
        bool operator!=(const iterator& other) const noexcept
        {
            return ptr != other.ptr;
        }

        // Returns references to the current key and value.
        key_value_t operator*() noexcept
        {
            return {*ptr->key(), *ptr->value()};
        }

    private:
        Entry_t* ptr;
        Entry_t* end;
    };

    class const_iterator
    {
    public:
        // Creates a const iterator over the allocated table range.
        const_iterator(const Entry_t* p, const Entry_t* e) noexcept : ptr(p), end(e)
        {
            while (ptr != end && ptr->state != OCCUPIED)
            {
                ptr += 1;
            }
        }
        const_iterator(const const_iterator&) noexcept = default;
        const_iterator(const_iterator&&) noexcept = default;

        ~const_iterator() noexcept = default;

        const_iterator& operator=(const const_iterator&) noexcept = default;
        const_iterator& operator=(const_iterator&&) noexcept = default;

        // Advances to the next occupied slot.
        const_iterator& operator++() noexcept
        {
            if (ptr != end)
            {
                ptr += 1;
                while (ptr != end && ptr->state != OCCUPIED)
                {
                    ptr += 1;
                }
            }
            return *this;
        }

        // Compares iterator positions.
        bool operator!=(const const_iterator& other) const noexcept
        {
            return ptr != other.ptr;
        }

        // Returns const references to the current key and value.
        const_key_value_t operator*() const noexcept
        {
            return {*ptr->key(), *ptr->value()};
        }

    private:
        const Entry_t* ptr;
        const Entry_t* end;
    };

public:
    // Iteration follows internal table/probe order, not key order, matching std::unordered_map rather than std::map.
    iterator begin() noexcept
    {
        return iterator(getTableBegin(), getTableEnd());
    }

    // Returns an iterator one past the last table slot.
    iterator end() noexcept
    {
        return iterator(getTableEnd(), getTableEnd());
    }

    // Returns a const iterator to the first occupied slot.
    const_iterator begin() const noexcept
    {
        return const_iterator(getTableBegin(), getTableEnd());
    }

    // Returns a const iterator one past the last table slot.
    const_iterator end() const noexcept
    {
        return const_iterator(getTableEnd(), getTableEnd());
    }

private:
    Entry_t* getTableBegin() noexcept
    {
        return table;
    }

    Entry_t* getTableEnd() noexcept
    {
        return table ? (table + tableSize) : nullptr;
    }

    const Entry_t* getTableBegin() const noexcept
    {
        return table;
    }

    const Entry_t* getTableEnd() const noexcept
    {
        return table ? (table + tableSize) : nullptr;
    }

    // Rebuild table when there are more than 25% of tombstones
    void rehashIfNeeded(bool force) noexcept
    {
        if (force || tombstones > tableSize / 4)
        {
            if (count == 0)
            {
                clear();
                return;
            }

            rehashInPlace();
        }
    }

    size_t getHash(const K& key) noexcept
    {
        return hasher(key) % tableSize;
    }

    static void constructEntry(Entry_t& entry, const K& key, const V& value) noexcept
    {
        ::new (static_cast<void*>(entry.keyStorage)) K(key);
        ::new (static_cast<void*>(entry.valueStorage)) V(value);
    }

    static void destroyEntry(Entry_t& entry) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<V>)
        {
            entry.value()->~V();
        }
        if constexpr (!std::is_trivially_destructible_v<K>)
        {
            entry.key()->~K();
        }
    }

    bool isProbePath(size_t start, size_t target, size_t end) const noexcept
    {
        if (start <= end)
        {
            return target >= start && target <= end;
        }
        return target >= start || target <= end;
    }

    void rehashInPlace() noexcept
    {
        // Backward-shift entries into each tombstone without allocating a temporary table.
        while (tombstones > 0)
        {
            size_t hole = 0;
            while (table[hole].state != TOMBSTONE)
            {
                hole += 1;
            }

            size_t scan = hole + 1;
            if (scan == tableSize)
            {
                scan = 0;
            }

            for (size_t searched = 0; searched < tableSize; searched++)
            {
                if (table[scan].state == EMPTY)
                {
                    break;
                }
                if (table[scan].state == OCCUPIED && isProbePath(getHash(*table[scan].key()), hole, scan))
                {
                    constructEntry(table[hole], *table[scan].key(), *table[scan].value());
                    table[hole].state = OCCUPIED;

                    destroyEntry(table[scan]);
                    table[scan].state = TOMBSTONE;
                    hole = scan;
                }

                scan += 1;
                if (scan == tableSize)
                {
                    scan = 0;
                }
            }

            table[hole].state = EMPTY;
            tombstones -= 1;
        }
    }

private:
    HashFn   hasher;
    Entry_t* table{nullptr};
    size_t   tableSize{0};
    size_t   count{0};
    size_t   tombstones{0};
};

} // namespace lightstd
