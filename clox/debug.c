#include "debug.h"


#include <stdio.h>

static int SimpleInstruction(const char* name, int offset) {
	printf("%s\n", name);
	return offset + 1;
}

void DisassembleChunk(Chunk* chunk, const char* name)
{
	printf("== %s ==\n", name);
	for (int offset = 0; offset < chunk->count; ) {
		offset = DisassembleInstruction(chunk, offset);
	}
}

int DisassembleInstruction(Chunk* chunk, int offset)
{
	printf("%04d ", offset);

	uint8_t instruction = chunk->code[offset];
	switch (instruction) {
	case OP_RETURN: {
		return SimpleInstruction("OP_RETURN", offset);
	}
	default:
		printf("unknow opcode %d\n", instruction);
		return offset + 1;
	}
}
