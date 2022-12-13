#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>

#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif // DEBUG_PRINT_CODE


typedef struct {
	Token previous;
	Token current;
	bool had_error;
	bool panic_mode;
} Parser;

typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT,  // =
	PREC_OR,          // or
	PREC_AND,         // and
	PREC_EQUALITY,    // == !=
	PREC_COMPARISON,  // < > <= >=
	PREC_TERM,        // + -
	PREC_FACTOR,      // * /
	PREC_UNARY,       // ! -
	PREC_CALL,        // . ()
	PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();
typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compiling_chunk;
static void Grouping();
static void Unary();
static void Binary();
static void Number();
ParseRule rules[] = {
	[TOKEN_LEFT_PAREN]				= {Grouping, NULL,   PREC_NONE},
	[TOKEN_RIGHT_PAREN]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_LEFT_BRACE]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_RIGHT_BRACE]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_COMMA]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_DOT]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_MINUS]					= {Unary,    Binary, PREC_TERM},
	[TOKEN_PLUS]					= {NULL,     Binary, PREC_TERM},
	[TOKEN_SEMICOLON]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_SLASH]					= {NULL,     Binary, PREC_FACTOR},
	[TOKEN_STAR]					= {NULL,     Binary, PREC_FACTOR},
	[TOKEN_BANG]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_BANG_EQUAL]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_EQUAL]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_EQUAL_EQUAL]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_GREATER]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_GREATER_EQUAL]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_LESS]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_LESS_EQUAL]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_IDENTIFIER]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_STRING]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_NUMBER]					= {Number,   NULL,   PREC_NONE},
	[TOKEN_AND]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_CLASS]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_ELSE]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_FALSE]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_FOR]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_FUN]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_IF]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_NIL]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_OR]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_PRINT]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_RETURN]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_SUPER]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_THIS]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_TRUE]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_VAR]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_WHILE]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_ERROR]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_EOF]						= {NULL,     NULL,   PREC_NONE},
};

static Chunk* CurrentChunk() {
	return compiling_chunk;
}
static void ErrorAt(Token* token, const char* message) {
	if (parser.panic_mode) return;
	parser.panic_mode = true;
	fprintf(stderr, "[line %d] Error", token->line);
	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	}
	else if (token->type == TOKEN_ERROR) {

	}
	else {
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}
	fprintf(stderr, ": %s\n", message);
	parser.had_error = true;
}
static void ErrorAtCurrent(const char* message) {
	ErrorAt(&parser.current, message);
}
static void Error(const char* message) {
	ErrorAt(&parser.previous, message);
}
static void Advance() {
	parser.previous = parser.current;
	for (;;) {
		parser.current = ScanToken();
		if (parser.current.type != TOKEN_ERROR) break;

		ErrorAtCurrent(parser.current.start);
	}
}
static void Consume(TokenType type, const char* message) {
	if (parser.current.type == type) {
		Advance();
		return;
	}
	ErrorAtCurrent(message);
}
static void EmitByte(uint8_t byte) {
	WriteChunk(CurrentChunk(), byte, parser.previous.line);
}
static void EmitBytes(uint8_t byte1, uint8_t byte2) {
	EmitByte(byte1);
	EmitByte(byte2);
}
static void EmitReturn() {
	EmitByte(OP_RETURN);
}
static uint8_t MakeConstant(Value value) {
	int index = AddConstant(CurrentChunk(), value);
	if (index > UINT8_MAX) {
		Error("too many constant in one chunk.");
		return 0;
	}
	return (uint8_t)index;
}
static void EmitConstant(Value value) {
	EmitBytes(OP_CONSTANT, MakeConstant(value));
}
static void EndCompiler() {
	EmitReturn();
#ifdef DEBUG_PRINT_CODE
	if (!parser.had_error) {
		DisassembleChunk(CurrentChunk(), "code");
	}
#endif // DEBUG_PRINT_CODE

}
static ParseRule* GetRule(TokenType type) {
	return &rules[type];
}
static void ParsePrecedence(Precedence precedence) {
	Advance();
	ParseFn prefix_rule = GetRule(parser.previous.type)->prefix;
	if (prefix_rule == NULL) {
		Error("expect expression.");
		return;
	}
	prefix_rule();
	while (precedence <= GetRule(parser.current.type)->precedence) {
		Advance();
		ParseFn infix_rule = GetRule(parser.previous.type)->infix;
		infix_rule();
	}
}
static void Expression() {
	ParsePrecedence(PREC_ASSIGNMENT);
}
static void Grouping() {
	Expression();
	Consume(TOKEN_RIGHT_PAREN, "expect ')' after expression.");
}
static void Number() {
	double value = strtod(parser.previous.start, NULL);
	EmitConstant(value);
}
static void Unary() {
	TokenType type = parser.previous.type;
	ParsePrecedence(PREC_UNARY);
	switch (type) {
	case TOKEN_MINUS: EmitByte(OP_NEGATE); break;
	default: return;
	}
}
static void Binary() {
	TokenType type = parser.previous.type;
	ParseRule* rule = GetRule(type);
	ParsePrecedence((Precedence)(rule->precedence + 1));
	switch (type) {
	case TOKEN_PLUS: EmitByte(OP_ADD); break;
	case TOKEN_MINUS: EmitByte(OP_SUBTRACT); break;
	case TOKEN_STAR: EmitByte(OP_MULTIPLY); break;
	case TOKEN_SLASH: EmitByte(OP_DIVIDE); break;
	default: return;  // unreachable
	}
}

bool Compile(const char* source, Chunk* chunk) {
	InitScanner(source);
	compiling_chunk = chunk;
	parser.had_error = false;
	parser.panic_mode = false;

	Advance();
	Expression();
	Consume(TOKEN_EOF, "expect end of expression.");
	EndCompiler();

  	return !parser.had_error;

	/*int line = -1;
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
	}*/
}
