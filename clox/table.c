#include "table.h"

#include <string.h>

#include "memory.h"

#define TABLE_MAX_LOAD (0.75)

static Entry* FindEntry(Entry* entries, int capacity, ObjString* key) {
	uint32_t index = key->hash % capacity;
	Entry* tombstone = NULL;
	for (;;) {
		Entry* entry = &entries[index];
		if (entry->key == NULL) {
			if (IS_NIL(entry->value)) {
				return (tombstone == NULL) ? entry : tombstone;
			}
			else {
				if (tombstone == NULL) {
					tombstone = entry;
				}
			}
		} else if (entry->key == key) {
			return entry;
		}
		index = (index + 1) % capacity;
	}
}
static void AdjustCapacity(Table* table, int capacity) {
	Entry* entries = ALLOCATE(Entry, capacity);
	for (int i = 0; i < capacity; i++) {
		entries[i].key = NULL;
		entries[i].value = NIL_VAL;
	}

	table->count = 0;
	for (int i = 0; i < table->capacity; i++) {
		Entry* entry = &table->entries[i];
		if (entry->key != NULL) {
			Entry* dest = FindEntry(entries, capacity, entry->key);
			dest->key = entry->key;
			dest->value = entry->value;
			table->count++;
		}
	}

	FREE_ARRAY(Entry, table->entries, table->capacity);
	table->capacity = capacity;
	table->entries = entries;
}

void InitTable(Table* table)
{
	table->capacity = 0;
	table->count = 0;
	table->entries = NULL;
}

void FreeTable(Table* table)
{
	FREE_ARRAY(Entry, table->entries, table->capacity);
	InitTable(table);
}

bool TableGet(Table* table, ObjString* key, Value* value)
{
	if (table->count == 0) {
		return false;
	}
	Entry* entry = FindEntry(table->entries, table->capacity, key);
	if (entry->key == NULL) {
		return false;
	}
	*value = entry->value;
	return true;
}

bool TableSet(Table* table, ObjString* key, Value value)
{
	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
		int capacity = GROW_CAPACITY(table->capacity);
		AdjustCapacity(table, capacity);
	}

	Entry* entry = FindEntry(table->entries, table->capacity, key);
	bool is_new = (entry->key == NULL);
	if (is_new && IS_NIL(entry->value)) table->count++;

	entry->key = key;
	entry->value = value;
	return is_new;
}

bool TableDelete(Table* table, ObjString* key)
{
	if (table->count == 0) {
		return false;
	}

	Entry* entry = FindEntry(table->entries, table->capacity, key);
	if (entry->key == NULL) {
		return false;
	}
	entry->key = NULL;
	entry->value = BOOL_VAL(true);
	return true;
}

void TableAddAll(Table* dest, Table* src)
{
	for (int i = 0; i < src->capacity; i++) {
		Entry* entry = &src->entries[i];
		if (entry->key != NULL) {
			TableSet(dest, entry->key, entry->value);
		}
	}
}

ObjString* TableFindString(Table* table, const char* src, int length, uint32_t hash)
{
	if (table->count == 0) {
		return NULL;
	}

	uint32_t index = hash % table->capacity;
	for (;;) {
		Entry* entry = &table->entries[index];
		if (entry->key == NULL) {
			if (IS_NIL(entry->value)) {
				return NULL;
			}
		}
		else if (entry->key->length == length &&
			entry->key->hash == hash &&
			memcmp(entry->key->str, src, length) == 0) {
			return entry->key;
		}
		index = (index + 1) % table->capacity;
	}
	return NULL;
}
