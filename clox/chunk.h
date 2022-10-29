#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum {
	OP_CONSTANT,
	OP_RETURN,
	OP_NEGATE,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
} OpCode;

typedef struct {
	int count;
	int capaciy;
	uint8_t* code;
	int* lines;
	ValueArray	constants;
} Chunk;

void InitChunk(Chunk* chunk);
void FreeChunk(Chunk* chunk);
void WriteChunk(Chunk* chunk, uint8_t byte, int line);
int AddConstant(Chunk* chunk, Value value);

#endif // !CLOX_CHUNK_H
