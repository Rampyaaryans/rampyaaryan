/*
 * ============================================================================
 *  RAMPYAARYAN - Hash Table Header
 * ============================================================================
 */

#ifndef RAMPYAARYAN_TABLE_H
#define RAMPYAARYAN_TABLE_H

#include "common.h"
#include "value.h"

/* Forward declaration to avoid circular includes */
typedef struct VM VM;

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);
void tableRemoveWhite(Table* table);
void markTable(VM* vm, Table* table);

#endif /* RAMPYAARYAN_TABLE_H */
