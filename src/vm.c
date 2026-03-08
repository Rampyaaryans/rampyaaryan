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

    initTable(&vm->globals);
    initTable(&vm->strings);

    setCurrentVM(vm);

    /* Register built-in functions */
    registerNatives(vm);
}

void freeVM(VM* vm) {
    freeTable(&vm->globals);
    freeTable(&vm->strings);
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
 *  FUNCTION CALL
 * ============================================================================ */
static bool callValue(VM* vm, Value callee, int argCount);

static bool callClosure(VM* vm, ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError(vm, "'%s' ko %d arguments chahiye, lekin %d diye gaye.",
            closure->function->name ? closure->function->name->chars : "script",
            closure->function->arity, argCount);
        return false;
    }

    if (vm->frameCount == MAX_CALL_FRAMES) {
        runtimeError(vm, "Stack overflow! Bahut zyada function calls (recursion check karo).");
        return false;
    }

    CallFrame* frame = &vm->frames[vm->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk->code;
    frame->slots = vm->stackTop - argCount - 1;
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
            default:
                break;
        }
    }
    runtimeError(vm, "Sirf functions ko call kar sakte ho.");
    return false;
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

            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm, vm->stackTop - 1);
                pop(vm);
                break;
            }

            case OP_RETURN: {
                Value result = pop(vm);
                closeUpvalues(vm, frame->slots);
                vm->frameCount--;
                if (vm->frameCount == 0) {
                    pop(vm);
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
                if (!IS_LIST(listVal)) {
                    runtimeError(vm, "Sirf list mein index se value set kar sakte ho.");
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

            default:
                runtimeError(vm, "Anjaana bytecode instruction: %d", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

/* ============================================================================
 *  INTERPRET - Main entry point
 * ============================================================================ */
InterpretResult interpret(VM* vm, const char* source, const char* filename) {
    ObjFunction* function = compile(vm, source, filename);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(vm, OBJ_VAL(function));
    ObjClosure* closure = newClosure(vm, function);
    pop(vm);
    push(vm, OBJ_VAL(closure));
    callClosure(vm, closure, 0);

    return run(vm);
}
