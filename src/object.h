/*
 * ============================================================================
 *  RAMPYAARYAN - Object System Header
 *  Heap-allocated objects: strings, functions, closures, lists
 * ============================================================================
 */

#ifndef RAMPYAARYAN_OBJECT_H
#define RAMPYAARYAN_OBJECT_H

#include "common.h"
#include "value.h"

/* Forward declarations */
typedef struct VM VM;
typedef struct Chunk Chunk;

/* ============================================================================
 *  OBJECT TYPES
 * ============================================================================ */
typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
    OBJ_LIST,
} ObjType;

/* ============================================================================
 *  BASE OBJECT (all objects start with this)
 * ============================================================================ */
struct Obj {
    ObjType type;
    bool isMarked;  /* For GC */
    struct Obj* next; /* Intrusive linked list for GC */
};

/* ============================================================================
 *  OBJECT TYPE CHECKS
 * ============================================================================ */
#define OBJ_TYPE(value)     (AS_OBJ(value)->type)
#define IS_STRING(value)    isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)  isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)    isObjType(value, OBJ_NATIVE)
#define IS_CLOSURE(value)   isObjType(value, OBJ_CLOSURE)
#define IS_LIST(value)      isObjType(value, OBJ_LIST)

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

/* ============================================================================
 *  STRING OBJECT
 * ============================================================================ */
struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

#define AS_STRING(value)    ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString*)AS_OBJ(value))->chars)

ObjString* copyString(VM* vm, const char* chars, int length);
ObjString* takeString(VM* vm, char* chars, int length);
ObjString* concatenateStrings(VM* vm, ObjString* a, ObjString* b);
ObjString* repeatString(VM* vm, ObjString* str, int times);

/* ============================================================================
 *  FUNCTION OBJECT
 * ============================================================================ */
struct ObjFunction {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk* chunk;
    ObjString* name;
};

#define AS_FUNCTION(value)  ((ObjFunction*)AS_OBJ(value))

ObjFunction* newFunction(VM* vm);

/* ============================================================================
 *  NATIVE FUNCTION
 * ============================================================================ */
typedef Value (*NativeFn)(VM* vm, int argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
    const char* name;
    int arity; /* -1 = variadic */
} ObjNative;

#define AS_NATIVE(value)    ((ObjNative*)AS_OBJ(value))
#define AS_NATIVE_FN(value) (((ObjNative*)AS_OBJ(value))->function)

ObjNative* newNative(VM* vm, NativeFn function, const char* name, int arity);

/* ============================================================================
 *  UPVALUE (for closures)
 * ============================================================================ */
typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

ObjUpvalue* newUpvalue(VM* vm, Value* slot);

/* ============================================================================
 *  CLOSURE
 * ============================================================================ */
typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

#define AS_CLOSURE(value)   ((ObjClosure*)AS_OBJ(value))

ObjClosure* newClosure(VM* vm, ObjFunction* function);

/* ============================================================================
 *  LIST OBJECT
 * ============================================================================ */
struct ObjList {
    Obj obj;
    ValueArray items;
};

#define AS_LIST(value)      ((ObjList*)AS_OBJ(value))

ObjList* newList(VM* vm);
void listAppend(VM* vm, ObjList* list, Value value);
Value listGet(ObjList* list, int index);
void listSet(ObjList* list, int index, Value value);

/* ============================================================================
 *  OBJECT UTILITIES
 * ============================================================================ */
Obj* allocateObject(VM* vm, size_t size, ObjType type);
void printObject(Value value);
void freeObject(VM* vm, Obj* object);

#define ALLOCATE_OBJ(vm, type, objectType) \
    (type*)allocateObject(vm, sizeof(type), objectType)

#endif /* RAMPYAARYAN_OBJECT_H */
