#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"


#define ALLOCATE(type, count) \
	(type*)reallocate(NULL, 0, sizeof(type) * (count))

#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : 2 * (capacity))

#define GROW_ARRAY(type, pointer, old_capacity, new_capacity) \
	(type*)reallocate(pointer, (old_capacity) * sizeof(type), \
	(new_capacity) * sizeof(type))

#define FREE_ARRAY(type, pointer, old_capacity) \
	reallocate(pointer, (old_capacity) * sizeof(type), 0)

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)
		
void* reallocate(void* pointer, size_t old_capacity, size_t new_capacity);
void FreeObjects();

#endif // !CLOX_MEMORY_H
