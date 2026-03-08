/*
 * ============================================================================
 *  RAMPYAARYAN - Memory Management Header
 * ============================================================================
 */

#ifndef RAMPYAARYAN_MEMORY_H
#define RAMPYAARYAN_MEMORY_H

#include "common.h"

/* Forward declarations */
typedef struct Obj Obj;
typedef struct VM VM;

/* ============================================================================
 *  MEMORY ALLOCATION
 * ============================================================================ */
#define ALLOCATE(type, count) \
    (type*)ram_reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) \
    ram_reallocate(pointer, sizeof(type), 0)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)ram_reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    ram_reallocate(pointer, sizeof(type) * (oldCount), 0)

/* Core allocation function - all memory goes through this */
void* ram_reallocate(void* pointer, size_t oldSize, size_t newSize);

/* Garbage collector */
void ram_collect_garbage(VM* vm);
void ram_free_objects(VM* vm);

/* Memory tracking */
typedef struct {
    size_t bytesAllocated;
    size_t nextGC;
    size_t totalAllocations;
    size_t totalFrees;
} MemoryStats;

extern MemoryStats memStats;

#endif /* RAMPYAARYAN_MEMORY_H */
