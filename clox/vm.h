#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"

#define STACK_MAX 256

typedef struct {
	Chunk* chunk;
	uint8_t* ip;
	Value stack[STACK_MAX];
	Value* stakp_top;
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;


void InitVM();
void FreeVM();
//InterpretResult Interpret(Chunk* chunk);
InterpretResult Interpret(const char* source);
void PushStack(Value value);
Value PopStack();

#endif // !CLOX_VM_H
