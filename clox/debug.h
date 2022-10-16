#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h"

void DisassembleChunk(Chunk* chunk, const char* name);
int DisassembleInstruction(Chunk* chunk, int offset);

#endif // !CLOX_DEBUG_H
