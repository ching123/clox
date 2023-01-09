#include "vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"



VM vm;
InterpretResult Run();

static void ResetStack() {
	vm.stack_top = vm.stack;
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
	InitTable(&vm.globals);
	InitTable(&vm.strings);
	vm.obj_head = NULL;
}

void FreeVM() {
	FreeTable(&vm.globals);
	FreeTable(&vm.strings);
	FreeObjects();
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
	*vm.stack_top = value;
	vm.stack_top++;
}

Value PopStack() {
	vm.stack_top--;
	return *vm.stack_top;
}

static Value PeekStack(int distance) {
	return vm.stack_top[-1 - distance];
}
static bool IsFalsey(Value value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
static void Concatenate() {
	ObjString* b = AS_STRING(PopStack());
	ObjString* a = AS_STRING(PopStack());
	int length = a->length + b->length;
	char* buffer = ALLOCATE(char, length + 1);
	memcpy(buffer, a->str, a->length);
	memcpy(buffer + a->length, b->str, b->length);
	buffer[length] = '\0';

	ObjString* result = TakeString(buffer, length);
	PushStack(OBJ_VAL(result));
}

static InterpretResult Run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(ValueType, op) \
	do { \
		if (!IS_NUMBER(PeekStack(0)) || !IS_NUMBER(PeekStack(1))) { \
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
		for (Value* slot = vm.stack; slot < vm.stack_top; ++slot) {
			printf("[");
			PrintValue(*slot);
			printf("]");
		}
		printf("\n");
		DisassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif // DEBUG_TRACE_EXECUTION

		uint8_t instruction;
		switch (instruction = READ_BYTE()) {
		case OP_PRINT: {
			PrintValue(PopStack());
			printf("\n");
			break;
		}
		case OP_RETURN: {
			return INTERPRET_OK;
		}
		case OP_POP: {
			PopStack();
			break;
		}
		case OP_DEFINE_GLOBAL: {
			ObjString* name = READ_STRING();
			// ensures the VM can still find the value if a garbage
			// collection is triggered right in the middle of adding
			// it to the hash table. That¡¯s a distinct possibility
			// since the hash table requires dynamic allocation
			// when it resizes.
			// ???
			// http://www.craftinginterpreters.com/global-variables.html#:~:text=the%20hash%20table.-,That%20ensures%20the%20VM%20can%20still%20find%20the%20value%20if%20a%20garbage%20collection%20is%20triggered%20right%20in%20the%20middle%20of%20adding%20it%20to%20the%20hash%20table.%20That%E2%80%99s%20a%20distinct%20possibility%20since%20the%20hash%20table%20requires%20dynamic%20allocation%20when%20it%20resizes.,-This%20code%20doesn%E2%80%99t
			TableSet(&vm.globals, name, PeekStack(0));
			PopStack();
			break;
		}
		case OP_GET_LOCAL: {
			uint8_t slot = READ_BYTE();
			PushStack(vm.stack[slot]);
			break;
		}
		case OP_SET_LOCAL: {
			uint8_t slot = READ_BYTE();
			vm.stack[slot] = PeekStack(0);
			break;
		}
		case OP_GET_GLOBAL: {
			ObjString* name = READ_STRING();
			Value value;
			if (!TableGet(&vm.globals, name, &value)) {
				RuntimeError("undefined variable '%s'.", name->str);
				return INTERPRET_RUNTIME_ERROR;
			}
			PushStack(value);
			break;
		}
		case OP_SET_GLOBAL: {
			ObjString* name = READ_STRING();
			if (TableSet(&vm.globals, name, PeekStack(0))) {
				TableDelete(&vm.globals, name);
				RuntimeError("undefined variable '%s'.", name->str);
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
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
			if (IS_STRING(PeekStack(0)) && IS_STRING(PeekStack(1))) {
				Concatenate();
			}
			else if (IS_NUMBER(PeekStack(0)) && IS_NUMBER(PeekStack(1))) {
				double b = AS_NUMBER(PopStack());
				double a = AS_NUMBER(PopStack());
				PushStack(NUMBER_VAL(a + b));
			}
			else {
				RuntimeError("operands must be two strings or two numbers.");
				return INTERPRET_RUNTIME_ERROR;
			}
			//BINARY_OP(NUMBER_VAL, +);
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
#undef READ_STRING
#undef BINARY_OP
}