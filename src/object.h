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
#include "table.h"

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
    OBJ_MAP,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_BOUND_METHOD,
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
#define IS_MAP(value)       isObjType(value, OBJ_MAP)
#define IS_CLASS(value)     isObjType(value, OBJ_CLASS)
#define IS_INSTANCE(value)  isObjType(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)

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
    int minArity;
    bool isVariadic;
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
 *  MAP/DICTIONARY OBJECT
 * ============================================================================ */
typedef struct {
    Value key;
    Value value;
    bool isOccupied;
    bool isTombstone;
} MapEntry;

struct ObjMap {
    Obj obj;
    int count;
    int capacity;
    MapEntry* entries;
};

#define AS_MAP(value)       ((ObjMap*)AS_OBJ(value))

ObjMap* newMap(VM* vm);
bool mapSet(VM* vm, ObjMap* map, Value key, Value value);
bool mapGet(ObjMap* map, Value key, Value* result);
bool mapDelete(ObjMap* map, Value key);
bool mapHasKey(ObjMap* map, Value key);
int mapLength(ObjMap* map);
uint32_t hashValue(Value value);

/* ============================================================================
 *  CLASS OBJECT
 * ============================================================================ */
typedef struct ObjClass {
    Obj obj;
    ObjString* name;
    Table methods;
    struct ObjClass* superclass;
} ObjClass;

#define AS_CLASS(value)     ((ObjClass*)AS_OBJ(value))

ObjClass* newClass(VM* vm, ObjString* name);

/* ============================================================================
 *  INSTANCE OBJECT
 * ============================================================================ */
typedef struct {
    Obj obj;
    ObjClass* klass;
    Table fields;
} ObjInstance;

#define AS_INSTANCE(value)  ((ObjInstance*)AS_OBJ(value))

ObjInstance* newInstance(VM* vm, ObjClass* klass);

/* ============================================================================
 *  BOUND METHOD
 * ============================================================================ */
typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure* method;
} ObjBoundMethod;

#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))

ObjBoundMethod* newBoundMethod(VM* vm, Value receiver, ObjClosure* method);

/* ============================================================================
 *  OBJECT UTILITIES
 * ============================================================================ */
Obj* allocateObject(VM* vm, size_t size, ObjType type);
void printObject(Value value);
void freeObject(VM* vm, Obj* object);

#define ALLOCATE_OBJ(vm, type, objectType) \
    (type*)allocateObject(vm, sizeof(type), objectType)

#endif /* RAMPYAARYAN_OBJECT_H */
