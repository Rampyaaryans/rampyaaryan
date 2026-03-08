/*
 * ============================================================================
 *  RAMPYAARYAN - Compiler Header
 *  Parses tokens and emits bytecode in a single pass (Pratt parser)
 * ============================================================================
 */

#ifndef RAMPYAARYAN_COMPILER_H
#define RAMPYAARYAN_COMPILER_H

#include "common.h"
#include "chunk.h"
#include "object.h"
#include "lexer.h"

/* Forward declaration */
typedef struct VM VM;

/* ============================================================================
 *  COMPILER RESULT
 * ============================================================================ */
ObjFunction* compile(VM* vm, const char* source, const char* filename);

/* Mark compiler roots for GC */
void markCompilerRoots(VM* vm);

#endif /* RAMPYAARYAN_COMPILER_H */
