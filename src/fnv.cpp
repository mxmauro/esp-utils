#include "fnv.h"

#define FNV1A32_PRIME  16777619u

// -----------------------------------------------------------------------------

uint32_t fnv1a32(const void *data, size_t len, uint32_t hash)
{
    const uint8_t *bytes = (const uint8_t *)data;

    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= FNV1A32_PRIME;
    }
    return hash;
}
