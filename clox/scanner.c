#include "scanner.h"

#include <stdio.h>
#include <string.h>

#include "common.h"

Scanner scanner;

void InitScanner(const char* source) {
	scanner.start = source;
	scanner.current = source;
	scanner.line = 1;
}

static bool IsAtEnd() {
	return (*scanner.current == '\0');
}
static char Advance() {
	return *scanner.current++;
}
static bool Match(char expected) {
	if (IsAtEnd() || *scanner.current != expected) return false;
	scanner.current++;
	return true;
}
static char Peek() {
	return *scanner.current;
}
static char PeekNext() {
	return IsAtEnd()? '\0' : scanner.current[1];
}
static bool IsDigit(char c) {
	return (c >= '0' && c <= '9');
}
static bool IsAlpha(char c) {
	return ((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		c == '_');
}
static TokenType CheckKeyword(
	int start, int length, const char* rest, TokenType type) {
	if (scanner.current - scanner.start == start + length &&
		memcmp(scanner.start + start, rest, length) == 0) {
		return type;
	}
	return TOKEN_IDENTIFIER;
}

static Token MakeToken(TokenType type) {
	Token token;
	token.type = type;
	token.start = scanner.start;
	token.length = (int)(scanner.current - scanner.start);
	token.line = scanner.line;
	return token;
}
static Token ErrorToken(const char* message) {
	Token token;
	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = (int)strlen(message);
	token.line = scanner.line;
	return token;
}
static TokenType identifierType() {
	switch (scanner.start[0]) {
	case 'a': return CheckKeyword(1, 2, "nd", TOKEN_AND);
	case 'c': return CheckKeyword(1, 4, "lass", TOKEN_CLASS);
	case 'e': return CheckKeyword(1, 3, "lse", TOKEN_ELSE);
	case 'f': {
		if (scanner.current - scanner.start > 1) {
			switch (scanner.start[1]) {
			case 'a': return CheckKeyword(2, 3, "lse", TOKEN_FALSE);
			case 'o': return CheckKeyword(2, 1, "r", TOKEN_FOR);
			case 'u': return CheckKeyword(2, 1, "n", TOKEN_FUN);
			default: break;
			}
		}
	}
	case 'i': return CheckKeyword(1, 1, "f", TOKEN_IF);
	case 'n': return CheckKeyword(1, 2, "il", TOKEN_NIL);
	case 'o': return CheckKeyword(1, 1, "r", TOKEN_OR);
	case 'p': return CheckKeyword(1, 4, "rint", TOKEN_PRINT);
	case 'r': return CheckKeyword(1, 5, "eturn", TOKEN_RETURN);
	case 's': return CheckKeyword(1, 4, "uper", TOKEN_SUPER);
	case 't': {
		switch (scanner.start[1]) {
		case 'h': return CheckKeyword(2, 2, "is", TOKEN_THIS);
		case 'r': return CheckKeyword(2, 2, "ue", TOKEN_TRUE);
		default: break;
		}
	case 'v': return CheckKeyword(1, 2, "ar", TOKEN_VAR);
	case 'w': return CheckKeyword(1, 4, "hile", TOKEN_WHILE);
	default: break;
	}
	}
	return TOKEN_IDENTIFIER;
}
static Token String() {
	while (Peek() != '"' && !IsAtEnd()) {
		if (Peek() == '\n') scanner.line++;
		Advance();
	}
	if (IsAtEnd()) return ErrorToken("unterminated string.");
	Advance();
	return MakeToken(TOKEN_STRING);
}
static Token Number() {
	while (IsDigit(Peek())) Advance();

	if (Peek() == '.' && IsDigit(PeekNext())) {
		Advance();
		while (IsDigit(Peek())) Advance();
	}
	return MakeToken(TOKEN_NUMBER);
}
static Token Identifier() {
	while (IsAlpha(Peek()) || IsDigit(Peek())) Advance();
	return MakeToken(identifierType());
}
static void SkipWhitespace() {
	for (;;) {
		char c = Peek();
		switch (c) {
		case ' ':
		case '\t':
		case '\r':
			Advance();
			break;
		case '\n':
			scanner.line++;
			Advance();
			break;
		case '/':
			if (PeekNext() == '/') {
				while (Peek() != '\n' && !IsAtEnd()) Advance();
			}
			else {
				return;
			}
			break;
		default:
			return;
		}
	}
}

Token ScanToken() {
	SkipWhitespace();
	scanner.start = scanner.current;

	if (IsAtEnd()) return MakeToken(TOKEN_EOF);

	char c = Advance();
	if (IsAlpha(c)) return Identifier();
	if (IsDigit(c)) return Number();

	switch (c) {
	case '(': return MakeToken(TOKEN_LEFT_PAREN);
	case ')': return MakeToken(TOKEN_RIGHT_PAREN);
	case '{': return MakeToken(TOKEN_LEFT_BRACE);
	case '}': return MakeToken(TOKEN_RIGHT_BRACE);
	case ',': return MakeToken(TOKEN_COMMA);
	case '.': return MakeToken(TOKEN_DOT);
	case '-': return MakeToken(TOKEN_MINUS);
	case '+': return MakeToken(TOKEN_PLUS);
	case ';': return MakeToken(TOKEN_SEMICOLON);
	case '/': return MakeToken(TOKEN_SLASH);
	case '*': return MakeToken(TOKEN_STAR);
	case '!': return MakeToken(Match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
	case '=': return MakeToken(Match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
	case '>': return MakeToken(Match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
	case '<': return MakeToken(Match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
	case '"': return String();

	}
	return ErrorToken("unexpected character.");
}
