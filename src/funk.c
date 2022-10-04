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

static char advance_char(FunkScanner* scanner) {
	scanner->current++;
	return scanner->current[-1];
}

static char peek_char(FunkScanner* scanner) {
	return *scanner->current;
}

static char peek_char_next(FunkScanner* scanner) {
	if (is_at_end(scanner)) {
		return '\0';
	}

	return scanner->current[1];
}

static void skip_whitespace(FunkScanner* scanner) {
	while (true) {
		char c = peek_char(scanner);

		switch (c) {
			case ' ':
			case '\r':
			case '\t': {
				advance_char(scanner);
				break;
			}

			case '\n': {
				scanner->line++;
				advance_char(scanner);
				break;
			}

			case '/': {
				if (peek_char_next(scanner) == '/') {
					while (peek_char(scanner) != '\n' && !is_at_end(scanner)) {
						advance_char(scanner);
					}
				} else if (peek_char_next(scanner) == '*') {
					advance_char(scanner);
					advance_char(scanner);

					while ((peek_char(scanner) != '*' || peek_char_next(scanner) != '/') && !is_at_end(scanner)) {
						if (peek_char(scanner) == '\n') {
							scanner->line++;
						}

						advance_char(scanner);
					}

					advance_char(scanner);
					advance_char(scanner);
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

	char c = advance_char(scanner);

	if (is_alpha(c)) {
		while (is_alpha(peek_char(scanner))) {
			advance_char(scanner);
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

const char* funk_to_string(sFunkVm* vm, FunkFunction* function) {
	return function->name->chars;
}

FunkBasicFunction* funk_create_basic_function(sFunkVm* vm, FunkString* name) {
	FunkBasicFunction* function = (FunkBasicFunction*) vm->allocFn(sizeof(FunkBasicFunction));

	function->parent.name = name;
	function->code = NULL;
	function->code_length = 0;
	function->code_allocated = 0;

	function->constants = NULL;
	function->constants_allocated = 0;
	function->constants_length = 0;

	return function;
}

void funk_write_instruction(sFunkVm* vm, FunkBasicFunction* function, uint8_t instruction) {
	if (function->code_allocated < function->code_length + 1) {
		uint16_t new_size = FUNK_GROW_CAPACITY(function->code_allocated);
		uint8_t* new_chunk = (uint8_t*) vm->allocFn(sizeof(uint8_t) * new_size);

		memcpy((void*) new_chunk, (void*) function->code, function->code_allocated);
		vm->freeFn((void*) function->code);

		function->code = new_chunk;
		function->code_allocated = new_size;
	}

	function->code[function->code_length++] = instruction;
}

uint16_t funk_add_constant(sFunkVm* vm, FunkBasicFunction* function, FunkObject* constant) {
	for (uint16_t i = 0; i < function->constants_length; i++) {
		if (function->constants[i] == constant) {
			return i;
		}
	}

	if (function->constants_allocated < function->constants_length + 1) {
		uint16_t new_size = FUNK_GROW_CAPACITY(function->constants_allocated);
		FunkObject** new_constants = (FunkObject**) vm->allocFn(sizeof(FunkObject*) * new_size);

		memcpy((void*) new_constants, (void*) function->constants, function->constants_allocated);
		vm->freeFn((void*) function->constants);

		function->constants = new_constants;
		function->constants_allocated = new_size;
	}

	function->constants[function->constants_length++] = (FunkObject*) constant;
	return function->constants_length - 1;
}


FunkNativeFunction* funk_create_native_function(sFunkVm* vm, FunkString* name, Funk_NativeFn fn) {
	FunkNativeFunction* function = (FunkNativeFunction*) vm->allocFn(sizeof(FunkNativeFunction));

	function->parent.name = name;
	function->fn = fn;

	return function;
}

static void advance_token(FunkCompiler* compiler) {
	compiler->previous = compiler->current;
	compiler->current = funk_scan_token(compiler->scanner);
}

static void compilation_error(FunkCompiler* compiler, const char* error) {
	// TODO: print the line
	compiler->vm->errorFn(compiler->vm, error);
}

static void consume_token(FunkCompiler* compiler, FunkTokenType type, const char* message) {
	if (compiler->current.type == type) {
		return advance_token(compiler);
	}

	compilation_error(compiler, message);
}

static bool match_token(FunkCompiler* compiler, FunkTokenType type) {
	if (compiler->current.type == type) {
		advance_token(compiler);
		return true;
	}

	return false;
}

static uint16_t add_string_constant(FunkCompiler* compiler, const char* string, uint16_t length) {
	FunkString* string_object = funk_create_string(compiler->vm, string, length);
	return funk_add_constant(compiler->vm, compiler->function, (FunkObject*) string_object);
}

static uint16_t add_string_constant_for_previous_token(FunkCompiler* compiler) {
	return add_string_constant(compiler, compiler->previous.start, compiler->previous.length);
}

static void write_uint8_t(FunkCompiler* compiler, uint8_t byte) {
	funk_write_instruction(compiler->vm, compiler->function, byte);
}

static void write_uint16_t(FunkCompiler* compiler, uint16_t byte) {
	funk_write_instruction(compiler->vm, compiler->function, (uint8_t) ((byte << 8) & 0xff));
	funk_write_instruction(compiler->vm, compiler->function, (uint8_t) (byte & 0xff));
}

static void compile_expression(FunkCompiler* compiler) {
	consume_token(compiler, FUNK_TOKEN_NAME, "Function name expected");
	uint16_t name = add_string_constant_for_previous_token(compiler);

	write_uint8_t(compiler, FUNK_INSTRUCTION_GET);
	write_uint16_t(compiler, name);

	if (match_token(compiler, FUNK_TOKEN_LEFT_PAREN)) {
		uint8_t arg_count = 0;

		if (!match_token(compiler, FUNK_TOKEN_RIGHT_PAREN)) {
			do {
				compile_expression(compiler);
				arg_count++;
			} while (match_token(compiler, FUNK_TOKEN_COMMA));

			consume_token(compiler, FUNK_TOKEN_RIGHT_PAREN, "')' expected after function arguments");
		}

		write_uint8_t(compiler, FUNK_INSTRUCTION_CALL);
		write_uint8_t(compiler, arg_count);
	}
}

FunkFunction* funk_compile_string(sFunkVm* vm, const char* name, const char* string) {
	FunkString* string_name = funk_create_string(vm, name, strlen(name));
	FunkBasicFunction* function = funk_create_basic_function(vm, string_name);

	FunkScanner scanner;
	funk_init_scanner(&scanner, string);

	FunkCompiler compiler;

	compiler.vm = vm;
	compiler.scanner = &scanner;
	compiler.function = function;

	advance_token(&compiler);

	while (compiler.current.type != FUNK_TOKEN_EOF) {
		compile_expression(&compiler);
	}

	for (uint16_t i = 0; i < function->code_length; i++) {
		printf("%i\n", function->code[i]);
	}

	return &function->parent;
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

bool funk_run_function(FunkVm* vm, FunkFunction* function) {
	if (function->object.type == FUNK_OBJECT_NATIVE_FUNCTION) {
		FunkNativeFunction* nativeFunction = (FunkNativeFunction*) function;
		nativeFunction->fn(vm, NULL, 0);

		return true;
	}
}

bool funk_run_string(FunkVm* vm, const char* name, const char* string) {
	FunkFunction* function = funk_compile_string(vm, name, string);
	return funk_run_function(vm, function);
}