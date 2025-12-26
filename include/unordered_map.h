#pragma once

#include "fnv.h"
#include <esp_err.h>

// -----------------------------------------------------------------------------

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

// Default hash using FNV-1a on raw bytes (works for POD types)
template<typename K>
struct StaticHashMapDefaultHash
{
    uint32_t operator()(const K& key) const
        {
            return fnv1a32(&key, sizeof(key));
        }
};

// Simple static unordered map with open addressing and linear probing.
// Note: No dynamic resizing, fixed size table allocated at initialization.
template<typename K, typename V, typename HashFn = StaticHashMapDefaultHash<K>>
class StaticHashMap
{
private:
    typedef enum State_e : uint8_t {
        EMPTY = 0,
        OCCUPIED = 1,
        TOMBSTONE = 2
    } State_t;

    typedef struct Entry_s {
        K key;
        V value;
        State_t state;
    } Entry_t;

public:
    StaticHashMap() : table(nullptr), tableSize(0), count(0), tombstones(0)
        {
        };

    ~StaticHashMap()
        {
            done();
        };

    esp_err_t init(size_t _tableSize)
        {
            if (_tableSize < 1) {
                return ESP_ERR_INVALID_ARG;
            }

            table = (Entry_t *)malloc(_tableSize * sizeof(Entry_t));
            if (!table) {
                return ESP_ERR_NO_MEM;
            }
            tableSize = _tableSize;

            for (size_t i = 0; i < tableSize; i++) {
                table[i].state = EMPTY;
            }

            // Done
            return ESP_OK;
        };

    void done()
        {
            if (table) {
                memset(table, 0, tableSize * sizeof(Entry_t));
                free(table);
                table = nullptr;
            }
            tableSize = 0;
            count = 0;
            tombstones = 0;
        };

    // Insert or update
    V* insert(const K& key, const V& value, bool* inserted = nullptr)
        {
            size_t idx = getHash(key) % tableSize;
            size_t start = idx;
            size_t firstTombstone = tableSize; // Invalid index

            if (inserted) {
                *inserted = false;
            }

            do {
                if (table[idx].state == EMPTY) {
                    if (count >= tableSize) {
                        return nullptr;
                    }

                    // Use tombstone slot if we found one earlier
                    if (firstTombstone != tableSize) {
                        idx = firstTombstone;
                        tombstones -= 1;
                    }
                    table[idx].key = key;
                    table[idx].value = value;
                    table[idx].state = OCCUPIED;
                    count += 1;
                    if (inserted) {
                        *inserted = true;
                    }
                    return &table[idx].value;
                }
                if (table[idx].state == TOMBSTONE && firstTombstone == tableSize) {
                    firstTombstone = idx; // Remember first tombstone
                }
                if (table[idx].state == OCCUPIED && table[idx].key == key) {
                    table[idx].value = value; // Update existing
                    return &table[idx].value;
                }
                if ((++idx) >= tableSize) {
                    idx = 0;
                }
            }
            while (idx != start);

            // Done
            return nullptr;
        };

    // Find value by key (fast - only stops at EMPTY)
    V* find(const K& key)
        {
            size_t idx = getHash(key) % tableSize;
            size_t start = idx;

            do {
                if (table[idx].state == EMPTY) {
                    return nullptr;
                }
                if (table[idx].state == OCCUPIED && table[idx].key == key) {
                    return &table[idx].value;
                }
                if ((++idx) >= tableSize) {
                    idx = 0;
                }
            }
            while (idx != start);

            // Not found
            return nullptr;
        };

    // Check if key exists
    bool contains(const K& key)
        {
            return find(key) != nullptr;
        };

    // Remove entry (fast - just mark as tombstone)
    bool erase(const K& key)
        {
            size_t idx = getHash(key) % tableSize;
            size_t start = idx;

            do {
                if (table[idx].state == EMPTY) {
                    return false;
                }
                if (table[idx].state == OCCUPIED && table[idx].key == key) {
                    table[idx].state = TOMBSTONE;
                    memset(&table[idx].key, 0, sizeof(K));
                    memset(&table[idx].value, 0, sizeof(V));

                    count -= 1;
                    tombstones += 1;
                    rehashIfNeeded(false);
                    return true;
                }
                if ((++idx) >= tableSize) {
                    idx = 0;
                }
            }
            while (idx != start);

            // Not found
            return false;
        };

    // Get current size
    size_t size() const
        {
            return (size_t)count;
        };

    // Check if empty
    bool empty() const
        {
            return count == 0;
        };

    // Clear all entries
    void clear()
        {
            for (size_t i = 0; i < tableSize; i += 1) {
                table[i].state = EMPTY;
                memset(&table[i].key, 0, sizeof(K));
                memset(&table[i].value, 0, sizeof(V));
            }
            count = 0;
            tombstones = 0;
        };

    // Force cleanup of tombstones
    void compact()
        {
            if (tombstones > 0) {
                rehashIfNeeded(true);
            }
        };

public:
    struct KeyValue {
        K& key;
        V& value;
    };

public:
    class Iterator
    {
    public:
        Iterator(Entry_t* p, Entry_t* e) : ptr(p), end(e)
            {
                // Skip to first occupied slot
                while (ptr != end && ptr->state != OCCUPIED) {
                    ptr += 1;
                }
            };

        Iterator& operator++()
            {
                if (ptr != end) {
                    ptr += 1;
                    // Skip non-occupied slots
                    while (ptr != end && ptr->state != OCCUPIED) {
                        ptr += 1;
                    }
                }
                return *this;
            };

        bool operator!=(const Iterator& other) const
            {
                return ptr != other.ptr;
            };

        KeyValue operator*()
            {
                return {ptr->key, ptr->value};
            };

    private:
        Entry_t* ptr;
        Entry_t* end;
    };

public:
    Iterator begin()
        {
            return Iterator(table, table + tableSize);
        };

    Iterator end()
        {
            return Iterator(table + tableSize, table + tableSize);
        };

private:
    // Rebuild table when there are more than 25% of tombstones
    void rehashIfNeeded(bool force)
        {
            if (force || tombstones > tableSize / 4) {
                if (count == 0) {
                    clear();
                    return;
                }

                // Try to allocate temporary storage for optimal rehashing
                Entry_t* temp = (Entry_t*)malloc(count * sizeof(Entry_t));
                if (temp) {
                    // Fast path: We have temporary storage, do optimal rehash
                    size_t i;

                    // Extract all occupied entries
                    size_t tempIdx = 0;
                    for (i = 0; i < tableSize; i++) {
                        if (table[i].state == OCCUPIED) {
                            temp[tempIdx].key = table[i].key;
                            temp[tempIdx].value = table[i].value;
                            tempIdx++;
                        }
                    }

                    // Clear table
                    clear();

                    // Re-insert all entries
                    for (i = 0; i < tempIdx; i++) {
                        insert(temp[i].key, temp[i].value);
                    }

                    memset(temp, 0, count * sizeof(Entry_t));
                    free(temp);
                }
                else {
                    // Slow path: No memory available, do in-place redistribution
                    size_t i, pass;

                    // In-place rehashing using multi-pass redistribution
                    for (pass = 0; pass < 2; pass++) {
                        for (i = 0; i < tableSize; i++) {
                            if (table[i].state == OCCUPIED) {
                                size_t idealIdx = getHash(table[i].key);

                                // If this entry is not in its ideal position
                                if (idealIdx != i) {
                                    size_t targetIdx = idealIdx;

                                    // Find where this entry should go
                                    while (targetIdx != i) {
                                        if (table[targetIdx].state != OCCUPIED) {
                                            // Found an empty/tombstone slot, move entry here
                                            table[targetIdx].key = table[i].key;
                                            table[targetIdx].value = table[i].value;
                                            table[targetIdx].state = OCCUPIED;

                                            table[i].state = TOMBSTONE;
                                            memset(&table[i].key, 0, sizeof(K));
                                            memset(&table[i].value, 0, sizeof(V));

                                            break;
                                        }
                                        // Slot occupied, try next
                                        if ((++targetIdx) >= tableSize) {
                                            targetIdx = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Clean up all tombstones
                    for (i = 0; i < tableSize; i++) {
                        if (table[i].state == TOMBSTONE) {
                            table[i].state = EMPTY;
                        }
                    }
                }

                tombstones = 0;
            }
        };

    size_t getHash(const K& key)
        {
            return hasher(key) % tableSize;
        };

private:
    HashFn hasher;
    Entry_t *table;
    size_t tableSize;
    size_t count;
    size_t tombstones;
};
