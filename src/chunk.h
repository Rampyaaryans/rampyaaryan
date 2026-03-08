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
    OP_CLOSE_UPVALUE,   /* Close upvalue */
    OP_RETURN,          /* wapas do */

    /* Built-in Operations */
    OP_PRINT,           /* likho */
    OP_INPUT,           /* pucho */

    /* List Operations */
    OP_LIST_NEW,        /* Create new list with N elements */
    OP_LIST_GET,        /* Get list[index] */
    OP_LIST_SET,        /* Set list[index] = value */

    /* String Operations */
    OP_STR_CONCAT,      /* String concatenation */

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
