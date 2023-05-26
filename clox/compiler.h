#ifndef CLOC_COMPILER_H
#define CLOC_COMPILER_H

#include "vm.h"

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

ObjFunction* Compile(const char* source);

#endif // !CLOC_COMPILER_H
