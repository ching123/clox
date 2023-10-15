#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "value.h"
#include "chunk.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) IsObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) IsObjType(value, OBJ_FUNCTION)
#define IS_CLOSURE(value) IsObjType(value, OBJ_CLOSURE)
#define IS_NATIVE(value) IsObjType(value, OBJ_NATIVE)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)

#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->str)


typedef enum {
	OBJ_STRING,
	OBJ_UPVALUE,
	OBJ_FUNCTION,
	OBJ_CLOSURE,
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

typedef struct ObjUpvalue {
	Obj obj;
	Value* location;
	Value closed;
	struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct {
	Obj obj;
	int upvalue_count;
	int arity;
	Chunk chunk;
	ObjString* name;
} ObjFunction;

typedef struct {
	Obj obj;
	ObjFunction* function;
	ObjUpvalue** upvalues;
	int upvalue_count;
} ObjClosure;

typedef Value(*NativeFn)(int argument_count, Value* args);

typedef struct {
	Obj obj;
	NativeFn function;
} ObjNative;

ObjString* TakeString(char* src, int length);
ObjString* CopyString(const char* src, int length);
ObjUpvalue* NewUpvalue(Value* value);
ObjFunction* NewFunction();
ObjClosure* NewClosure(ObjFunction* function);
ObjNative* NewNative(NativeFn function);
void PrintObject(Value value);

static inline bool IsObjType(Value value, ObjType type) {
	return IS_OBJ(value) && (AS_OBJ(value)->type == type);
}

#endif // !CLOX_OBJECT_H
