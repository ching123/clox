#include "vm.h"

#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "compiler.h"



VM vm;
InterpretResult Run();

static void ResetStack() {
	vm.stakp_top = vm.stack;
}
static void RuntimeError(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputs("\n", stderr);

	size_t index = vm.ip - vm.chunk->code - 1;
	int line_num = vm.chunk->lines[index];
	fprintf(stderr, "[line %d] in script.\n", line_num);
	ResetStack();
}
void InitVM() {
	ResetStack();
}

void FreeVM() {
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
	Chunk chunk;
	InitChunk(&chunk);

	if (!Compile(source, &chunk)) {
		FreeChunk(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}

	vm.chunk = &chunk;
	vm.ip = chunk.code;

	InterpretResult result = Run();

	FreeChunk(&chunk);
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

static Value PeekStack(int distance) {
	return vm.stakp_top[-1 - distance];
}
static bool IsFalsey(Value value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static InterpretResult Run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(ValueType, op) \
	do { \
		if (!IS_NUMBER(PeekStack(0)) || !IS_NUMBER(PeekStack(0))) { \
			RuntimeError("operands must be numbers."); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		double b = AS_NUMBER(PopStack()); \
		double a = AS_NUMBER(PopStack()); \
		PushStack(ValueType(a op b)); \
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
		case OP_NIL: PushStack(NIL_VAL); break;
		case OP_TRUE: PushStack(BOOL_VAL(true)); break;
		case OP_FALSE: PushStack(BOOL_VAL(false)); break;
		case OP_NOT: PushStack(BOOL_VAL(IsFalsey(PopStack()))); break;
		case OP_NEGATE: {
			if (!IS_NUMBER(PeekStack(0))) {
				RuntimeError("operand must be number.");
				return INTERPRET_RUNTIME_ERROR;
			}
			PushStack(NUMBER_VAL(-AS_NUMBER(PopStack())));
			break;
		}
		case OP_EQUAL: {
			Value b = PopStack();
			Value a = PopStack();
			PushStack(BOOL_VAL(ValuesEqual(a, b)));
			break;
		}
		case OP_GREATER: {
			BINARY_OP(BOOL_VAL, >);
			break;
		}
		case OP_LESS: {
			BINARY_OP(BOOL_VAL, <);
			break;
		}
		case OP_ADD: {
			BINARY_OP(NUMBER_VAL, +);
			break;
		}
		case OP_SUBTRACT: {
			BINARY_OP(NUMBER_VAL, -);
			break;
		}
		case OP_MULTIPLY: {
			BINARY_OP(NUMBER_VAL, *);
			break;
		}
		case OP_DIVIDE: {
			BINARY_OP(NUMBER_VAL, / );
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