#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "object.h"

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

typedef void (*ParseFn)(bool can_assign);
typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

typedef struct {
	Token name;
	int depth;
} Local;
typedef struct {
	Local locals[UINT8_COUNT];
	int local_count;
	int scope_depth;
} Compiler;

Parser parser;
Compiler* current = NULL;
Chunk* compiling_chunk;

static bool IdentifiersEqual(const Token* a, const Token* b);
static void Declaration();

static void Grouping(bool can_assign);
static void Unary(bool can_assign);
static void Binary(bool can_assign);
static void Variable(bool can_assign);
static void Number(bool can_assign);
static void String(bool can_assign);
static void Literal(bool can_assign);

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
	[TOKEN_BANG]					= {Unary,    NULL,   PREC_NONE},
	[TOKEN_BANG_EQUAL]				= {NULL,     Binary, PREC_EQUALITY},
	[TOKEN_EQUAL]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_EQUAL_EQUAL]				= {NULL,     Binary, PREC_EQUALITY},
	[TOKEN_GREATER]					= {NULL,     Binary, PREC_EQUALITY},
	[TOKEN_GREATER_EQUAL]			= {NULL,     Binary, PREC_EQUALITY},
	[TOKEN_LESS]					= {NULL,     Binary, PREC_EQUALITY},
	[TOKEN_LESS_EQUAL]				= {NULL,     Binary, PREC_EQUALITY},
	[TOKEN_IDENTIFIER]				= {Variable, NULL,   PREC_NONE},
	[TOKEN_STRING]					= {String,   NULL,   PREC_NONE},
	[TOKEN_NUMBER]					= {Number,   NULL,   PREC_NONE},
	[TOKEN_AND]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_CLASS]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_ELSE]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_FALSE]					= {Literal,  NULL,   PREC_NONE},
	[TOKEN_FOR]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_FUN]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_IF]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_NIL]						= {Literal,  NULL,   PREC_NONE},
	[TOKEN_OR]						= {NULL,     NULL,   PREC_NONE},
	[TOKEN_PRINT]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_RETURN]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_SUPER]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_THIS]					= {NULL,     NULL,   PREC_NONE},
	[TOKEN_TRUE]					= {Literal,  NULL,   PREC_NONE},
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
static bool Check(TokenType type) {
	return parser.current.type == type;
}
static bool Match(TokenType type) {
	if (!Check(type)) return false;
	
	Advance();
	return true;
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
	bool can_assign = precedence <= PREC_ASSIGNMENT;
	prefix_rule(can_assign);
	while (precedence <= GetRule(parser.current.type)->precedence) {
		Advance();
		ParseFn infix_rule = GetRule(parser.previous.type)->infix;
		infix_rule(can_assign);
	}

	if (can_assign && Match(TOKEN_EQUAL)) {
		Error("invalid assignment target.");
	}
}
static void Expression() {
	ParsePrecedence(PREC_ASSIGNMENT);
}
static void Grouping(bool can_assign) {
	Expression();
	Consume(TOKEN_RIGHT_PAREN, "expect ')' after expression.");
}
static void Number(bool can_assign) {
	double value = strtod(parser.previous.start, NULL);
	EmitConstant(NUMBER_VAL(value));
}
static void String(bool can_assign) {
	EmitConstant(OBJ_VAL(CopyString(parser.previous.start + 1, parser.previous.length - 2)));
}
static void Unary(bool can_assign) {
	TokenType type = parser.previous.type;
	ParsePrecedence(PREC_UNARY);
	switch (type) {
	case TOKEN_BANG: EmitByte(OP_NOT); break;
	case TOKEN_MINUS: EmitByte(OP_NEGATE); break;
	default: return;
	}
}
static void Binary(bool can_assign) {
	TokenType type = parser.previous.type;
	ParseRule* rule = GetRule(type);
	ParsePrecedence((Precedence)(rule->precedence + 1));
	switch (type) {
	case TOKEN_BANG_EQUAL: EmitBytes(OP_EQUAL, OP_NOT); break;
	case TOKEN_EQUAL_EQUAL: EmitByte(OP_EQUAL); break;
	case TOKEN_GREATER: EmitByte(OP_GREATER); break;
	case TOKEN_GREATER_EQUAL: EmitBytes(OP_LESS, OP_NOT); break;
	case TOKEN_LESS: EmitByte(OP_LESS); break;
	case TOKEN_LESS_EQUAL: EmitBytes(OP_GREATER, OP_NOT); break;
	case TOKEN_PLUS: EmitByte(OP_ADD); break;
	case TOKEN_MINUS: EmitByte(OP_SUBTRACT); break;
	case TOKEN_STAR: EmitByte(OP_MULTIPLY); break;
	case TOKEN_SLASH: EmitByte(OP_DIVIDE); break;
	default: return;  // unreachable
	}
}
static uint8_t IndentifierConstant(Token* name) {
	return MakeConstant(OBJ_VAL(CopyString(name->start, name->length)));
}
static ResolveLocal(Compiler* compiler, const Token* name) {
	for (int i = compiler->local_count - 1; i >= 0; i--) {
		Local* local = &compiler->locals[i];
		if (IdentifiersEqual(name, &local->name)) {
			if (local->depth == -1) {
				Error("can not read local variable in its own initializer.");
			}
			return i;
		}
	}
	return -1;
}
static void NamedVariable(Token name, bool can_assign) {
	uint8_t get_op, set_op;
	int index = ResolveLocal(current, &name);
	if (index != -1) {
		get_op = OP_GET_LOCAL;
		set_op = OP_SET_LOCAL;
	}
	else {
		index = IndentifierConstant(&name);
		get_op = OP_GET_GLOBAL;
		set_op = OP_SET_GLOBAL;
	}
	if (can_assign && Match(TOKEN_EQUAL)) {
		Expression();
		EmitBytes(set_op, index);
	}
	else {
		EmitBytes(get_op, index);
	}
}
static void Variable(bool can_assign) {
	NamedVariable(parser.previous, can_assign);
}
static void Literal(bool can_assign) {
	switch (parser.previous.type) {
	case TOKEN_FALSE: EmitByte(OP_FALSE); break;
	case TOKEN_TRUE: EmitByte(OP_TRUE); break;
	case TOKEN_NIL: EmitByte(OP_NIL); break;
	default: return;  // unreachable
	}
}
static void PrintStatement() {
	Expression();
	Consume(TOKEN_SEMICOLON, "expect ';' after value.");
	EmitByte(OP_PRINT);
}
static void ExpressionStatement() {
	Expression();
	Consume(TOKEN_SEMICOLON, "expect ';' after expression.");
	EmitByte(OP_POP);
}
static void Block() {
	while (!Check(TOKEN_RIGHT_BRACE) && !Check(TOKEN_EOF)) {
		Declaration();
	}
	Consume(TOKEN_RIGHT_BRACE, "expect '}' after block.");
}
static void BeginScope() {
	current->scope_depth++;
}
static void EndScope() {
	current->scope_depth--;
	while (current->local_count > 0 &&
		current->locals[current->local_count - 1].depth > current->scope_depth) {
		EmitByte(OP_POP);
		current->local_count--;
	}

}
static void Statement() {
	if (Match(TOKEN_PRINT)) {
		PrintStatement();
	}
	else if (Match(TOKEN_LEFT_BRACE)) {
		BeginScope();
		Block();
		EndScope();
	}
	else {
		ExpressionStatement();
	}
}
static void Synchronize() {
	parser.panic_mode = false;
	while (parser.current.type != TOKEN_EOF) {
		if (parser.current.type == TOKEN_SEMICOLON) return;
		switch (parser.current.type) {
		case TOKEN_CLASS:
		case TOKEN_FUN:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;
		default:
			break;
		}
		Advance();
	}
}
static bool IdentifiersEqual(const Token* a, const Token* b) {
	return (a->length == b->length &&
		memcmp(a->start, b->start, a->length) == 0);
}
static void AddLocal(const Token* name) {
	if (current->local_count == UINT8_COUNT) {
		Error("too many local variable in function.");
		return;
	}

	Local* local = &current->locals[current->local_count++];
	local->depth = -1;
	local->name = *name;
}
static void DeclareVariable() {
	if (current->scope_depth == 0) {
		return;
	}
	
	Token* name = &parser.previous;
	for (int i = current->local_count - 1; i >= 0; i--) {
		Local* local = &current->locals[i];
		if (local->depth != -1 && local->depth < current->scope_depth) {
			break;
		}
		if (IdentifiersEqual(name, &parser.previous)) {
			Error("already a variable with the same name in this scope.");
		}
	}
	AddLocal(name);
}
static uint8_t ParseVariable(const char* error_message) {
	Consume(TOKEN_IDENTIFIER, error_message);
	
	DeclareVariable();
	if (current->scope_depth > 0) {
		return 0;
	}

	return IndentifierConstant(&parser.previous);
}
static void MarkInitialized() {
	current->locals[current->local_count - 1].depth =
		current->scope_depth;
}
static void DefineVariable(uint8_t index) {
	if (current->scope_depth > 0) {
		MarkInitialized();
		return;
	}

	EmitBytes(OP_DEFINE_GLOBAL, index);
}
static void VarDeclaration() {
	uint8_t global = ParseVariable("expect variable name.");
	if (Match(TOKEN_EQUAL)) {
		Expression();
	}
	else {
		EmitByte(OP_NIL);
	}
	Consume(TOKEN_SEMICOLON, "expect ';' after variable declaration.");
	DefineVariable(global);
}
static void Declaration() {
	if (Match(TOKEN_VAR)) {
		VarDeclaration();
	}
	else {
		Statement();
	}

	if (parser.panic_mode) Synchronize();
}

static void InitCompiler(Compiler* compiler) {
	compiler->local_count = 0;
	compiler->scope_depth = 0;
	current = compiler;
}

bool Compile(const char* source, Chunk* chunk) {
	InitScanner(source);
	Compiler compiler;
	InitCompiler(&compiler);
	compiling_chunk = chunk;
	parser.had_error = false;
	parser.panic_mode = false;

	Advance();
	while (!Match(TOKEN_EOF)) {
		Declaration();
	}
	/*Expression();
	Consume(TOKEN_EOF, "expect end of expression.");*/
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
