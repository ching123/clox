#include "vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"



VM vm;
InterpretResult Run();
static bool Call(ObjFunction* function, int arg_count);

static void ResetStack() {
	vm.stack_top = vm.stack;
	vm.frame_count = 0;
}
static void RuntimeError(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputs("\n", stderr);

	for (int i = vm.frame_count - 1; i >= 0; i--) {
		CallFrame* frame = &vm.frames[i];
		ObjFunction* function = frame->function;
		size_t index = frame->ip - frame->function->chunk.code - 1;
		fprintf(stderr, "[line %d] in ", function->chunk.lines[index]);
		if (function->name == NULL) {
			fprintf(stderr, "script\n");
		}
		else {
			fprintf(stderr, "%s()\n", function->name->str);
		}
	}

	ResetStack();
}
static void DefineNative(const char* name, NativeFn function) {
	PushStack(OBJ_VAL(CopyString(name, (int)strlen(name))));
	PushStack(OBJ_VAL(NewNative(function)));
	TableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
	PopStack();
	PopStack();
}
static Value ClockNative(int arg_count, Value* args) {
	return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
void InitVM() {
	ResetStack();
	InitTable(&vm.globals);
	InitTable(&vm.strings);
	vm.obj_head = NULL;

	DefineNative("clock", ClockNative);
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

	ObjFunction* function = Compile(source);
	if (function == NULL) {
		return INTERPRET_COMPILE_ERROR;
	}

	PushStack(OBJ_VAL(function));
	Call(function, 0);

	printf("\n\n");
	return Run();
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
static bool Call(ObjFunction* function, int arg_count) {
	if (arg_count != function->arity) {
		RuntimeError("expect %d arguments but got %d.",
			function->arity, arg_count);
		return false;
	}
	if (vm.frame_count == FRAME_MAX) {
		RuntimeError("stack overflow.");
		return false;
	}

	CallFrame* frame = &vm.frames[vm.frame_count++];
	frame->function = function;
	frame->ip = function->chunk.code;
	frame->slots = vm.stack_top - arg_count - 1;
	return true;
}
static bool CallValue(Value value, int arg_count) {
	if (IS_OBJ(value)) {
		switch (OBJ_TYPE(value)) {
		case OBJ_FUNCTION: return Call(AS_FUNCTION(value), arg_count);
		case OBJ_NATIVE: {
			NativeFn function = AS_NATIVE(value);
			Value result = function(arg_count, vm.stack_top - arg_count);
			vm.stack_top -= arg_count + 1;
			PushStack(result);
			return true;
		}
		default: break;
		}
	}
	RuntimeError("can only call functions and classes.");
	return false;
}

static InterpretResult Run() {
	CallFrame* frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
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
		DisassembleInstruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
#endif // DEBUG_TRACE_EXECUTION

		uint8_t instruction;
		switch (instruction = READ_BYTE()) {
		case OP_PRINT: {
			PrintValue(PopStack());
			printf("\n");
			break;
		}
		case OP_RETURN: {
			Value result = PopStack();
			vm.frame_count--;
			if (vm.frame_count == 0) {
				PopStack();
				return INTERPRET_OK;
			}

			vm.stack_top = frame->slots;
			PushStack(result);
			frame = &vm.frames[vm.frame_count - 1];
			break;
		}
		case OP_CALL: {
			int arg_count = READ_BYTE();
			if (!CallValue(PeekStack(arg_count), arg_count)) {
				return INTERPRET_RUNTIME_ERROR;
			}
			frame = &vm.frames[vm.frame_count - 1];
			break;
		}
		case OP_LOOP: {
			uint16_t offset = READ_SHORT();
			frame->ip -= offset;
			break;
		}
		case OP_JUMP: {
			uint16_t offset = READ_SHORT();
			frame->ip += offset;
			break;
		}
		case OP_JUMP_IF_FALSE: {
			uint16_t offset = READ_SHORT();
			if (IsFalsey(PeekStack(0))) {
				frame->ip += offset;
			}
			break;
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
#undef READ_SHORT
#undef BINARY_OP
}