#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "value.h"
#include "chunk.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) IsObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) IsObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) IsObjType(value, OBJ_NATIVE)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)

#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->str)


typedef enum {
	OBJ_STRING,
	OBJ_FUNCTION,
	OBJ_NATIVE,
} ObjType;

struct Obj {
	ObjType type;
	struct Obj* next;
};

struct ObjString {
	Obj obj;
	int length;
	char* str;
	uint32_t hash;
};

typedef struct {
	Obj obj;
	int arity;
	Chunk chunk;
	ObjString* name;
} ObjFunction;

typedef Value(*NativeFn)(int argument_count, Value* args);

typedef struct {
	Obj obj;
	NativeFn function;
} ObjNative;

ObjString* TakeString(char* src, int length);
ObjString* CopyString(const char* src, int length);
ObjFunction* NewFunction();
ObjNative* NewNative(NativeFn function);
void PrintObject(Value value);

static inline bool IsObjType(Value value, ObjType type) {
	return IS_OBJ(value) && (AS_OBJ(value)->type == type);
}

#endif // !CLOX_OBJECT_H
