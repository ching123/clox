#include "chunk.h"


#include "memory.h"

void InitChunk(Chunk* chunk)
{
	chunk->capaciy = 0;
	chunk->count = 0;
	chunk->code = NULL;
}

void FreeChunk(Chunk* chunk)
{
	FREE_ARRAY(uint8_t, chunk->code, chunk->capaciy);
	InitChunk(chunk);
}

void WriteChunk(Chunk* chunk, uint8_t byte)
{
	if (chunk->count >= chunk->capaciy) {
		int old_capacity = chunk->capaciy;
		chunk->capaciy = GROW_CAPACITY(old_capacity);
		chunk->code = GROW_ARRAY(uint8_t, chunk->code,
			old_capacity, chunk->capaciy);
	}
	chunk->code[chunk->count] = byte;
	chunk->count++;
}
