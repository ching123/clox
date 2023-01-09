#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum {
	OP_CONSTANT,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_PRINT,
	OP_RETURN,
	OP_POP,
	OP_DEFINE_GLOBAL,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_GET_GLOBAL,
	OP_SET_GLOBAL,
	OP_NOT,
	OP_NEGATE,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
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
