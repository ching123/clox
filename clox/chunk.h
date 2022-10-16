#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"

typedef enum {
	OP_RETURN,
} OpCode;

typedef struct {
	int count;
	int capaciy;
	uint8_t* code;
} Chunk;

void InitChunk(Chunk* chunk);
void FreeChunk(Chunk* chunk);
void WriteChunk(Chunk* chunk, uint8_t byte);

#endif // !CLOX_CHUNK_H
