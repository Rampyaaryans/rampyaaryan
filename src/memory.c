/*
 * ============================================================================
 *  RAMPYAARYAN - Memory Management Implementation
 *  Custom allocator with mark-sweep garbage collector
 * ============================================================================
 */

#include "memory.h"
#include "vm.h"
#include "object.h"
#include "table.h"
#include "compiler.h"

#ifdef DEBUG_LOG_GC
#include "debug.h"
#endif

/* Global memory stats */
MemoryStats memStats = {0, 1024 * 1024, 0, 0};

/* The current VM instance for GC (set during interpretation) */
static VM* currentVM = NULL;

void setCurrentVM(VM* vm) {
    currentVM = vm;
}

/* ============================================================================
 *  CORE ALLOCATOR
 *  Every single memory allocation/deallocation goes through this function
 * ============================================================================ */
void* ram_reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (currentVM != NULL) {
        currentVM->bytesAllocated += newSize - oldSize;
    }
    memStats.bytesAllocated += newSize - oldSize;

    if (newSize > oldSize) {
        memStats.totalAllocations++;

#ifdef DEBUG_STRESS_GC
        if (currentVM != NULL) {
            ram_collect_garbage(currentVM);
        }
#endif

        if (currentVM != NULL && currentVM->bytesAllocated > currentVM->nextGC) {
            ram_collect_garbage(currentVM);
        }
    }

    if (newSize == 0) {
        memStats.totalFrees++;
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) {
        fprintf(stderr, "Galti: Memory allocate nahi ho paayi! (%zu bytes)\n", newSize);
        exit(1);
    }
    return result;
}

/* ============================================================================
 *  GARBAGE COLLECTOR - Mark Phase
 * ============================================================================ */

static void markValue(VM* vm, Value value) {
    if (IS_OBJ(value)) {
        Obj* object = AS_OBJ(value);
        if (object == NULL) return;
        if (object->isMarked) return;

#ifdef DEBUG_LOG_GC
        printf("  [GC] %p mark ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
#endif

        object->isMarked = true;

        /* Add to gray stack for processing */
        if (vm->grayCount + 1 > vm->grayCapacity) {
            vm->grayCapacity = GROW_CAPACITY(vm->grayCapacity);
            vm->grayStack = (Obj**)realloc(vm->grayStack,
                sizeof(Obj*) * vm->grayCapacity);
            if (vm->grayStack == NULL) {
                fprintf(stderr, "Galti: GC gray stack allocate nahi ho paayi!\n");
                exit(1);
            }
        }
        vm->grayStack[vm->grayCount++] = object;
    }
}

static void markArray(VM* vm, ValueArray* array) {
    for (int i = 0; i < array->count; i++) {
        markValue(vm, array->values[i]);
    }
}

static void blackenObject(VM* vm, Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("  [GC] %p blacken ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    switch (object->type) {
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markValue(vm, OBJ_VAL(closure->function));
            for (int i = 0; i < closure->upvalueCount; i++) {
                if (closure->upvalues[i] != NULL) {
                    markValue(vm, OBJ_VAL(closure->upvalues[i]));
                }
            }
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            if (function->name != NULL) {
                markValue(vm, OBJ_VAL(function->name));
            }
            markArray(vm, &function->chunk->constants);
            break;
        }
        case OBJ_UPVALUE:
            markValue(vm, ((ObjUpvalue*)object)->closed);
            break;
        case OBJ_LIST: {
            ObjList* list = (ObjList*)object;
            markArray(vm, &list->items);
            break;
        }
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

static void markRoots(VM* vm) {
    /* Mark stack values */
    for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
        markValue(vm, *slot);
    }

    /* Mark call frame closures */
    for (int i = 0; i < vm->frameCount; i++) {
        markValue(vm, OBJ_VAL(vm->frames[i].closure));
    }

    /* Mark open upvalues */
    for (ObjUpvalue* upvalue = vm->openUpvalues;
         upvalue != NULL;
         upvalue = upvalue->next) {
        markValue(vm, OBJ_VAL(upvalue));
    }

    /* Mark globals */
    markTable(vm, &vm->globals);

    /* Mark compiler roots (if compiling) */
    markCompilerRoots(vm);
}

static void traceReferences(VM* vm) {
    while (vm->grayCount > 0) {
        Obj* object = vm->grayStack[--vm->grayCount];
        blackenObject(vm, object);
    }
}

/* ============================================================================
 *  GARBAGE COLLECTOR - Sweep Phase
 * ============================================================================ */
static void sweep(VM* vm) {
    Obj* previous = NULL;
    Obj* object = vm->objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                vm->objects = object;
            }

#ifdef DEBUG_LOG_GC
            printf("  [GC] %p free type %d\n", (void*)unreached, unreached->type);
#endif
            freeObject(vm, unreached);
        }
    }
}

/* ============================================================================
 *  GARBAGE COLLECTOR - Main Entry Point
 * ============================================================================ */
void ram_collect_garbage(VM* vm) {
    if (vm == NULL) return;

#ifdef DEBUG_LOG_GC
    printf("-- GC begin\n");
    size_t before = vm->bytesAllocated;
#endif

    markRoots(vm);
    traceReferences(vm);
    tableRemoveWhite(&vm->strings);
    sweep(vm);

    vm->nextGC = vm->bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- GC end. Collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - vm->bytesAllocated, before, vm->bytesAllocated, vm->nextGC);
#endif
}

/* Free ALL objects (on VM shutdown) */
void ram_free_objects(VM* vm) {
    Obj* object = vm->objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(vm, object);
        object = next;
    }
    free(vm->grayStack);
    vm->grayStack = NULL;
}
