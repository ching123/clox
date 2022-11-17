#include "compiler.h"

#include <stdio.h>

#include "scanner.h"

void Compile(const char* source) {
	InitScanner(source);
	int line = -1;
	for (;;) {
		Token token = ScanToken();
		if (line != token.line) {
			printf("%4d ", token.line);
			line = token.line;
		}
		else {
			printf("   | ");
		}
		printf("%2d '%.*s'\n", token.type, token.length, token.start);

		if (token.type == TOKEN_EOF) break;
	}
}
