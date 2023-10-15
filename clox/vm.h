#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"
#include "object.h"

#define FRAME_MAX 64
#define STACK_MAX (FRAME_MAX * UINT8_COUNT)

typedef struct {
	ObjClosure* closure;
	uint8_t* ip;
	Value* slots;
} CallFrame;

typedef struct {
	CallFrame frames[FRAME_MAX];
	int frame_count;
	Value stack[STACK_MAX];
	Value* stack_top;
	Table globals;
	Table strings;
	Obj* obj_head;
	ObjUpvalue* open_upvalues;
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void InitVM();
void FreeVM();
//InterpretResult Interpret(Chunk* chunk);
InterpretResult Interpret(const char* source);
void PushStack(Value value);
Value PopStack();

#endif // !CLOX_VM_H
