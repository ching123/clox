
#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[])
{
    Chunk chunk;
    InitChunk(&chunk);
    WriteChunk(&chunk, OP_RETURN);
    DisassembleChunk(&chunk, "test chunk");
    FreeChunk(&chunk);
    return 0;
}
