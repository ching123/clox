#include "object.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
	(type*)AllocateObject(sizeof(type), objectType)

static Obj* AllocateObject(size_t size, ObjType type) {
	Obj* obj = (Obj*)reallocate(NULL, 0, size);
	obj->type = type;

	obj->next = vm.obj_head;
	vm.obj_head = obj;

	return obj;
}

static ObjString* AllocateString(char* buffer, int length, uint32_t hash) {
	ObjString* obj_str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	obj_str->length = length;
	obj_str->str = buffer;
	obj_str->hash = hash;
	TableSet(&vm.strings, obj_str, NIL_VAL);
	return obj_str;
}

static uint32_t HashString(const char* src, int length) {
	uint32_t hash = 2166136261u;
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)src[i];
		hash *= 16777619;
	}
	return hash;
}

static void PrintFunction(const ObjFunction* function) {
	function->name != NULL ?
		printf("<fn %s>", function->name->str) : printf("<script>");
}

ObjString* TakeString(char* src, int length)
{
	uint32_t hash = HashString(src, length);
	ObjString* interned = TableFindString(&vm.strings, src, length, hash);
	if (interned != NULL) {
		FREE_ARRAY(char, src, length + 1);
		return interned;
	}
	return AllocateString(src, length, hash);
}

ObjString* CopyString(const char* src, int length)
{
	uint32_t hash = HashString(src, length);
	ObjString* interned = TableFindString(&vm.strings, src, length, hash);
	if (interned != NULL) {
		return interned;
	}

	char* buffer = ALLOCATE(char, length + 1);
	memcpy(buffer, src, length);
	buffer[length] = '\0';
	return AllocateString(buffer, length, hash);
}

ObjFunction* NewFunction()
{
	ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
	function->arity = 0;
	function->name = NULL;
	InitChunk(&function->chunk);
	return function;
}

ObjNative* NewNative(NativeFn function)
{
	ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
	native->function = function;
	return native;
}

void PrintObject(Value value)
{
	switch (OBJ_TYPE(value))
	{
	case OBJ_STRING:
		printf("%s", AS_CSTRING(value));
		break;
	case OBJ_FUNCTION:
		PrintFunction(AS_FUNCTION(value));
		break;
	case OBJ_NATIVE:
		printf("<native fn>");
		break;
	}
}
