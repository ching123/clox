#include "value.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"

bool ValuesEqual(Value a, Value b)
{
	if (a.type != b.type) return false;
	switch (a.type) {
	case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
	case VAL_NIL: return true;
	case VAL_NUMBER: return AS_NUMBER(a) = AS_NUMBER(b);
	case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b);
	default: return false;  // unreachable
	}
}

void InitValueArray(ValueArray* array)
{
	array->capacity = 0;
	array->count = 0;
	array->values = NULL;
}

void WriteValueArray(ValueArray* array, Value value)
{
	if (array->count >= array->capacity) {
		int old_capacity = array->capacity;
		array->capacity = GROW_CAPACITY(old_capacity);
		array->values = GROW_ARRAY(Value, array->values,
			old_capacity, array->capacity);
	}
	array->values[array->count] = value;
	array->count++;
}

void FreeValueArray(ValueArray* array)
{
	FREE_ARRAY(Value, array->values, array->capacity);
	InitValueArray(array);
}

void PrintValue(Value value)
{
	switch (value.type) {
	case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
	case VAL_NIL: printf("nil"); break;
	case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
	case VAL_OBJ: PrintObject(value); break;
	}
}
