#include "chunk.h"


#include "memory.h"

void InitChunk(Chunk* chunk)
{
	chunk->capaciy = 0;
	chunk->count = 0;
	chunk->code = NULL;
	chunk->lines = NULL;
	InitValueArray(&chunk->constants);
}

void FreeChunk(Chunk* chunk)
{
	FREE_ARRAY(uint8_t, chunk->code, chunk->capaciy);
	FREE_ARRAY(int, chunk->lines, chunk->capaciy);
	InitChunk(chunk);
	FreeValueArray(&chunk->constants);
}

void WriteChunk(Chunk* chunk, uint8_t byte, int line)
{
	if (chunk->count >= chunk->capaciy) {
		int old_capacity = chunk->capaciy;
		chunk->capaciy = GROW_CAPACITY(old_capacity);
		chunk->code = GROW_ARRAY(uint8_t, chunk->code,
			old_capacity, chunk->capaciy);
		chunk->lines = GROW_ARRAY(int, chunk->lines,
			old_capacity, chunk->capaciy);
	}
	chunk->code[chunk->count] = byte;
	chunk->lines[chunk->count] = line;
	chunk->count++;
}

int AddConstant(Chunk* chunk, Value value)
{
	WriteValueArray(&chunk->constants, value);
	return chunk->constants.count - 1;
}
