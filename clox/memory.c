#include "memory.h"

#include <stdlib.h>

#include "vm.h"
#include "object.h"


static void FreeObject(Obj* obj) {
	switch (obj->type)
	{
	case OBJ_STRING: {
		ObjString* obj_str = (ObjString*)(obj);
		FREE_ARRAY(char, obj_str->str, obj_str->length);
		FREE(ObjString, obj);
		break;
	}
	case OBJ_FUNCTION: {
		ObjFunction* function = (ObjFunction*)(obj);
		FreeChunk(&function->chunk);
		FREE(ObjFunction, function);
		break;
	}
	case OBJ_NATIVE: {
		FREE(ObjNative, obj);
		break;
	}
	default:
		break;
	}
}

void* reallocate(void* pointer, size_t old_capacity, size_t new_capacity)
{
	if (new_capacity == 0) {
		free(pointer);
		return NULL;
	}

	void* result = realloc(pointer, new_capacity);
	if (result == NULL) exit(1);
	return result;
}

void FreeObjects()
{
	Obj* obj = vm.obj_head;
	while (obj != NULL) {
		Obj* next = obj->next;
		FreeObject(obj);
		obj = next;
	}
}
