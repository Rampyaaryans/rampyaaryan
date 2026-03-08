/*
 * ============================================================================
 *  RAMPYAARYAN - Virtual Machine Header
 *  Stack-based bytecode VM with garbage collection
 * ============================================================================
 */

#ifndef RAMPYAARYAN_VM_H
#define RAMPYAARYAN_VM_H

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "table.h"
#include "object.h"

/* ============================================================================
 *  CALL FRAME
 * ============================================================================ */
typedef struct {
    ObjClosure* closure;
    uint8_t* ip;        /* Instruction pointer */
    Value* slots;       /* Points into VM's value stack */
} CallFrame;

/* ============================================================================
 *  EXCEPTION HANDLER (for try-catch)
 * ============================================================================ */
#define MAX_EXCEPTION_HANDLERS 64

typedef struct {
    uint8_t* handlerIP;    /* IP to jump to (catch block) */
    int frameIndex;         /* Which call frame the handler is in */
    Value* stackTop;        /* Stack top when try was entered */
} ExceptionHandler;

/* ============================================================================
 *  VM RESULT CODES
 * ============================================================================ */
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

/* ============================================================================
 *  VIRTUAL MACHINE
 * ============================================================================ */
struct VM {
    /* Call stack */
    CallFrame frames[MAX_CALL_FRAMES];
    int frameCount;

    /* Value stack */
    Value stack[STACK_MAX];
    Value* stackTop;

    /* Global variables */
    Table globals;

    /* String interning table */
    Table strings;

    /* Upvalues linked list */
    ObjUpvalue* openUpvalues;

    /* GC: linked list of all objects */
    Obj* objects;
    int grayCount;
    int grayCapacity;
    Obj** grayStack;

    /* GC stats */
    size_t bytesAllocated;
    size_t nextGC;

    /* Error handling */
    char errorMessage[1024];
    bool hadError;

    /* Re-entrant call support */
    int baseFrameCount;

    /* Exception handling */
    ExceptionHandler exceptionHandlers[MAX_EXCEPTION_HANDLERS];
    int exceptionCount;

    /* Constructor name (interned) */
    ObjString* initString;

    /* Module cache (imported files) */
    Table modules;

    /* Current script file path */
    const char* scriptPath;
};

/* ============================================================================
 *  VM LIFECYCLE
 * ============================================================================ */
void initVM(VM* vm);
void freeVM(VM* vm);

/* ============================================================================
 *  EXECUTION
 * ============================================================================ */
InterpretResult interpret(VM* vm, const char* source, const char* filename);
InterpretResult interpretChunk(VM* vm, ObjFunction* function);

/* ============================================================================
 *  STACK OPERATIONS
 * ============================================================================ */
void push(VM* vm, Value value);
Value pop(VM* vm);
Value peek(VM* vm, int distance);

/* ============================================================================
 *  ERROR REPORTING
 * ============================================================================ */
void runtimeError(VM* vm, const char* format, ...);

/* ============================================================================
 *  NATIVE FUNCTION REGISTRATION
 * ============================================================================ */
void defineNative(VM* vm, const char* name, NativeFn function, int arity);

/* ============================================================================
 *  CALLABLE FROM NATIVE FUNCTIONS
 * ============================================================================ */
Value vmCallFunction(VM* vm, Value callee, int argCount, Value* args);

#endif /* RAMPYAARYAN_VM_H */
