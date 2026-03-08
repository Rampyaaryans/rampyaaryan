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
    }
}
