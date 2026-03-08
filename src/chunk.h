/*
 * ============================================================================
 *  RAMPYAARYAN - Bytecode Chunk Header
 *  Stores compiled bytecode instructions and constants
 * ============================================================================
 */

#ifndef RAMPYAARYAN_CHUNK_H
#define RAMPYAARYAN_CHUNK_H

#include "common.h"
#include "value.h"

/* ============================================================================
 *  OPCODES - The instruction set of the Rampyaaryan VM
 * ============================================================================ */
typedef enum {
    /* Constants & Literals */
    OP_CONSTANT,        /* Push constant from pool */
    OP_CONSTANT_LONG,   /* Push constant (16-bit index) */
    OP_NULL,            /* Push khali */
    OP_TRUE,            /* Push sach */
    OP_FALSE,           /* Push jhooth */

    /* Stack Operations */
    OP_POP,             /* Pop top value */
    OP_POPN,            /* Pop N values */
    OP_DUP,             /* Duplicate top */

    /* Variables */
    OP_DEFINE_GLOBAL,   /* Define global variable */
    OP_GET_GLOBAL,      /* Read global variable */
    OP_SET_GLOBAL,      /* Set global variable */
    OP_DEFINE_GLOBAL_LONG, /* Define global (16-bit index) */
    OP_GET_GLOBAL_LONG,    /* Read global (16-bit index) */
    OP_SET_GLOBAL_LONG,    /* Set global (16-bit index) */
    OP_GET_LOCAL,       /* Read local variable */
    OP_SET_LOCAL,       /* Set local variable */
    OP_GET_UPVALUE,     /* Read upvalue (closure) */
    OP_SET_UPVALUE,     /* Set upvalue (closure) */

    /* Arithmetic */
    OP_ADD,             /* + (jod) */
    OP_SUBTRACT,        /* - (ghata) */
    OP_MULTIPLY,        /* * (guna) */
    OP_DIVIDE,          /* / (bhag) */
    OP_MODULO,          /* % (baaki) */
    OP_POWER,           /* ** (power/ghat) */
    OP_NEGATE,          /* Unary - */

    /* Comparison */
    OP_EQUAL,           /* == (barabar) */
    OP_NOT_EQUAL,       /* != (barabar nahi) */
    OP_GREATER,         /* > (bada) */
    OP_LESS,            /* < (chhota) */
    OP_GREATER_EQUAL,   /* >= */
    OP_LESS_EQUAL,      /* <= */

    /* Logical */
    OP_NOT,             /* nahi */

    /* Control Flow */
    OP_JUMP,            /* Unconditional jump */
    OP_JUMP_IF_FALSE,   /* Jump if falsy */
    OP_LOOP,            /* Loop back */

    /* Functions */
    OP_CALL,            /* Call function */
    OP_CLOSURE,         /* Create closure */
    OP_CLOSURE_LONG,    /* Create closure (16-bit index) */
    OP_CLOSE_UPVALUE,   /* Close upvalue */
    OP_RETURN,          /* wapas do */

    /* Built-in Operations */
    OP_PRINT,           /* likho */
    OP_INPUT,           /* pucho */

    /* List Operations */
    OP_LIST_NEW,        /* Create new list with N elements */
    OP_LIST_GET,        /* Get list[index] */
    OP_LIST_SET,        /* Set list[index] = value */
    OP_LIST_APPEND,     /* Append value to list (for comprehensions) */
    OP_SLICE,           /* Slice list/string [start:end] */

    /* String Operations */
    OP_STR_CONCAT,      /* String concatenation */

    /* Map Operations */
    OP_MAP_NEW,         /* Create new map with N key-value pairs */

    /* Bitwise Operations */
    OP_BIT_AND,         /* & */
    OP_BIT_OR,          /* | */
    OP_BIT_XOR,         /* ^ */
    OP_BIT_NOT,         /* ~ */
    OP_SHIFT_LEFT,      /* << */
    OP_SHIFT_RIGHT,     /* >> */

    /* Try-Catch */
    OP_TRY,             /* Begin try block (operand: catch jump offset) */
    OP_TRY_END,         /* End try block (pop handler) */
    OP_THROW,           /* Throw exception */

    /* OOP - Classes */
    OP_CLASS,           /* Create class */
    OP_CLASS_LONG,      /* Create class (16-bit index) */
    OP_GET_PROPERTY,    /* Get object property */
    OP_GET_PROPERTY_LONG, /* Get object property (16-bit) */
    OP_SET_PROPERTY,    /* Set object property */
    OP_SET_PROPERTY_LONG, /* Set object property (16-bit) */
    OP_METHOD,          /* Define method on class */
    OP_METHOD_LONG,     /* Define method (16-bit index) */
    OP_INVOKE,          /* Optimized method call */
    OP_INVOKE_LONG,     /* Optimized method call (16-bit) */
    OP_INHERIT,         /* Class inheritance */
    OP_GET_SUPER,       /* Access super method */
    OP_GET_SUPER_LONG,  /* Access super method (16-bit) */
    OP_SUPER_INVOKE,    /* Super method call */
    OP_SUPER_INVOKE_LONG, /* Super method call (16-bit) */

    /* Module system */
    OP_IMPORT,          /* Import module */
    OP_IMPORT_LONG,     /* Import module (16-bit) */
    OP_EXPORT,          /* Export variable */

    /* Utility */
    OP_LENGTH,          /* Get length of list/string/map */
    OP_IN,              /* Membership test: value in collection */
    OP_MAP_KEYS,        /* Get list of map keys */
    OP_ITER_PREP,       /* Prepare iterable: maps → keys list, others unchanged */

    /* Program Control */
    OP_HALT,            /* Stop VM */
} OpCode;

/* ============================================================================
 *  BYTECODE CHUNK
 * ============================================================================ */
typedef struct Chunk {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;       /* Line numbers for each bytecode (for error reporting) */
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);
void writeConstant(Chunk* chunk, Value value, int line);

#endif /* RAMPYAARYAN_CHUNK_H */
