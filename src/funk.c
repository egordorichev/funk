#include "funk.h"
#include <stdio.h>
#include <string.h>

void funk_init_scanner(FunkScanner* scanner, const char* code) {
	scanner->start = code;
	scanner->current = code;
	scanner->line = 1;
}

static bool is_at_end(FunkScanner* scanner) {
	return *scanner->current == '\0';
}

static char advance(FunkScanner* scanner) {
	scanner->current++;
	return scanner->current[-1];
}

static char peek(FunkScanner* scanner) {
	return *scanner->current;
}

static char peek_next(FunkScanner* scanner) {
	if (is_at_end(scanner)) {
		return '\0';
	}

	return scanner->current[1];
}

static void skip_whitespace(FunkScanner* scanner) {
	while (true) {
		char c = peek(scanner);

		switch (c) {
			case ' ':
			case '\r':
			case '\t': {
				advance(scanner);
				break;
			}

			case '\n': {
				scanner->line++;
				advance(scanner);
				break;
			}

			case '/': {
				if (peek_next(scanner) == '/') {
					while (peek(scanner) != '\n' && !is_at_end(scanner)) {
						advance(scanner);
					}
				} else if (peek_next(scanner) == '*') {
					advance(scanner);
					advance(scanner);

					while ((peek(scanner) != '*' || peek_next(scanner) != '/') && !is_at_end(scanner)) {
						if (peek(scanner) == '\n') {
							scanner->line++;
						}

						advance(scanner);
					}

					advance(scanner);
					advance(scanner);
				}

				break;
			}

			default: return;
		}
	}
}

static FunkToken make_token(FunkScanner* scanner, FunkTokenType type) {
	FunkToken token;

	token.type = type;
	token.start = scanner->start;
	token.length = (uint16_t) (scanner->current - scanner->start);
	token.line = scanner->line;

	return token;
}

static bool is_alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

FunkToken funk_scan_token(FunkScanner* scanner) {
	skip_whitespace(scanner);
	scanner->start = scanner->current;

	if (is_at_end(scanner)) {
		return make_token(scanner, FUNK_TOKEN_EOF);
	}

	char c = advance(scanner);

	if (is_alpha(c)) {
		while (is_alpha(peek(scanner))) {
			advance(scanner);
		}

		return make_token(scanner, FUNK_TOKEN_NAME);
	}

	switch (c) {
		case '(': return make_token(scanner, FUNK_TOKEN_LEFT_PAREN);
		case ')': return make_token(scanner, FUNK_TOKEN_RIGHT_PAREN);
		case ',': return make_token(scanner, FUNK_TOKEN_COMMA);

		default: return make_token(scanner, FUNK_TOKEN_EOF);
	}
}

void funk_free_object(sFunkVm* vm, FunkObject* object) {
	switch (object->type) {
		case FUNK_OBJECT_BASIC_FUNCTION: {
			FunkBasicFunction* function = (FunkBasicFunction*) object;
			vm->freeFn((void*) function->code);

			break;
		}

		case FUNK_OBJECT_STRING: {
			FunkString* string = (FunkString*) object;
			vm->freeFn((void*) string->chars);

			break;
		}

		case FUNK_OBJECT_NATIVE_FUNCTION: {
			break;
		}

		default: {
			UNREACHABLE
		}
	}

	vm->freeFn((void*) object);
}

FunkString* funk_create_string(sFunkVm* vm, const char* chars, uint16_t length) {
	FunkString* string = (FunkString*) vm->allocFn(sizeof(FunkString));

	string->chars = (const char*) vm->allocFn(length);
	string->length = length;

	memcpy((void*) string->chars, chars, length);

	return string;
}

FunkBasicFunction* funk_create_basic_function(sFunkVm* vm, FunkString* name) {
	FunkBasicFunction* function = (FunkBasicFunction*) vm->allocFn(sizeof(FunkBasicFunction));

	function->parent.name = name;
	function->code = NULL;
	function->code_length = 0;
	function->code_allocated = 0;

	return function;
}

void funk_write_instruction(sFunkVm* vm, FunkBasicFunction* function, FunkInstruction instruction) {
	if (function->code_allocated < function->code_length + 1) {
		uint16_t new_size = FUNK_GROW_CAPACITY(function->code_allocated);
		uint8_t* new_chunk = (uint8_t*) vm->allocFn(sizeof(uint8_t) * new_size);

		memcpy((void*) new_chunk, (void*) function->code, function->code_allocated);
		vm->freeFn((void*) function->code);

		function->code = new_chunk;
		function->code_allocated = new_size;
	}

	function->code[function->code_length++] = (uint8_t) instruction;
}

FunkNativeFunction* funk_create_native_function(sFunkVm* vm, FunkString* name, Funk_NativeFn fn) {
	FunkNativeFunction* function = (FunkNativeFunction*) vm->allocFn(sizeof(FunkNativeFunction));

	function->parent.name = name;
	function->fn = fn;

	return function;
}

FunkVm* funk_create_vm(Funk_AllocFn allocFn, Funk_FreeFn freeFn, Funk_ErrorFn errorFn) {
	FunkVm* vm = (FunkVm*) allocFn(sizeof(FunkVm));

	if (vm == NULL) {
		errorFn(NULL, "Failed to allocate the vm");
		return NULL;
	}

	vm->allocFn = allocFn;
	vm->freeFn = freeFn;
	vm->errorFn = errorFn;

	return vm;
}

void funk_free_vm(FunkVm* vm) {
	if (vm == NULL) {
		return;
	}

	vm->freeFn((void*) vm);
}

bool funk_run_string(FunkVm* vm, const char* string) {
	FunkScanner scanner;
	FunkToken token;

	funk_init_scanner(&scanner, string);

	do {
		token = funk_scan_token(&scanner);
		printf("%i\n", token.type);
	} while (token.type != FUNK_TOKEN_EOF);

	return true;
}