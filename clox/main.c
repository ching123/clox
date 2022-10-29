
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[])
{
    InitVM();
    Chunk chunk;
    InitChunk(&chunk);

    int constant_index = AddConstant(&chunk, 1.2);
    WriteChunk(&chunk, OP_CONSTANT, 123);
    WriteChunk(&chunk, constant_index, 123);

    constant_index = AddConstant(&chunk, 3.4);
    WriteChunk(&chunk, OP_CONSTANT, 123);
    WriteChunk(&chunk, constant_index, 123);

    WriteChunk(&chunk, OP_ADD, 123);

    constant_index = AddConstant(&chunk, 5.6);
    WriteChunk(&chunk, OP_CONSTANT, 123);
    WriteChunk(&chunk, constant_index, 123);

    WriteChunk(&chunk, OP_DIVIDE, 123);
    WriteChunk(&chunk, OP_NEGATE, 123);

    WriteChunk(&chunk, OP_RETURN, 123);
    //DisassembleChunk(&chunk, "test chunk");
    Interpret(&chunk);
    FreeVM();
    FreeChunk(&chunk);
    return 0;
}
