#include "memory.h"

#include <stdlib.h>


void* reallocate(void* pointer, int old_capacity, int new_capacity)
{
	if (new_capacity == 0) {
		free(pointer);
		return NULL;
	}

	void* result = realloc(pointer, new_capacity);
	if (result == NULL) exit(1);
	return result;
}
