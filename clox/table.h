#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "object.h"

typedef struct {
	ObjString* key;
	Value value;
} Entry;

typedef struct {
	int count;
	int capacity;
	Entry* entries;
} Table;


void InitTable(Table* table);
void FreeTable(Table* table);
bool TableGet(Table* table, ObjString* key, Value* value);
bool TableSet(Table* table, ObjString* key, Value value);
bool TableDelete(Table* table, ObjString* key);
void TableAddAll(Table* dest, Table* src);
ObjString* TableFindString(Table* table,
	const char* src, int length, uint32_t hash);

#endif // !CLOX_TABLE_H
