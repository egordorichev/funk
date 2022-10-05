#include "funk.h"
#include <string.h>
#include <printf.h>

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
		case '{': return make_token(scanner, FUNK_TOKEN_LEFT_BRACE);
		case '}': return make_token(scanner, FUNK_TOKEN_RIGHT_BRACE);
		case ',': return make_token(scanner, FUNK_TOKEN_COMMA);

		default: return make_token(scanner, FUNK_TOKEN_EOF);
	}
}

void funk_free_object(sFunkVm* vm, FunkObject* object) {
	switch (object->type) {
		case FUNK_OBJECT_BASIC_FUNCTION: {
			FunkBasicFunction* function = (FunkBasicFunction*) object;

			if (function->code != NULL) {
				vm->freeFn((void *) function->code);
			}

			if (function->constants != NULL) {
				vm->freeFn((void *) function->constants);
			}

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

static FunkObject* allocate_object(FunkVm* vm, size_t size) {
	FunkObject* object = (FunkObject*) vm->allocFn(size);

	object->next = vm->objects;
	vm->objects = object;

	return object;
}

static uint32_t hash_string(const char* key, int length) {
	uint32_t hash = 2166136261u;

	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t) key[i];
		hash *= 16777619;
	}

	return hash;
}

FunkString* funk_create_string(sFunkVm* vm, const char* chars, uint16_t length) {
	uint32_t hash = hash_string(chars, length);
	FunkString* interned = funk_table_find_string(&vm->strings, chars, length, hash);

	if (interned != NULL) {
		return interned;
	}

	FunkString* string = (FunkString*) allocate_object(vm, sizeof(FunkString));

	string->object.type = FUNK_OBJECT_STRING;
	string->chars = (const char*) vm->allocFn(length);
	string->length = length;
	string->hash = hash;

	memcpy((void*) string->chars, chars, length);

	funk_table_set(vm, &vm->strings, string, (FunkObject*) string);

	return string;
}

const char* funk_to_string(sFunkVm* vm, FunkFunction* function) {
	return function->name->chars;
}

FunkBasicFunction* funk_create_basic_function(sFunkVm* vm, FunkString* name) {
	FunkBasicFunction* function = (FunkBasicFunction*) allocate_object(vm, sizeof(FunkBasicFunction));

	function->parent.object.type = FUNK_OBJECT_BASIC_FUNCTION;
	function->parent.name = name;
	function->argCount = 0;

	function->code = NULL;
	function->codeLength = 0;
	function->codeAllocated = 0;

	function->constants = NULL;
	function->constantsAllocated = 0;
	function->constantsLength = 0;

	return function;
}

void funk_write_instruction(sFunkVm* vm, FunkBasicFunction* function, uint8_t instruction) {
	if (function->codeAllocated < function->codeLength + 1) {
		uint16_t new_size = FUNK_GROW_CAPACITY(function->codeAllocated);
		uint8_t* new_chunk = (uint8_t*) vm->allocFn(sizeof(uint8_t) * new_size);

		memcpy((void*) new_chunk, (void*) function->code, function->codeAllocated);
		vm->freeFn((void*) function->code);

		function->code = new_chunk;
		function->codeAllocated = new_size;
	}

	function->code[function->codeLength++] = instruction;
}

uint16_t funk_add_constant(sFunkVm* vm, FunkBasicFunction* function, FunkObject* constant) {
	for (uint16_t i = 0; i < function->constantsLength; i++) {
		if (function->constants[i] == constant) {
			return i;
		}
	}

	if (function->constantsAllocated < function->constantsLength + 1) {
		uint16_t new_size = FUNK_GROW_CAPACITY(function->constantsAllocated);
		FunkObject** new_constants = (FunkObject**) vm->allocFn(sizeof(FunkObject*) * new_size);

		memcpy((void*) new_constants, (void*) function->constants, function->constantsAllocated);
		vm->freeFn((void*) function->constants);

		function->constants = new_constants;
		function->constantsAllocated = new_size;
	}

	function->constants[function->constantsLength++] = (FunkObject*) constant;
	return function->constantsLength - 1;
}


FunkNativeFunction* funk_create_native_function(sFunkVm* vm, FunkString* name, FunkNativeFn fn) {
	FunkNativeFunction* function = (FunkNativeFunction*) allocate_object(vm, sizeof(FunkNativeFunction));

	function->parent.object.type = FUNK_OBJECT_NATIVE_FUNCTION;
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

	if (compiler->previous.length == 8 && memcmp(compiler->previous.start, "function", 8) == 0) {
		consume_token(compiler, FUNK_TOKEN_NAME, "Expected function name");

		FunkVm* vm = compiler->vm;
		FunkBasicFunction* oldFunction = compiler->function;
		FunkString* name = funk_create_string(vm, compiler->previous.start, compiler->previous.length);

		compiler->function = funk_create_basic_function(vm,  name);

		consume_token(compiler, FUNK_TOKEN_LEFT_PAREN, "Expected '(' after function name");

		if (!match_token(compiler, FUNK_TOKEN_RIGHT_PAREN)) {
			do {
				consume_token(compiler, FUNK_TOKEN_NAME, "Expected argument name");
				compile_expression(compiler);

				compiler->function->argCount++;
			} while (match_token(compiler, FUNK_TOKEN_COMMA));

			consume_token(compiler, FUNK_TOKEN_RIGHT_PAREN, "Expected ')' after function arguments");
		}

		consume_token(compiler, FUNK_TOKEN_LEFT_BRACE, "Expected '{' after function arguments");

		while (!match_token(compiler, FUNK_TOKEN_RIGHT_BRACE)) {
			compile_expression(compiler);
		}

		write_uint8_t(compiler, FUNK_INSTRUCTION_PUSH_NULL);
		write_uint8_t(compiler, FUNK_INSTRUCTION_RETURN);

		FunkObject* newFunction = (FunkObject*) compiler->function;
		compiler->function = oldFunction;

		write_uint8_t(compiler, FUNK_INSTRUCTION_DEFINE);
		write_uint16_t(compiler, funk_add_constant(vm, oldFunction, newFunction));

		return;
	}

	uint16_t name = add_string_constant_for_previous_token(compiler);
	bool isACall = match_token(compiler, FUNK_TOKEN_LEFT_PAREN);

	write_uint8_t(compiler, isACall ? FUNK_INSTRUCTION_GET : FUNK_INSTRUCTION_GET_STRING);
	write_uint16_t(compiler, name);

	if (isACall) {
		uint8_t argCount = 0;

		if (!match_token(compiler, FUNK_TOKEN_RIGHT_PAREN)) {
			do {
				compile_expression(compiler);
				argCount++;
			} while (match_token(compiler, FUNK_TOKEN_COMMA));

			consume_token(compiler, FUNK_TOKEN_RIGHT_PAREN, "')' expected after function arguments");
		}

		write_uint8_t(compiler, FUNK_INSTRUCTION_CALL);
		write_uint8_t(compiler, argCount);
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

	write_uint8_t(&compiler, FUNK_INSTRUCTION_RETURN);

	return &function->parent;
}

void funk_init_table(FunkTable* table) {
	table->capacity = -1;
	table->count = 0;
	table->entries = NULL;
}

void funk_free_table(FunkVm* vm, FunkTable* table) {
	if (table->capacity > 0) {
		vm->freeFn(table->entries);
	}

	funk_init_table(table);
}

static FunkTableEntry* find_entry(FunkTableEntry* entries, int capacity, FunkString* key) {
	uint32_t index = key->hash % capacity;
	FunkTableEntry* tombstone = NULL;

	while (true) {
		FunkTableEntry* entry = &entries[index];

		if (entry->key == NULL) {
			if (entry->value == NULL) {
				return tombstone != NULL ? tombstone : entry;
			} else if (tombstone == NULL) {
				tombstone = entry;
			}
		} if (entry->key == key) {
			return entry;
		}

		index = (index + 1) % capacity;
	}
}

static void adjust_capacity(FunkVm* vm, FunkTable* table, int capacity) {
	FunkTableEntry* entries = (FunkTableEntry*) vm->allocFn(sizeof(FunkTableEntry) * (capacity + 1));

	for (int i = 0; i <= capacity; i++) {
		entries[i].key = NULL;
		entries[i].value = NULL;
	}

	table->count = 0;

	for (int i = 0; i <= table->capacity; i++) {
		FunkTableEntry* entry = &table->entries[i];

		if (entry->key == NULL) {
			continue;
		}

		FunkTableEntry* destination = find_entry(entries, capacity, entry->key);

		destination->key = entry->key;
		destination->value = entry->value;

		table->count++;
	}

	vm->freeFn(table->entries);
	table->capacity = capacity;
	table->entries = entries;
}

bool funk_table_set(FunkVm* vm, FunkTable* table, FunkString* key, FunkObject* value) {
	if (table->count + 1 > (table->capacity + 1) * TABLE_MAX_LOAD) {
		int capacity = FUNK_GROW_CAPACITY(table->capacity + 1) - 1;
		adjust_capacity(vm, table, capacity);
	}

	FunkTableEntry* entry = find_entry(table->entries, table->capacity, key);
	bool is_new = entry->key == NULL;

	if (is_new && entry->value == NULL) {
		table->count++;
	}

	entry->key = key;
	entry->value = value;

	return is_new;
}

bool funk_table_get(FunkTable* table, FunkString* key, FunkObject** value) {
	if (table->count == 0) {
		return false;
	}

	FunkTableEntry* entry = find_entry(table->entries, table->capacity, key);

	if (entry->key == NULL) {
		return false;
	}

	*value = entry->value;
	return true;
}

FunkString* funk_table_find_string(FunkTable* table, const char* chars, uint16_t length, uint32_t hash) {
	if (table->count == 0) {
		return NULL;
	}

	uint32_t index = hash % table->capacity;

	while (true) {
		FunkTableEntry* entry = &table->entries[index];

		if (entry->key == NULL) {
			if (entry->value == NULL) {
				return NULL;
			}
		} else if (entry->key->length == length &&
       entry->key->hash == hash &&
       memcmp(entry->key->chars, chars, length) == 0) {

			return entry->key;
		}

		index = (index + 1) % table->capacity;
	}
}

FunkVm* funk_create_vm(FunkAllocFn allocFn, FunkFreeFn freeFn, FunkErrorFn errorFn) {
	FunkVm* vm = (FunkVm*) allocFn(sizeof(FunkVm));

	if (vm == NULL) {
		errorFn(NULL, "Failed to allocate the vm");
		return NULL;
	}

	vm->allocFn = allocFn;
	vm->freeFn = freeFn;
	vm->errorFn = errorFn;
	vm->stackTop = vm->stack;

	funk_init_table(&vm->globals);
	funk_init_table(&vm->strings);

	return vm;
}

void funk_free_vm(FunkVm* vm) {
	if (vm == NULL) {
		return;
	}

	funk_free_table(vm, &vm->globals);
	funk_free_table(vm, &vm->strings);

	FunkObject* object = vm->objects;

	while (object != NULL) {
		FunkObject* next = object->next;
		funk_free_object(vm, object);
		object = next;
	}

	vm->freeFn((void*) vm);
}

FunkFunction* funk_run_function(FunkVm* vm, FunkFunction* function) {
	if (function->object.type == FUNK_OBJECT_NATIVE_FUNCTION) {
		FunkNativeFunction* nativeFunction = (FunkNativeFunction*) function;
		return nativeFunction->fn(vm, NULL, 0);
	}

	FunkBasicFunction* fn = (FunkBasicFunction*) function;

	register uint8_t* ip = fn->code;
	FunkObject** constants = fn->constants;

	#define READ_UINT8() (*ip++)
	#define READ_UINT16() (ip += 2, (uint16_t) ((ip[-2] << 8) | ip[-1]))
	#define READ_CONSTANT() (constants[READ_UINT16()])
	#define PUSH(value) (*vm->stackTop = value, vm->stackTop++)
	#define POP() (*(--vm->stackTop))

	while (true) {
		#ifdef FUNK_TRACE_STACK
			printf("%s ", funkInstructionNames[*ip]);

			for (FunkFunction** slot = vm->stack; slot < vm->stackTop; slot++) {
				if (*slot == NULL) {
					printf("[ null ]");
					continue;
				}

				printf("[ %s ]", (*slot)->name->chars);
			}

			printf("\n");
		#endif

		switch (*ip++) {
			case FUNK_INSTRUCTION_RETURN: {
				return POP();
			}

			case FUNK_INSTRUCTION_CALL: {
				uint8_t argCount = READ_UINT8();

				FunkFunction** stackTop = vm->stackTop - argCount - 1;
				FunkFunction* callee = *stackTop;
				FunkFunction* result;

				if (callee == NULL) {
					vm->errorFn(vm, "Attempt to call a null value");
					return NULL;
				}

				if (callee->object.type == FUNK_OBJECT_NATIVE_FUNCTION) {
					FunkNativeFunction* nativeFunction = (FunkNativeFunction*) callee;
					result = nativeFunction->fn(vm, (vm->stackTop - argCount), argCount);
				} else {
					FunkBasicFunction* basicFunction = (FunkBasicFunction*) callee;
					vm->stackTop = stackTop;

					for (uint8_t i = argCount; i < basicFunction->argCount; i++) {
						vm->stackTop[i + 1] = NULL;
					}

					result = funk_run_function(vm, callee);
				}

				vm->stackTop = stackTop - 1;
				PUSH(result);

				break;
			}

			case FUNK_INSTRUCTION_GET: {
				FunkString* name = (FunkString*) READ_CONSTANT();
				FunkFunction* result = NULL;

				funk_table_get(&vm->globals, name, (FunkObject**) &result);
				PUSH(result);

				break;
			}

			case FUNK_INSTRUCTION_GET_STRING: {
				FunkString* name = (FunkString*) READ_CONSTANT();
				FunkFunction* result = NULL;

				if (!funk_table_get(&vm->globals, name, (FunkObject**) &result)) {
					result = (FunkFunction*) funk_create_basic_function(vm, name);
				}

				PUSH(result);
				break;
			}

			case FUNK_INSTRUCTION_POP: {
				POP();
				break;
			}

			case FUNK_INSTRUCTION_DEFINE: {
				FunkBasicFunction* basicFunction = (FunkBasicFunction*) READ_CONSTANT();
				funk_table_set(vm, &vm->globals, basicFunction->parent.name, (FunkObject*) basicFunction);

				break;
			}

			case FUNK_INSTRUCTION_PUSH_NULL: {
				PUSH(NULL);
				break;
			}

			default: {
				vm->errorFn(vm, "Unknown instruction");
				return NULL;
			}
		}
	}

	#undef READ_UINT8
	#undef READ_UINT16
	#undef PUSH
	#undef POP

	return NULL;
}

FunkFunction* funk_run_string(FunkVm* vm, const char* name, const char* string) {
	FunkFunction* function = funk_compile_string(vm, name, string);
	return funk_run_function(vm, function);
}

void funk_set_global(FunkVm* vm, const char* name, FunkFunction* function) {
	funk_table_set(vm, &vm->globals, funk_create_string(vm, name, strlen(name)), (FunkObject*) function);
}

void funk_define_native(FunkVm* vm, const char* name, FunkNativeFn fn) {
	FunkString* name_string = funk_create_string(vm, name, strlen(name));
	funk_table_set(vm, &vm->globals, name_string, (FunkObject*) funk_create_native_function(vm, name_string, fn));
}