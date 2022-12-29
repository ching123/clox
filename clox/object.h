#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) IsObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->str)

typedef enum {
	OBJ_STRING,
} ObjType;

struct Obj {
	ObjType type;
	struct Obj* next;
};

struct ObjString {
	Obj obj;
	int length;
	char* str;
};

ObjString* TakeString(char* src, int length);
ObjString* CopyString(const char* src, int length);
void PrintObject(Value value);

static inline bool IsObjType(Value value, ObjType type) {
	return IS_OBJ(value) && (AS_OBJ(value)->type == type);
}

#endif // !CLOX_OBJECT_H
