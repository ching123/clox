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

static ObjString* AllocateString(char* buffer, int length) {
	ObjString* obj_str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	obj_str->length = length;
	obj_str->str = buffer;
	return obj_str;
}

ObjString* TakeString(char* src, int length)
{
	return AllocateString(src, length);
}

ObjString* CopyString(const char* src, int length)
{
	char* buffer = ALLOCATE(char, length + 1);
	memcpy(buffer, src, length);
	buffer[length] = '\0';
	return AllocateString(buffer, length);
}

void PrintObject(Value value)
{
	switch (OBJ_TYPE(value))
	{
	case OBJ_STRING:
		printf("%s", AS_CSTRING(value));
		break;
	}
}
