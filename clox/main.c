#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void Repl() {
    char line[1024];
    for (;;) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        //Interpret(line);
    }
}

static char* ReadFile(const char* path) {
    FILE* fp = NULL;
    fopen_s(&fp, path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "could not open file '%s'.", path);
        exit(74);
    }
    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    rewind(fp);
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "not enough memroy to read '%s'.", path);
        exit(74);
    }
    size_t byte_read = fread(buffer, sizeof(char), file_size, fp);
    if (byte_read != file_size) {
        fprintf(stderr, "could not read file '%s'.", path);
        exit(74);
    }
    buffer[byte_read] = '\0';
    fclose(fp);

    return buffer;
}

static void RunFile(const char* path) {
    char* source = ReadFile(path);
    InterpretResult ret = Interpret(source);
    free(source);

    if (ret == INTERPRET_COMPILE_ERROR) exit(65);
    if (ret == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[])
{
    InitVM();
    if (argc == 1) {
        Repl();
    }
    else if (argc == 2) {
        RunFile(argv[1]);
    }
    else {
        fprintf(stderr, "usage: clox [path]\n");
        exit(64);
    }
    FreeVM();

    return 0;
    /*
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
    */
}
