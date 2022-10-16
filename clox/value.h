#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

typedef double Value;

typedef struct {
	int capacity;
	int count;
	Value* values;
} ValueArray;

void InitValueArray(ValueArray* array);
void WriteValueArray(ValueArray* array, Value value);
void FreeValueArray(ValueArray* array);
void PrintValue(Value value);

#endif // !CLOX_VALUE_H
