/*
 * ============================================================================
 *  RAMPYAARYAN - Debug/Disassembler Header
 * ============================================================================
 */

#ifndef RAMPYAARYAN_DEBUG_H
#define RAMPYAARYAN_DEBUG_H

#include "common.h"
#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);
const char* opcodeName(OpCode opcode);

#endif /* RAMPYAARYAN_DEBUG_H */
