#ifndef saffron_memory_h
#define saffron_memory_h

#include "common.h"
#include "value.h"

// Growth factor MUST be a power of 2 for optimized lookups
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);
void markValue(Value value);
void markArray(ValueArray *array);
void markObject(Obj* object);
void collectGarbage();
void freeObjects();
void freeNodes();

#endif