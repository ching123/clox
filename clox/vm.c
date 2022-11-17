#include "vm.h"

#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "compiler.h"



VM vm;

void ResetStack() {
	vm.stakp_top = vm.stack;
}
void InitVM() {
	ResetStack();
}

void FreeVM() {
}

InterpretResult Run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
	do { \
		Value b = PopStack(); \
		Value a = PopStack(); \
		PushStack(a op b); \
	} while (0)

	for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
		printf("          ");
		for (Value* slot = vm.stack; slot < vm.stakp_top; ++slot) {
			printf("[");
			PrintValue(*slot);
			printf("]");
		}
		printf("\n");
		DisassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif // DEBUG_TRACE_EXECUTION

		uint8_t instruction;
		switch (instruction = READ_BYTE()) {
		case OP_RETURN: {
			PrintValue(PopStack());
			printf("\n");
			return INTERPRET_OK;
		}
		case OP_CONSTANT: {
			Value constant = READ_CONSTANT();
			PushStack(constant);
			break;
		}
		case OP_NEGATE: {
			PushStack(-PopStack());
			break;
		}
		case OP_ADD: {
			BINARY_OP(+);
			break;
		}
		case OP_SUBTRACT: {
			BINARY_OP(-);
			break;
		}
		case OP_MULTIPLY: {
			BINARY_OP(*);
			break;
		}
		case OP_DIVIDE: {
			BINARY_OP(/);
			break;
		}

		default:
			break;
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}
/*
InterpretResult Interpret(Chunk* chunk) {
	vm.chunk = chunk;
	vm.ip = chunk->code;
	return Run();
}
*/
InterpretResult Interpret(const char* source)
{
	Compile(source);
	return INTERPRET_OK;
}

void PushStack(Value value) {
	*vm.stakp_top = value;
	vm.stakp_top++;
}

Value PopStack() {
	vm.stakp_top--;
	return *vm.stakp_top;
}
