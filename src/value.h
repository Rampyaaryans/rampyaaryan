/*
 * ============================================================================
 *  RAMPYAARYAN - Value System Header
 *  Handles all value types: numbers, booleans, null, and object pointers
 * ============================================================================
 */

#ifndef RAMPYAARYAN_VALUE_H
#define RAMPYAARYAN_VALUE_H

#include "common.h"

/* Forward declarations */
typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjList ObjList;
typedef struct ObjFunction ObjFunction;

/* ============================================================================
 *  VALUE TYPES
 * ============================================================================ */
typedef enum {
    VAL_BOOL,
    VAL_NULL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

/* Tagged union for values */
typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

/* ============================================================================
 *  TYPE CHECK MACROS
 * ============================================================================ */
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NULL(value)    ((value).type == VAL_NULL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

/* ============================================================================
 *  VALUE EXTRACTION MACROS
 * ============================================================================ */
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)

/* ============================================================================
 *  VALUE CREATION MACROS
 * ============================================================================ */
#define BOOL_VAL(value)   ((Value){VAL_BOOL,   {.boolean = (value)}})
#define NULL_VAL          ((Value){VAL_NULL,    {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER,  {.number = (value)}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ,    {.obj = (Obj*)(object)}})

/* ============================================================================
 *  DYNAMIC ARRAY OF VALUES (constant pool)
 * ============================================================================ */
typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

/* ============================================================================
 *  VALUE OPERATIONS
 * ============================================================================ */
bool valuesEqual(Value a, Value b);
void printValue(Value value);
void printValueRepr(Value value); /* With quotes for strings */
char* valueToString(Value value); /* Caller must free! */
const char* valueTypeName(Value value);
bool isTruthy(Value value);

#endif /* RAMPYAARYAN_VALUE_H */
