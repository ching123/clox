
#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[])
{
    Chunk chunk;
    InitChunk(&chunk);

    int constant_index = AddConstant(&chunk, 1.2);
    WriteChunk(&chunk, OP_CONSTANT, 123);
    WriteChunk(&chunk, constant_index, 123);

    WriteChunk(&chunk, OP_RETURN, 123);
    DisassembleChunk(&chunk, "test chunk");
    FreeChunk(&chunk);
    return 0;
}
