/*
 * ============================================================================
 *  RAMPYAARYAN - Object System Implementation
 *  Heap-allocated objects with garbage collection support
 * ============================================================================
 */

#include "object.h"
#include "memory.h"
#include "table.h"
#include "vm.h"
#include "chunk.h"

#include <math.h>

/* ============================================================================
 *  OBJECT ALLOCATION
 * ============================================================================ */
Obj* allocateObject(VM* vm, size_t size, ObjType type) {
    Obj* object = (Obj*)ram_reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;

    /* Add to VM's linked list of all objects (for GC) */
    object->next = vm->objects;
    vm->objects = object;

#ifdef DEBUG_LOG_GC
    printf("[GC] %p allocate %zu for type %d\n", (void*)object, size, type);
#endif

    return object;
}

/* ============================================================================
 *  STRING HASHING (FNV-1a)
 * ============================================================================ */
static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

/* ============================================================================
 *  STRING OBJECT
 * ============================================================================ */
static ObjString* allocateString(VM* vm, char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(vm, ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    /* Intern the string in VM's string table */
    /* Push/pop to protect from GC during table insertion */
    push(vm, OBJ_VAL(string));
    tableSet(&vm->strings, string, NULL_VAL);
    pop(vm);

    return string;
}

ObjString* copyString(VM* vm, const char* chars, int length) {
    uint32_t hash = hashString(chars, length);

    /* Check if already interned */
    ObjString* interned = tableFindString(&vm->strings, chars, length, hash);
    if (interned != NULL) return interned;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(vm, heapChars, length, hash);
}

ObjString* takeString(VM* vm, char* chars, int length) {
    uint32_t hash = hashString(chars, length);

    ObjString* interned = tableFindString(&vm->strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(vm, chars, length, hash);
}

ObjString* concatenateStrings(VM* vm, ObjString* a, ObjString* b) {
    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    return takeString(vm, chars, length);
}

ObjString* repeatString(VM* vm, ObjString* str, int times) {
    if (times <= 0) {
        return copyString(vm, "", 0);
    }
    int length = str->length * times;
    char* chars = ALLOCATE(char, length + 1);
    for (int i = 0; i < times; i++) {
        memcpy(chars + i * str->length, str->chars, str->length);
    }
    chars[length] = '\0';
    return takeString(vm, chars, length);
}

/* ============================================================================
 *  FUNCTION OBJECT
 * ============================================================================ */
ObjFunction* newFunction(VM* vm) {
    ObjFunction* function = ALLOCATE_OBJ(vm, ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->minArity = 0;
    function->isVariadic = false;
    function->upvalueCount = 0;
    function->name = NULL;

    /* Allocate chunk */
    function->chunk = ALLOCATE(Chunk, 1);
    initChunk(function->chunk);

    return function;
}

/* ============================================================================
 *  NATIVE FUNCTION
 * ============================================================================ */
ObjNative* newNative(VM* vm, NativeFn function, const char* name, int arity) {
    ObjNative* native = ALLOCATE_OBJ(vm, ObjNative, OBJ_NATIVE);
    native->function = function;
    native->name = name;
    native->arity = arity;
    return native;
}

/* ============================================================================
 *  UPVALUE
 * ============================================================================ */
ObjUpvalue* newUpvalue(VM* vm, Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(vm, ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->closed = NULL_VAL;
    upvalue->next = NULL;
    return upvalue;
}

/* ============================================================================
 *  CLOSURE
 * ============================================================================ */
ObjClosure* newClosure(VM* vm, ObjFunction* function) {
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(vm, ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

/* ============================================================================
 *  LIST OBJECT
 * ============================================================================ */
ObjList* newList(VM* vm) {
    ObjList* list = ALLOCATE_OBJ(vm, ObjList, OBJ_LIST);
    initValueArray(&list->items);
    return list;
}

void listAppend(VM* vm, ObjList* list, Value value) {
    UNUSED(vm);
    writeValueArray(&list->items, value);
}

Value listGet(ObjList* list, int index) {
    return list->items.values[index];
}

void listSet(ObjList* list, int index, Value value) {
    list->items.values[index] = value;
}

/* ============================================================================
 *  VALUE HASHING (for map keys)
 * ============================================================================ */
uint32_t hashValue(Value value) {
    switch (value.type) {
        case VAL_BOOL: return AS_BOOL(value) ? 3u : 5u;
        case VAL_NULL: return 7u;
        case VAL_NUMBER: {
            double num = AS_NUMBER(value);
            if (num == 0) return 0u;
            uint32_t hash = 2166136261u;
            uint8_t* bytes = (uint8_t*)&num;
            for (size_t i = 0; i < sizeof(double); i++) {
                hash ^= bytes[i];
                hash *= 16777619u;
            }
            return hash;
        }
        case VAL_OBJ: {
            if (IS_STRING(value)) {
                return AS_STRING(value)->hash;
            }
            uintptr_t ptr = (uintptr_t)AS_OBJ(value);
            return (uint32_t)(ptr ^ (ptr >> 16));
        }
        default: return 0u;
    }
}

/* ============================================================================
 *  MAP/DICTIONARY OBJECT
 * ============================================================================ */
ObjMap* newMap(VM* vm) {
    ObjMap* map = ALLOCATE_OBJ(vm, ObjMap, OBJ_MAP);
    map->count = 0;
    map->capacity = 0;
    map->entries = NULL;
    return map;
}

static MapEntry* findMapEntry(MapEntry* entries, int capacity, Value key) {
    uint32_t index = hashValue(key) & (capacity - 1);
    MapEntry* tombstone = NULL;

    for (;;) {
        MapEntry* entry = &entries[index];
        if (!entry->isOccupied) {
            if (!entry->isTombstone) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (valuesEqual(entry->key, key)) {
            return entry;
        }
        index = (index + 1) & (capacity - 1);
    }
}

static void adjustMapCapacity(VM* vm, ObjMap* map, int capacity) {
    MapEntry* entries = ALLOCATE(MapEntry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].isOccupied = false;
        entries[i].isTombstone = false;
        entries[i].key = NULL_VAL;
        entries[i].value = NULL_VAL;
    }

    map->count = 0;
    for (int i = 0; i < map->capacity; i++) {
        MapEntry* entry = &map->entries[i];
        if (!entry->isOccupied) continue;

        MapEntry* dest = findMapEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        dest->isOccupied = true;
        dest->isTombstone = false;
        map->count++;
    }

    FREE_ARRAY(MapEntry, map->entries, map->capacity);
    map->entries = entries;
    map->capacity = capacity;
}

bool mapSet(VM* vm, ObjMap* map, Value key, Value value) {
    if (map->count + 1 > map->capacity * 3 / 4) {
        int capacity = GROW_CAPACITY(map->capacity);
        if (capacity < 8) capacity = 8;
        adjustMapCapacity(vm, map, capacity);
    }

    MapEntry* entry = findMapEntry(map->entries, map->capacity, key);
    bool isNewKey = !entry->isOccupied;
    if (isNewKey && !entry->isTombstone) map->count++;

    entry->key = key;
    entry->value = value;
    entry->isOccupied = true;
    entry->isTombstone = false;
    return isNewKey;
}

bool mapGet(ObjMap* map, Value key, Value* result) {
    if (map->count == 0) return false;

    MapEntry* entry = findMapEntry(map->entries, map->capacity, key);
    if (!entry->isOccupied) return false;

    *result = entry->value;
    return true;
}

bool mapDelete(ObjMap* map, Value key) {
    if (map->count == 0) return false;

    MapEntry* entry = findMapEntry(map->entries, map->capacity, key);
    if (!entry->isOccupied) return false;

    entry->isOccupied = false;
    entry->isTombstone = true;
    map->count--;
    return true;
}

bool mapHasKey(ObjMap* map, Value key) {
    if (map->count == 0) return false;
    MapEntry* entry = findMapEntry(map->entries, map->capacity, key);
    return entry->isOccupied;
}

int mapLength(ObjMap* map) {
    return map->count;
}

/* ============================================================================
 *  CLASS OBJECT
 * ============================================================================ */
ObjClass* newClass(VM* vm, ObjString* name) {
    ObjClass* klass = ALLOCATE_OBJ(vm, ObjClass, OBJ_CLASS);
    klass->name = name;
    klass->superclass = NULL;
    initTable(&klass->methods);
    return klass;
}

/* ============================================================================
 *  INSTANCE OBJECT
 * ============================================================================ */
ObjInstance* newInstance(VM* vm, ObjClass* klass) {
    ObjInstance* instance = ALLOCATE_OBJ(vm, ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

/* ============================================================================
 *  BOUND METHOD
 * ============================================================================ */
ObjBoundMethod* newBoundMethod(VM* vm, Value receiver, ObjClosure* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(vm, ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

/* ============================================================================
 *  PRINT OBJECT
 * ============================================================================ */
void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_FUNCTION: {
            ObjFunction* fn = AS_FUNCTION(value);
            if (fn->name != NULL) {
                printf("<kaam %s>", fn->name->chars);
            } else {
                printf("<script>");
            }
            break;
        }
        case OBJ_NATIVE: {
            ObjNative* native = AS_NATIVE(value);
            printf("<built-in %s>", native->name);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = AS_CLOSURE(value);
            if (closure->function->name != NULL) {
                printf("<kaam %s>", closure->function->name->chars);
            } else {
                printf("<script>");
            }
            break;
        }
        case OBJ_UPVALUE:
            printf("<upvalue>");
            break;
        case OBJ_LIST: {
            ObjList* list = AS_LIST(value);
            printf("[");
            for (int i = 0; i < list->items.count; i++) {
                if (i > 0) printf(", ");
                printValueRepr(list->items.values[i]);
            }
            printf("]");
            break;
        }
        case OBJ_MAP: {
            ObjMap* map = AS_MAP(value);
            printf("{");
            bool first = true;
            for (int i = 0; i < map->capacity; i++) {
                if (!map->entries[i].isOccupied) continue;
                if (!first) printf(", ");
                first = false;
                printValueRepr(map->entries[i].key);
                printf(": ");
                printValueRepr(map->entries[i].value);
            }
            printf("}");
            break;
        }
        case OBJ_CLASS: {
            ObjClass* klass = AS_CLASS(value);
            printf("<kaksha %s>", klass->name->chars);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = AS_INSTANCE(value);
            printf("<%s instance>", instance->klass->name->chars);
            break;
        }
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* bound = AS_BOUND_METHOD(value);
            if (bound->method->function->name != NULL) {
                printf("<bound %s>", bound->method->function->name->chars);
            } else {
                printf("<bound method>");
            }
            break;
        }
    }
}

/* ============================================================================
 *  FREE OBJECT
 * ============================================================================ */
void freeObject(VM* vm, Obj* object) {
    UNUSED(vm);

#ifdef DEBUG_LOG_GC
    printf("[GC] %p free type %d\n", (void*)object, object->type);
#endif

    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(function->chunk);
            FREE(Chunk, function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_NATIVE: {
            FREE(ObjNative, object);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_UPVALUE: {
            FREE(ObjUpvalue, object);
            break;
        }
        case OBJ_LIST: {
            ObjList* list = (ObjList*)object;
            freeValueArray(&list->items);
            FREE(ObjList, object);
            break;
        }
        case OBJ_MAP: {
            ObjMap* map = (ObjMap*)object;
            FREE_ARRAY(MapEntry, map->entries, map->capacity);
            FREE(ObjMap, object);
            break;
        }
        case OBJ_CLASS: {
            ObjClass* klass = (ObjClass*)object;
            freeTable(&klass->methods);
            FREE(ObjClass, object);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            freeTable(&instance->fields);
            FREE(ObjInstance, object);
            break;
        }
        case OBJ_BOUND_METHOD: {
            FREE(ObjBoundMethod, object);
            break;
        }
    }
}
