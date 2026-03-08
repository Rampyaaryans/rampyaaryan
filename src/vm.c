/*
 * ============================================================================
 *  RAMPYAARYAN - Virtual Machine Implementation
 *  Stack-based bytecode VM with garbage collection
 * ============================================================================
 */

#include "vm.h"
#include "compiler.h"
#include "memory.h"
#include "native.h"
#include "object.h"
#include "debug.h"

/* From memory.c */
extern void setCurrentVM(VM* vm);

/* ============================================================================
 *  STACK OPERATIONS
 * ============================================================================ */
void push(VM* vm, Value value) {
    if (vm->stackTop >= vm->stack + STACK_MAX) {
        fprintf(stderr, "Stack overflow! Bahut zyada recursion ho raha hai.\n");
        exit(1);
    }
    *vm->stackTop = value;
    vm->stackTop++;
}

Value pop(VM* vm) {
    vm->stackTop--;
    return *vm->stackTop;
}

Value peek(VM* vm, int distance) {
    return vm->stackTop[-1 - distance];
}

static void resetStack(VM* vm) {
    vm->stackTop = vm->stack;
    vm->frameCount = 0;
    vm->openUpvalues = NULL;
}

/* ============================================================================
 *  ERROR REPORTING
 * ============================================================================ */
void runtimeError(VM* vm, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(vm->errorMessage, sizeof(vm->errorMessage), format, args);
    va_end(args);

    fprintf(stderr, "\n  \xe2\x9d\x8c Runtime Galti: %s\n", vm->errorMessage);

    /* Print stack trace */
    for (int i = vm->frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm->frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk->code - 1;
        int line = function->chunk->lines[instruction];
        fprintf(stderr, "    [Line %d] ", line);
        if (function->name == NULL) {
            fprintf(stderr, "main script mein\n");
        } else {
            fprintf(stderr, "kaam %s() mein\n", function->name->chars);
        }
    }

    resetStack(vm);
    vm->hadError = true;
}

/* ============================================================================
 *  VM LIFECYCLE
 * ============================================================================ */
void initVM(VM* vm) {
    resetStack(vm);
    vm->objects = NULL;
    vm->bytesAllocated = 0;
    vm->nextGC = 1024 * 1024; /* First GC at 1MB */
    vm->grayCount = 0;
    vm->grayCapacity = 0;
    vm->grayStack = NULL;
    vm->hadError = false;
    vm->baseFrameCount = 0;
    vm->exceptionCount = 0;

    initTable(&vm->globals);
    initTable(&vm->strings);
    initTable(&vm->modules);
    vm->scriptPath = NULL;

    setCurrentVM(vm);

    /* Intern the constructor name */
    vm->initString = copyString(vm, "shuru", 5);

    /* Register built-in functions */
    registerNatives(vm);
}

void freeVM(VM* vm) {
    freeTable(&vm->globals);
    freeTable(&vm->strings);
    freeTable(&vm->modules);
    ram_free_objects(vm);
    setCurrentVM(NULL);
}

/* ============================================================================
 *  NATIVE FUNCTION REGISTRATION
 * ============================================================================ */
void defineNative(VM* vm, const char* name, NativeFn function, int arity) {
    ObjString* nameStr = copyString(vm, name, (int)strlen(name));
    push(vm, OBJ_VAL(nameStr));
    ObjNative* native = newNative(vm, function, name, arity);
    push(vm, OBJ_VAL(native));
    tableSet(&vm->globals, AS_STRING(vm->stack[0]), vm->stack[1]);
    pop(vm);
    pop(vm);
}

/* ============================================================================
 *  CLOSURE/UPVALUE HELPERS
 * ============================================================================ */
static ObjUpvalue* captureUpvalue(VM* vm, Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm->openUpvalues;

    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(vm, local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm->openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(VM* vm, Value* last) {
    while (vm->openUpvalues != NULL &&
           vm->openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm->openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->openUpvalues = upvalue->next;
    }
}

/* ============================================================================
 *  FILE READING (for module imports)
 * ============================================================================ */
static char* vmReadFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) return NULL;

    fseek(file, 0L, SEEK_END);
    size_t fileSize = (size_t)ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) { fclose(file); return NULL; }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

/* Resolve relative import path against the current script's directory */
static char* resolveModulePath(const char* basePath, const char* importPath) {
    if (importPath[0] == '/' || importPath[0] == '\\' ||
        (importPath[0] != '\0' && importPath[1] == ':')) {
        /* Absolute path — use as-is */
        size_t len = strlen(importPath);
        char* result = (char*)malloc(len + 1);
        memcpy(result, importPath, len + 1);
        return result;
    }

    /* Relative: resolve from basePath's directory */
    if (basePath == NULL) {
        size_t len = strlen(importPath);
        char* result = (char*)malloc(len + 1);
        memcpy(result, importPath, len + 1);
        return result;
    }

    /* Find last slash in basePath */
    const char* lastSlash = basePath;
    for (const char* p = basePath; *p; p++) {
        if (*p == '/' || *p == '\\') lastSlash = p;
    }

    if (lastSlash == basePath && *lastSlash != '/' && *lastSlash != '\\') {
        /* No directory component */
        size_t len = strlen(importPath);
        char* result = (char*)malloc(len + 1);
        memcpy(result, importPath, len + 1);
        return result;
    }

    size_t dirLen = (size_t)(lastSlash - basePath + 1);
    size_t importLen = strlen(importPath);
    char* result = (char*)malloc(dirLen + importLen + 1);
    memcpy(result, basePath, dirLen);
    memcpy(result + dirLen, importPath, importLen + 1);
    return result;
}

/* ============================================================================
 *  FUNCTION CALL
 * ============================================================================ */
static bool callValue(VM* vm, Value callee, int argCount);

static bool callClosure(VM* vm, ObjClosure* closure, int argCount) {
    int minArity = closure->function->minArity;
    int maxArity = closure->function->arity;
    bool isVariadic = closure->function->isVariadic;

    if (isVariadic) {
        /* Variadic: need at least minArity args (excluding the rest param) */
        int requiredFixed = maxArity - 1;  /* last param is the rest list */
        if (argCount < minArity) {
            runtimeError(vm, "'%s' ko kam se kam %d arguments chahiye, lekin %d diye gaye.",
                closure->function->name ? closure->function->name->chars : "script",
                minArity, argCount);
            return false;
        }

        /* Collect extra args into a list */
        int extraCount = argCount - requiredFixed;
        if (extraCount < 0) extraCount = 0;
        ObjList* restList = newList(vm);
        /* The extra args are at the top of the stack */
        Value* extraStart = vm->stackTop - extraCount;
        for (int i = 0; i < extraCount; i++) {
            writeValueArray(&restList->items, extraStart[i]);
        }
        /* Pop the extra args and push the list */
        vm->stackTop -= extraCount;

        /* Push nulls for any missing optional fixed params */
        int fixedProvided = argCount < requiredFixed ? argCount : requiredFixed;
        for (int i = fixedProvided; i < requiredFixed; i++) {
            push(vm, NULL_VAL);
        }

        push(vm, OBJ_VAL(restList)); /* the rest parameter */

        if (vm->frameCount == MAX_CALL_FRAMES) {
            runtimeError(vm, "Stack overflow! Bahut zyada function calls (recursion check karo).");
            return false;
        }

        CallFrame* frame = &vm->frames[vm->frameCount++];
        frame->closure = closure;
        frame->ip = closure->function->chunk->code;
        frame->slots = vm->stackTop - maxArity - 1;
        return true;
    }

    /* Non-variadic */
    if (argCount < minArity || argCount > maxArity) {
        if (minArity == maxArity) {
            runtimeError(vm, "'%s' ko %d arguments chahiye, lekin %d diye gaye.",
                closure->function->name ? closure->function->name->chars : "script",
                maxArity, argCount);
        } else {
            runtimeError(vm, "'%s' ko %d se %d arguments chahiye, lekin %d diye gaye.",
                closure->function->name ? closure->function->name->chars : "script",
                minArity, maxArity, argCount);
        }
        return false;
    }

    /* Push null for missing default parameters */
    for (int i = argCount; i < maxArity; i++) {
        push(vm, NULL_VAL);
    }

    if (vm->frameCount == MAX_CALL_FRAMES) {
        runtimeError(vm, "Stack overflow! Bahut zyada function calls (recursion check karo).");
        return false;
    }

    CallFrame* frame = &vm->frames[vm->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk->code;
    frame->slots = vm->stackTop - maxArity - 1;
    return true;
}

static bool callValue(VM* vm, Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_CLOSURE:
                return callClosure(vm, AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE: {
                ObjNative* native = AS_NATIVE(callee);
                if (native->arity != -1 && argCount != native->arity) {
                    runtimeError(vm, "'%s' ko %d arguments chahiye, lekin %d diye gaye.",
                        native->name, native->arity, argCount);
                    return false;
                }
                Value result = native->function(vm, argCount, vm->stackTop - argCount);
                if (vm->hadError) return false;
                vm->stackTop -= argCount + 1;
                push(vm, result);
                return true;
            }
            case OBJ_CLASS: {
                ObjClass* klass = AS_CLASS(callee);
                vm->stackTop[-argCount - 1] = OBJ_VAL(newInstance(vm, klass));
                Value initializer;
                if (tableGet(&klass->methods, vm->initString, &initializer)) {
                    return callClosure(vm, AS_CLOSURE(initializer), argCount);
                } else if (argCount != 0) {
                    runtimeError(vm, "'%s' ko 0 arguments chahiye lekin %d diye gaye.",
                        klass->name->chars, argCount);
                    return false;
                }
                return true;
            }
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm->stackTop[-argCount - 1] = bound->receiver;
                return callClosure(vm, bound->method, argCount);
            }
            default:
                break;
        }
    }
    runtimeError(vm, "Sirf functions ko call kar sakte ho.");
    return false;
}

/* ============================================================================
 *  OOP HELPERS
 * ============================================================================ */
static bool bindMethod(VM* vm, ObjClass* klass, ObjString* name) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        return false;
    }
    ObjBoundMethod* bound = newBoundMethod(vm, peek(vm, 0), AS_CLOSURE(method));
    pop(vm);
    push(vm, OBJ_VAL(bound));
    return true;
}

static bool invokeFromClass(VM* vm, ObjClass* klass, ObjString* name, int argCount) {
    Value method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeError(vm, "Method '%s' nahi mila.", name->chars);
        return false;
    }
    return callClosure(vm, AS_CLOSURE(method), argCount);
}

static bool invoke(VM* vm, ObjString* name, int argCount) {
    Value receiver = peek(vm, argCount);

    if (!IS_INSTANCE(receiver)) {
        runtimeError(vm, "Sirf instances ke methods call kar sakte ho.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    /* Check fields first (might be a callable stored in a field) */
    Value value;
    if (tableGet(&instance->fields, name, &value)) {
        vm->stackTop[-argCount - 1] = value;
        return callValue(vm, value, argCount);
    }

    return invokeFromClass(vm, instance->klass, name, argCount);
}

static void defineMethod(VM* vm, ObjString* name) {
    Value method = peek(vm, 0);
    ObjClass* klass = AS_CLASS(peek(vm, 1));
    tableSet(&klass->methods, name, method);
    pop(vm);
}

/* ============================================================================
 *  MAIN EXECUTION LOOP
 * ============================================================================ */
static InterpretResult run(VM* vm) {
    CallFrame* frame = &vm->frames[vm->frameCount - 1];

/* Macros for the dispatch loop */
#define READ_BYTE()     (*frame->ip++)
#define READ_SHORT()    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk->constants.values[READ_BYTE()])
#define READ_STRING()   AS_STRING(READ_CONSTANT())
#define READ_CONSTANT_LONG() (frame->ip += 2, frame->closure->function->chunk->constants.values[(uint16_t)(frame->ip[-2] | (frame->ip[-1] << 8))])
#define READ_STRING_LONG()   AS_STRING(READ_CONSTANT_LONG())

#define BINARY_OP(valueType, op)                                    \
    do {                                                            \
        if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {   \
            runtimeError(vm, "Dono values numbers honi chahiye "    \
                "'%s' operator ke liye.", #op);                     \
            return INTERPRET_RUNTIME_ERROR;                         \
        }                                                           \
        double b = AS_NUMBER(pop(vm));                              \
        double a = AS_NUMBER(pop(vm));                              \
        push(vm, valueType(a op b));                                \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        /* Print stack */
        printf("          ");
        for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(frame->closure->function->chunk,
            (int)(frame->ip - frame->closure->function->chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {

            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(vm, constant);
                break;
            }

            case OP_CONSTANT_LONG: {
                uint8_t lo = READ_BYTE();
                uint8_t hi = READ_BYTE();
                int index = lo | (hi << 8);
                push(vm, frame->closure->function->chunk->constants.values[index]);
                break;
            }

            case OP_NULL:  push(vm, NULL_VAL); break;
            case OP_TRUE:  push(vm, BOOL_VAL(true)); break;
            case OP_FALSE: push(vm, BOOL_VAL(false)); break;

            case OP_POP: pop(vm); break;

            case OP_POPN: {
                uint8_t n = READ_BYTE();
                vm->stackTop -= n;
                break;
            }

            case OP_DUP: push(vm, peek(vm, 0)); break;

            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(&vm->globals, name, peek(vm, 0));
                pop(vm);
                break;
            }

            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm->globals, name, &value)) {
                    runtimeError(vm, "Variable '%s' nahi mila. Pehle 'maano' se declare karo.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, value);
                break;
            }

            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (tableSet(&vm->globals, name, peek(vm, 0))) {
                    tableDelete(&vm->globals, name);
                    runtimeError(vm, "Variable '%s' pehle se defined nahi hai.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_DEFINE_GLOBAL_LONG: {
                ObjString* name = READ_STRING_LONG();
                tableSet(&vm->globals, name, peek(vm, 0));
                pop(vm);
                break;
            }

            case OP_GET_GLOBAL_LONG: {
                ObjString* name = READ_STRING_LONG();
                Value value;
                if (!tableGet(&vm->globals, name, &value)) {
                    runtimeError(vm, "Variable '%s' nahi mila. Pehle 'maano' se declare karo.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, value);
                break;
            }

            case OP_SET_GLOBAL_LONG: {
                ObjString* name = READ_STRING_LONG();
                if (tableSet(&vm->globals, name, peek(vm, 0))) {
                    tableDelete(&vm->globals, name);
                    runtimeError(vm, "Variable '%s' pehle se defined nahi hai.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm, frame->slots[slot]);
                break;
            }

            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(vm, 0);
                break;
            }

            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(vm, *frame->closure->upvalues[slot]->location);
                break;
            }

            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(vm, 0);
                break;
            }

            /* ---- ARITHMETIC ---- */
            case OP_ADD: {
                Value b = peek(vm, 0);
                Value a = peek(vm, 1);

                if (IS_STRING(a) && IS_STRING(b)) {
                    ObjString* sb = AS_STRING(b);
                    ObjString* sa = AS_STRING(a);
                    ObjString* result = concatenateStrings(vm, sa, sb);
                    pop(vm); pop(vm);
                    push(vm, OBJ_VAL(result));
                } else if (IS_NUMBER(a) && IS_NUMBER(b)) {
                    double nb = AS_NUMBER(pop(vm));
                    double na = AS_NUMBER(pop(vm));
                    push(vm, NUMBER_VAL(na + nb));
                } else if (IS_LIST(a) && IS_LIST(b)) {
                    ObjList* lb = AS_LIST(b);
                    ObjList* la = AS_LIST(a);
                    ObjList* result = newList(vm);
                    for (int i = 0; i < la->items.count; i++)
                        listAppend(vm, result, la->items.values[i]);
                    for (int i = 0; i < lb->items.count; i++)
                        listAppend(vm, result, lb->items.values[i]);
                    pop(vm); pop(vm);
                    push(vm, OBJ_VAL(result));
                } else if (IS_STRING(a) || IS_STRING(b)) {
                    /* Auto-convert to string and concatenate */
                    char* sa = valueToString(a);
                    char* sb = valueToString(b);
                    if (sa && sb) {
                        int len = (int)(strlen(sa) + strlen(sb));
                        char* buf = ALLOCATE(char, len + 1);
                        strcpy(buf, sa);
                        strcat(buf, sb);
                        ObjString* result = takeString(vm, buf, len);
                        free(sa); free(sb);
                        pop(vm); pop(vm);
                        push(vm, OBJ_VAL(result));
                    } else {
                        if (sa) free(sa);
                        if (sb) free(sb);
                        runtimeError(vm, "Jod nahi kar sakte in values ko.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                } else {
                    runtimeError(vm, "'+' sirf numbers, strings, ya lists ke beech kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;

            case OP_MULTIPLY: {
                Value b = peek(vm, 0);
                Value a = peek(vm, 1);
                if (IS_STRING(a) && IS_NUMBER(b)) {
                    int times = (int)AS_NUMBER(b);
                    ObjString* str = AS_STRING(a);
                    ObjString* result = repeatString(vm, str, times);
                    pop(vm); pop(vm);
                    push(vm, OBJ_VAL(result));
                } else if (IS_NUMBER(a) && IS_STRING(b)) {
                    int times = (int)AS_NUMBER(a);
                    ObjString* str = AS_STRING(b);
                    ObjString* result = repeatString(vm, str, times);
                    pop(vm); pop(vm);
                    push(vm, OBJ_VAL(result));
                } else if (IS_NUMBER(a) && IS_NUMBER(b)) {
                    double nb = AS_NUMBER(pop(vm));
                    double na = AS_NUMBER(pop(vm));
                    push(vm, NUMBER_VAL(na * nb));
                } else {
                    runtimeError(vm, "'*' sirf numbers ke beech ya string repeat ke liye kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_DIVIDE: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    runtimeError(vm, "'/' sirf numbers ke beech kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop(vm));
                if (b == 0) {
                    runtimeError(vm, "Zero se divide nahi kar sakte!");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double a = AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL(a / b));
                break;
            }

            case OP_MODULO: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    runtimeError(vm, "'%%' sirf numbers ke beech kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop(vm));
                if (b == 0) {
                    runtimeError(vm, "Zero se modulo nahi kar sakte!");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double a = AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL(fmod(a, b)));
                break;
            }

            case OP_POWER: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    runtimeError(vm, "'**' sirf numbers ke beech kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop(vm));
                double a = AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL(pow(a, b)));
                break;
            }

            case OP_NEGATE: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    runtimeError(vm, "Minus sirf numbers ke saath kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
                break;
            }

            /* ---- COMPARISON ---- */
            case OP_EQUAL: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, BOOL_VAL(valuesEqual(a, b)));
                break;
            }

            case OP_NOT_EQUAL: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, BOOL_VAL(!valuesEqual(a, b)));
                break;
            }

            case OP_GREATER:       BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:          BINARY_OP(BOOL_VAL, <); break;
            case OP_GREATER_EQUAL: BINARY_OP(BOOL_VAL, >=); break;
            case OP_LESS_EQUAL:    BINARY_OP(BOOL_VAL, <=); break;

            case OP_NOT: {
                Value val = pop(vm);
                push(vm, BOOL_VAL(!isTruthy(val)));
                break;
            }

            /* ---- CONTROL FLOW ---- */
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }

            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (!isTruthy(peek(vm, 0))) {
                    frame->ip += offset;
                }
                break;
            }

            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }

            /* ---- FUNCTIONS ---- */
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!callValue(vm, peek(vm, argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm->frames[vm->frameCount - 1];
                break;
            }

            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(vm, function);
                push(vm, OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(vm, frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }

            case OP_CLOSURE_LONG: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT_LONG());
                ObjClosure* closure = newClosure(vm, function);
                push(vm, OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(vm, frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }

            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm, vm->stackTop - 1);
                pop(vm);
                break;
            }

            case OP_RETURN: {
                Value result = pop(vm);
                closeUpvalues(vm, frame->slots);
                vm->frameCount--;
                if (vm->frameCount == vm->baseFrameCount) {
                    if (vm->baseFrameCount == 0) {
                        pop(vm);
                        return INTERPRET_OK;
                    }
                    /* Re-entrant return: leave result on stack */
                    vm->stackTop = frame->slots;
                    push(vm, result);
                    return INTERPRET_OK;
                }
                vm->stackTop = frame->slots;
                push(vm, result);
                frame = &vm->frames[vm->frameCount - 1];
                break;
            }

            /* ---- BUILT-IN OPERATIONS ---- */
            case OP_PRINT: {
                Value val = pop(vm);
                printValue(val);
                break;
            }

            case OP_INPUT: {
                Value prompt = pop(vm);
                if (IS_STRING(prompt) && AS_STRING(prompt)->length > 0) {
                    printf("%s", AS_CSTRING(prompt));
                    fflush(stdout);
                }
                char buffer[4096];
                if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                    /* Remove trailing newline */
                    size_t len = strlen(buffer);
                    if (len > 0 && buffer[len - 1] == '\n') {
                        buffer[len - 1] = '\0';
                        len--;
                    }
                    if (len > 0 && buffer[len - 1] == '\r') {
                        buffer[len - 1] = '\0';
                        len--;
                    }
                    push(vm, OBJ_VAL(copyString(vm, buffer, (int)len)));
                } else {
                    push(vm, OBJ_VAL(copyString(vm, "", 0)));
                }
                break;
            }

            /* ---- LIST OPERATIONS ---- */
            case OP_LIST_NEW: {
                uint8_t count = READ_BYTE();
                ObjList* list = newList(vm);
                /* Items are on stack in order */
                /* We need to pop them and add in correct order */
                push(vm, OBJ_VAL(list)); /* protect from GC */
                for (int i = count; i > 0; i--) {
                    listAppend(vm, list, peek(vm, i));
                }
                /* Remove items and the list from stack, push list back */
                Value listVal = pop(vm); /* the list */
                for (int i = 0; i < count; i++) pop(vm);
                push(vm, listVal);
                break;
            }

            case OP_LIST_GET: {
                Value indexVal = pop(vm);
                Value listVal = pop(vm);
                if (IS_MAP(listVal)) {
                    ObjMap* map = AS_MAP(listVal);
                    Value result;
                    if (!mapGet(map, indexVal, &result)) {
                        runtimeError(vm, "Ye key map mein nahi mili.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(vm, result);
                    break;
                }
                if (!IS_LIST(listVal)) {
                    if (IS_STRING(listVal)) {
                        /* String indexing */
                        if (!IS_NUMBER(indexVal)) {
                            runtimeError(vm, "Index ek number hona chahiye.");
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        ObjString* str = AS_STRING(listVal);
                        int index = (int)AS_NUMBER(indexVal);
                        if (index < 0 || index >= str->length) {
                            runtimeError(vm, "String index %d seema se bahar hai (string length: %d).",
                                index, str->length);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                        push(vm, OBJ_VAL(copyString(vm, &str->chars[index], 1)));
                        break;
                    }
                    runtimeError(vm, "Sirf list ya string mein index se access kar sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!IS_NUMBER(indexVal)) {
                    runtimeError(vm, "List index ek number hona chahiye.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjList* list = AS_LIST(listVal);
                int index = (int)AS_NUMBER(indexVal);
                if (index < 0 || index >= list->items.count) {
                    runtimeError(vm, "List index %d seema se bahar hai (list mein %d items hain).",
                        index, list->items.count);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, listGet(list, index));
                break;
            }

            case OP_LIST_SET: {
                Value value = pop(vm);
                Value indexVal = pop(vm);
                Value listVal = pop(vm);
                if (IS_MAP(listVal)) {
                    ObjMap* map = AS_MAP(listVal);
                    mapSet(vm, map, indexVal, value);
                    push(vm, value);
                    break;
                }
                if (!IS_LIST(listVal)) {
                    runtimeError(vm, "Sirf list ya map mein index se value set kar sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!IS_NUMBER(indexVal)) {
                    runtimeError(vm, "List index ek number hona chahiye.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjList* list = AS_LIST(listVal);
                int index = (int)AS_NUMBER(indexVal);
                if (index < 0 || index >= list->items.count) {
                    runtimeError(vm, "List index %d seema se bahar hai.", index);
                    return INTERPRET_RUNTIME_ERROR;
                }
                listSet(list, index, value);
                push(vm, value);
                break;
            }

            case OP_LIST_APPEND: {
                /* Stack: [list, value] -> [list] (value appended to list) */
                Value value = pop(vm);
                Value listVal = peek(vm, 0);
                if (!IS_LIST(listVal)) {
                    runtimeError(vm, "Sirf list mein append kar sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                listAppend(vm, AS_LIST(listVal), value);
                break;
            }

            case OP_SLICE: {
                /* Stack: [obj, start, end] */
                Value endVal = pop(vm);
                Value startVal = pop(vm);
                Value objVal = pop(vm);

                if (!IS_NUMBER(startVal) || !IS_NUMBER(endVal)) {
                    runtimeError(vm, "Slice indices ko number hona chahiye.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int start = (int)AS_NUMBER(startVal);
                int end = (int)AS_NUMBER(endVal);

                if (IS_STRING(objVal)) {
                    ObjString* str = AS_STRING(objVal);
                    int len = str->length;
                    if (start < 0) start += len;
                    if (end < 0) end += len;
                    if (start < 0) start = 0;
                    if (end > len) end = len;
                    if (start >= end) {
                        push(vm, OBJ_VAL(copyString(vm, "", 0)));
                    } else {
                        push(vm, OBJ_VAL(copyString(vm, str->chars + start, end - start)));
                    }
                } else if (IS_LIST(objVal)) {
                    ObjList* list = AS_LIST(objVal);
                    int len = list->items.count;
                    if (start < 0) start += len;
                    if (end < 0) end += len;
                    if (start < 0) start = 0;
                    if (end > len) end = len;
                    ObjList* result = newList(vm);
                    push(vm, OBJ_VAL(result)); /* GC protect */
                    for (int i = start; i < end; i++) {
                        writeValueArray(&result->items, list->items.values[i]);
                    }
                    /* result already on stack */
                } else {
                    runtimeError(vm, "Sirf string ya list ko slice kar sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_STR_CONCAT: {
                Value b = pop(vm);
                Value a = pop(vm);
                char* sa = valueToString(a);
                char* sb = valueToString(b);
                if (sa && sb) {
                    int len = (int)(strlen(sa) + strlen(sb));
                    char* buf = ALLOCATE(char, len + 1);
                    strcpy(buf, sa);
                    strcat(buf, sb);
                    push(vm, OBJ_VAL(takeString(vm, buf, len)));
                    free(sa); free(sb);
                } else {
                    if (sa) free(sa);
                    if (sb) free(sb);
                    push(vm, NULL_VAL);
                }
                break;
            }

            case OP_HALT: {
                return INTERPRET_OK;
            }

            /* ---- MAP OPERATIONS ---- */
            case OP_MAP_NEW: {
                uint8_t pairCount = READ_BYTE();
                ObjMap* map = newMap(vm);
                push(vm, OBJ_VAL(map)); /* protect from GC */
                /* Key-value pairs are on stack: k1, v1, k2, v2, ... */
                /* They are below the map on the stack */
                for (int i = pairCount; i > 0; i--) {
                    Value val = peek(vm, 2 * i - 1);
                    Value key = peek(vm, 2 * i);
                    mapSet(vm, map, key, val);
                }
                Value mapVal = pop(vm); /* the map */
                for (int i = 0; i < pairCount * 2; i++) pop(vm);
                push(vm, mapVal);
                break;
            }

            /* ---- BITWISE OPERATIONS ---- */
            case OP_BIT_AND: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    runtimeError(vm, "'&' sirf integers ke beech kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                long long b = (long long)AS_NUMBER(pop(vm));
                long long a = (long long)AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL((double)(a & b)));
                break;
            }

            case OP_BIT_OR: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    runtimeError(vm, "'|' sirf integers ke beech kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                long long b = (long long)AS_NUMBER(pop(vm));
                long long a = (long long)AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL((double)(a | b)));
                break;
            }

            case OP_BIT_XOR: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    runtimeError(vm, "'^' sirf integers ke beech kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                long long b = (long long)AS_NUMBER(pop(vm));
                long long a = (long long)AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL((double)(a ^ b)));
                break;
            }

            case OP_BIT_NOT: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    runtimeError(vm, "'~' sirf integers ke saath kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                long long a = (long long)AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL((double)(~a)));
                break;
            }

            case OP_SHIFT_LEFT: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    runtimeError(vm, "'<<' sirf integers ke beech kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                long long b = (long long)AS_NUMBER(pop(vm));
                long long a = (long long)AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL((double)(a << b)));
                break;
            }

            case OP_SHIFT_RIGHT: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    runtimeError(vm, "'>>' sirf integers ke beech kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                long long b = (long long)AS_NUMBER(pop(vm));
                long long a = (long long)AS_NUMBER(pop(vm));
                push(vm, NUMBER_VAL((double)(a >> b)));
                break;
            }

            /* ---- TRY-CATCH ---- */
            case OP_TRY: {
                uint16_t offset = READ_SHORT();
                if (vm->exceptionCount >= MAX_EXCEPTION_HANDLERS) {
                    runtimeError(vm, "Bahut zyada nested try blocks.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ExceptionHandler* handler = &vm->exceptionHandlers[vm->exceptionCount++];
                handler->handlerIP = frame->ip + offset;
                handler->frameIndex = (int)(frame - vm->frames);
                handler->stackTop = vm->stackTop;
                break;
            }

            case OP_TRY_END: {
                vm->exceptionCount--;
                break;
            }

            case OP_THROW: {
                Value errorVal = pop(vm);

                if (vm->exceptionCount == 0) {
                    if (IS_STRING(errorVal)) {
                        runtimeError(vm, "%s", AS_CSTRING(errorVal));
                    } else {
                        char* str = valueToString(errorVal);
                        runtimeError(vm, "Exception: %s", str ? str : "anjaana");
                        if (str) free(str);
                    }
                    return INTERPRET_RUNTIME_ERROR;
                }

                ExceptionHandler handler = vm->exceptionHandlers[--vm->exceptionCount];

                /* Unwind frames */
                while (vm->frameCount - 1 > handler.frameIndex) {
                    closeUpvalues(vm, vm->frames[vm->frameCount - 1].slots);
                    vm->frameCount--;
                }

                vm->stackTop = handler.stackTop;
                push(vm, errorVal);

                frame = &vm->frames[vm->frameCount - 1];
                frame->ip = handler.handlerIP;
                break;
            }

            /* ---- OOP ---- */
            case OP_CLASS: {
                ObjString* name = READ_STRING();
                ObjClass* klass = newClass(vm, name);
                push(vm, OBJ_VAL(klass));
                break;
            }
            case OP_CLASS_LONG: {
                ObjString* name = READ_STRING_LONG();
                ObjClass* klass = newClass(vm, name);
                push(vm, OBJ_VAL(klass));
                break;
            }

            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(vm, 0))) {
                    runtimeError(vm, "Sirf instances ki properties access kar sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(vm, 0));
                ObjString* name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(vm);
                    push(vm, value);
                    break;
                }

                if (!bindMethod(vm, instance->klass, name)) {
                    runtimeError(vm, "'%s' property nahi mili '%s' mein.",
                        name->chars, instance->klass->name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_PROPERTY_LONG: {
                if (!IS_INSTANCE(peek(vm, 0))) {
                    runtimeError(vm, "Sirf instances ki properties access kar sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(vm, 0));
                ObjString* name = READ_STRING_LONG();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(vm);
                    push(vm, value);
                    break;
                }

                if (!bindMethod(vm, instance->klass, name)) {
                    runtimeError(vm, "'%s' property nahi mili '%s' mein.",
                        name->chars, instance->klass->name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(vm, 1))) {
                    runtimeError(vm, "Sirf instances ki properties set kar sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(vm, 1));
                ObjString* name = READ_STRING();
                tableSet(&instance->fields, name, peek(vm, 0));
                Value value = pop(vm);
                pop(vm);
                push(vm, value);
                break;
            }
            case OP_SET_PROPERTY_LONG: {
                if (!IS_INSTANCE(peek(vm, 1))) {
                    runtimeError(vm, "Sirf instances ki properties set kar sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(vm, 1));
                ObjString* name = READ_STRING_LONG();
                tableSet(&instance->fields, name, peek(vm, 0));
                Value value = pop(vm);
                pop(vm);
                push(vm, value);
                break;
            }

            case OP_METHOD: {
                ObjString* name = READ_STRING();
                defineMethod(vm, name);
                break;
            }
            case OP_METHOD_LONG: {
                ObjString* name = READ_STRING_LONG();
                defineMethod(vm, name);
                break;
            }

            case OP_INVOKE: {
                ObjString* methodName = READ_STRING();
                int argCount = READ_BYTE();
                if (!invoke(vm, methodName, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm->frames[vm->frameCount - 1];
                break;
            }
            case OP_INVOKE_LONG: {
                ObjString* methodName = READ_STRING_LONG();
                int argCount = READ_BYTE();
                if (!invoke(vm, methodName, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm->frames[vm->frameCount - 1];
                break;
            }

            case OP_INHERIT: {
                Value superclass = peek(vm, 1);
                if (!IS_CLASS(superclass)) {
                    runtimeError(vm, "Sirf ek kaksha se inherit kar sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjClass* subclass = AS_CLASS(peek(vm, 0));
                tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
                subclass->superclass = AS_CLASS(superclass);
                pop(vm);
                break;
            }

            case OP_GET_SUPER: {
                ObjString* name = READ_STRING();
                ObjClass* superclass = AS_CLASS(pop(vm));

                if (!bindMethod(vm, superclass, name)) {
                    runtimeError(vm, "Super method '%s' nahi mila.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_SUPER_LONG: {
                ObjString* name = READ_STRING_LONG();
                ObjClass* superclass = AS_CLASS(pop(vm));

                if (!bindMethod(vm, superclass, name)) {
                    runtimeError(vm, "Super method '%s' nahi mila.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_SUPER_INVOKE: {
                ObjString* methodName = READ_STRING();
                int argCount = READ_BYTE();
                ObjClass* superclass = AS_CLASS(pop(vm));
                if (!invokeFromClass(vm, superclass, methodName, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm->frames[vm->frameCount - 1];
                break;
            }
            case OP_SUPER_INVOKE_LONG: {
                ObjString* methodName = READ_STRING_LONG();
                int argCount = READ_BYTE();
                ObjClass* superclass = AS_CLASS(pop(vm));
                if (!invokeFromClass(vm, superclass, methodName, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm->frames[vm->frameCount - 1];
                break;
            }

            /* ---- UTILITY ---- */
            case OP_LENGTH: {
                Value val = pop(vm);
                if (IS_LIST(val)) {
                    push(vm, NUMBER_VAL(AS_LIST(val)->items.count));
                } else if (IS_STRING(val)) {
                    push(vm, NUMBER_VAL(AS_STRING(val)->length));
                } else if (IS_MAP(val)) {
                    push(vm, NUMBER_VAL(mapLength(AS_MAP(val))));
                } else {
                    runtimeError(vm, "Sirf list, string, ya map ki lambai nikal sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_IN: {
                Value collection = pop(vm);
                Value needle = pop(vm);
                if (IS_LIST(collection)) {
                    ObjList* list = AS_LIST(collection);
                    bool found = false;
                    for (int i = 0; i < list->items.count; i++) {
                        if (valuesEqual(needle, list->items.values[i])) {
                            found = true;
                            break;
                        }
                    }
                    push(vm, BOOL_VAL(found));
                } else if (IS_STRING(collection)) {
                    if (!IS_STRING(needle)) {
                        runtimeError(vm, "'mein' string ke liye sirf string se search kar sakte ho.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    ObjString* haystack = AS_STRING(collection);
                    ObjString* ndl = AS_STRING(needle);
                    bool found = (strstr(haystack->chars, ndl->chars) != NULL);
                    push(vm, BOOL_VAL(found));
                } else if (IS_MAP(collection)) {
                    if (!IS_STRING(needle)) {
                        runtimeError(vm, "'mein' map ke liye sirf string key se search kar sakte ho.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    ObjMap* map = AS_MAP(collection);
                    Value val;
                    bool found = mapGet(map, needle, &val);
                    push(vm, BOOL_VAL(found));
                } else {
                    runtimeError(vm, "'mein' sirf list, string, ya map ke saath kaam karta hai.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_MAP_KEYS: {
                Value val = pop(vm);
                if (!IS_MAP(val)) {
                    runtimeError(vm, "Sirf map ki keys nikal sakte ho.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjMap* map = AS_MAP(val);
                ObjList* keys = newList(vm);
                push(vm, OBJ_VAL(keys)); /* GC protect */
                for (int i = 0; i < map->capacity; i++) {
                    if (map->entries[i].isOccupied) {
                        writeValueArray(&keys->items, map->entries[i].key);
                    }
                }
                /* keys already on stack */
                break;
            }

            case OP_ITER_PREP: {
                /* If top of stack is a map, replace with its keys list */
                Value val = peek(vm, 0);
                if (IS_MAP(val)) {
                    pop(vm);
                    ObjMap* map = AS_MAP(val);
                    ObjList* keys = newList(vm);
                    push(vm, OBJ_VAL(keys));
                    for (int i = 0; i < map->capacity; i++) {
                        if (map->entries[i].isOccupied) {
                            writeValueArray(&keys->items, map->entries[i].key);
                        }
                    }
                }
                /* else: leave as-is (list or string) */
                break;
            }

            case OP_IMPORT:
            case OP_IMPORT_LONG: {
                ObjString* modulePath;
                if (instruction == OP_IMPORT) {
                    modulePath = READ_STRING();
                } else {
                    modulePath = READ_STRING_LONG();
                }

                /* Check module cache */
                Value cached;
                if (tableGet(&vm->modules, modulePath, &cached)) {
                    /* Already imported — skip */
                    break;
                }

                /* Resolve path relative to current script */
                char* fullPath = resolveModulePath(vm->scriptPath, modulePath->chars);

                /* Read the module file */
                char* source = vmReadFile(fullPath);
                if (source == NULL) {
                    runtimeError(vm, "Module file nahi khul sakti: '%s'", fullPath);
                    free(fullPath);
                    return INTERPRET_RUNTIME_ERROR;
                }

                /* Compile the module */
                ObjFunction* modFunc = compile(vm, source, fullPath);
                free(source);
                if (modFunc == NULL) {
                    runtimeError(vm, "Module compile nahi ho sakti: '%s'", fullPath);
                    free(fullPath);
                    return INTERPRET_RUNTIME_ERROR;
                }

                /* Execute the module (save/restore frame state) */
                push(vm, OBJ_VAL(modFunc));
                ObjClosure* modClosure = newClosure(vm, modFunc);
                pop(vm);
                push(vm, OBJ_VAL(modClosure));

                int savedBaseFrame = vm->baseFrameCount;
                vm->baseFrameCount = vm->frameCount;
                const char* savedPath = vm->scriptPath;
                vm->scriptPath = fullPath;

                callClosure(vm, modClosure, 0);
                InterpretResult modResult = run(vm);

                vm->scriptPath = savedPath;
                vm->baseFrameCount = savedBaseFrame;
                free(fullPath);

                if (modResult != INTERPRET_OK) {
                    return modResult;
                }

                /* Mark module as imported */
                tableSet(&vm->modules, modulePath, BOOL_VAL(true));

                /* Restore IP for current frame */
                frame = &vm->frames[vm->frameCount - 1];
                break;
            }

            default:
                runtimeError(vm, "Anjaana bytecode instruction: %d", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_CONSTANT_LONG
#undef READ_STRING_LONG
#undef BINARY_OP
}

/* ============================================================================
 *  INTERPRET - Main entry point
 * ============================================================================ */
InterpretResult interpret(VM* vm, const char* source, const char* filename) {
    if (vm->scriptPath == NULL) {
        vm->scriptPath = filename;
    }
    ObjFunction* function = compile(vm, source, filename);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(vm, OBJ_VAL(function));
    ObjClosure* closure = newClosure(vm, function);
    pop(vm);
    push(vm, OBJ_VAL(closure));
    callClosure(vm, closure, 0);

    return run(vm);
}

/* ============================================================================
 *  vmCallFunction - Call any callable from native C code (re-entrant)
 *  Supports both native functions and closures.
 * ============================================================================ */
Value vmCallFunction(VM* vm, Value callee, int argCount, Value* args) {
    /* Push callee and arguments onto the stack */
    push(vm, callee);
    for (int i = 0; i < argCount; i++) {
        push(vm, args[i]);
    }

    if (!callValue(vm, peek(vm, argCount), argCount)) {
        vm->hadError = true;
        return NULL_VAL;
    }

    /* For native functions, callValue already executed and pushed result */
    if (IS_OBJ(callee) && OBJ_TYPE(callee) == OBJ_NATIVE) {
        return pop(vm);
    }

    /* For closures, run the VM dispatch loop re-entrantly */
    int savedBase = vm->baseFrameCount;
    vm->baseFrameCount = vm->frameCount - 1;
    InterpretResult result = run(vm);
    vm->baseFrameCount = savedBase;

    if (result != INTERPRET_OK) {
        vm->hadError = true;
        return NULL_VAL;
    }

    /* The return value is on top of the stack (pushed by OP_RETURN) */
    return pop(vm);
}
