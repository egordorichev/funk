#include "funk.h"

#include <string.h>
#include <printf.h>
#include <math.h>

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
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-';
}

static bool is_digit(char c) {
	return c >= '0' && c <= '9';
}

static FunkTokenType decide_token_type(FunkScanner* scanner) {
	uint16_t length = (uint16_t) (scanner->current - scanner->start);

	if (length == 8 && memcmp(scanner->start, "function", 8) == 0) {
		return FUNK_TOKEN_FUNCTION;
	} else if (length == 6 && memcmp(scanner->start, "return", 6) == 0) {
		return FUNK_TOKEN_RETURN;
	}

	return FUNK_TOKEN_NAME;
}

static bool fits_for_name(char c) {
	return is_alpha(c) || is_digit(c) || c == '.';
}

FunkToken funk_scan_token(FunkScanner* scanner) {
	skip_whitespace(scanner);
	scanner->start = scanner->current;

	if (is_at_end(scanner)) {
		return make_token(scanner, FUNK_TOKEN_EOF);
	}

	char c = advance_char(scanner);

	if (is_alpha(c)) {
		while (fits_for_name(peek_char(scanner))) {
			advance_char(scanner);
		}

		return make_token(scanner, decide_token_type(scanner));
	}

	switch (c) {
		case '(': return make_token(scanner, FUNK_TOKEN_LEFT_PAREN);
		case ')': return make_token(scanner, FUNK_TOKEN_RIGHT_PAREN);
		case '{': return make_token(scanner, FUNK_TOKEN_LEFT_BRACE);
		case '}': return make_token(scanner, FUNK_TOKEN_RIGHT_BRACE);
		case ',': return make_token(scanner, FUNK_TOKEN_COMMA);
		case '=': {
			if (peek_char(scanner) == '>') {
				advance_char(scanner);
				return make_token(scanner, FUNK_TOKEN_ARROW);
			}
		}

		default: return make_token(scanner, FUNK_TOKEN_EOF);
	}
}

void funk_free_object(sFunkVm* vm, FunkObject* object) {
	switch (object->type) {
		case FUNK_OBJECT_BASIC_FUNCTION: {
			FunkBasicFunction* function = (FunkBasicFunction*) object;

			if (function->argumentNames != NULL) {
				vm->freeFn((void*) function->argumentNames);
			}

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
			FunkNativeFunction* function = (FunkNativeFunction*) object;

			if (function->cleanupFn != NULL) {
				function->cleanupFn(vm, function);
			}

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

	char* buffer = vm->allocFn(length + 1);
	memcpy((void*) buffer, chars, length);
	buffer[length] = '\0';

	string->object.type = FUNK_OBJECT_STRING;
	string->chars = (const char*) buffer;
	string->length = length;
	string->hash = hash;

	funk_table_set(vm, &vm->strings, string, (FunkObject*) string);

	return string;
}

const char* funk_to_string(FunkFunction* function) {
	return function == NULL ? "null" : function->name->chars;
}

FunkBasicFunction* funk_create_basic_function(sFunkVm* vm, FunkString* name) {
	FunkBasicFunction* function = (FunkBasicFunction*) allocate_object(vm, sizeof(FunkBasicFunction));

	function->parent.object.type = FUNK_OBJECT_BASIC_FUNCTION;
	function->parent.name = name;
	function->argumentCount = 0;
	function->argumentNames = NULL;

	function->code = NULL;
	function->codeLength = 0;
	function->codeAllocated = 0;

	function->constants = NULL;
	function->constantsAllocated = 0;
	function->constantsLength = 0;

	return function;
}

FunkBasicFunction* funk_create_empty_function(sFunkVm* vm, const char* name) {
	FunkString* nameString = funk_create_string(vm, name, strlen(name));
	return funk_create_basic_function(vm, nameString);
}

void funk_write_instruction(sFunkVm* vm, FunkBasicFunction* function, uint8_t instruction) {
	if (function->codeAllocated < function->codeLength + 1) {
		uint16_t newSize = FUNK_GROW_CAPACITY(function->codeAllocated);
		size_t totalSize = sizeof(uint8_t) * newSize;
		uint8_t* newChunk = (uint8_t*) vm->allocFn(totalSize);

		memcpy((void*) newChunk, (void*) function->code, sizeof(uint8_t) * function->codeAllocated);
		vm->freeFn((void*) function->code);

		function->code = newChunk;
		function->codeAllocated = newSize;
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
		uint16_t newSize = FUNK_GROW_CAPACITY(function->constantsAllocated);
		FunkObject** newConstants = (FunkObject**) vm->allocFn(sizeof(FunkObject*) * newSize);

		memcpy((void*) newConstants, (void*) function->constants, sizeof(FunkObject*) * function->constantsAllocated);
		vm->freeFn((void*) function->constants);

		function->constants = newConstants;
		function->constantsAllocated = newSize;
	}

	function->constants[function->constantsLength++] = (FunkObject*) constant;
	return function->constantsLength - 1;
}


FunkNativeFunction* funk_create_native_function(sFunkVm* vm, FunkString* name, FunkNativeFn fn) {
	FunkNativeFunction* function = (FunkNativeFunction*) allocate_object(vm, sizeof(FunkNativeFunction));

	function->parent.object.type = FUNK_OBJECT_NATIVE_FUNCTION;
	function->parent.name = name;
	function->fn = fn;
	function->data = NULL;
	function->cleanupFn = NULL;

	return function;
}

static void advance_token(FunkCompiler* compiler) {
	compiler->previous = compiler->current;
	compiler->current = funk_scan_token(compiler->scanner);
}

static void compilation_error(FunkCompiler* compiler, const char* error) {
	// TODO: print the line
	compiler->vm->errorFn(compiler->vm, error);
	compiler->hadError = true;
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

static void compile_expression(FunkCompiler* compiler);
static void compile_declaration(FunkCompiler* compiler);

static FunkFunction* compile_function(FunkCompiler* compiler, FunkString* name, bool lambda) {
	FunkBasicFunction* oldFunction = compiler->function;
	FunkVm* vm = compiler->vm;
	bool compiledBody = false;

	compiler->function = funk_create_basic_function(vm,  name);

	if (!lambda || compiler->current.type != FUNK_TOKEN_LEFT_BRACE) {
		consume_token(compiler, FUNK_TOKEN_LEFT_PAREN, "Expected '(' after function name");

		if (!match_token(compiler, FUNK_TOKEN_RIGHT_PAREN)) {
			FunkString* argumentNames[256];

			do {
				consume_token(compiler, FUNK_TOKEN_NAME, "Expected argument name");
				argumentNames[compiler->function->argumentCount++] = funk_create_string(vm, compiler->previous.start, compiler->previous.length);
			} while (match_token(compiler, FUNK_TOKEN_COMMA));

			size_t size = sizeof(FunkString*) * compiler->function->argumentCount;
			compiler->function->argumentNames = (FunkString**) vm->allocFn(size);

			memcpy((void*) compiler->function->argumentNames, argumentNames, size);

			consume_token(compiler, FUNK_TOKEN_RIGHT_PAREN, "Expected ')' after function arguments");
		}

		if (lambda) {
			consume_token(compiler, FUNK_TOKEN_ARROW, "Expected '=>' after function arguments");

			if (compiler->current.type != FUNK_TOKEN_LEFT_BRACE) {
				compile_expression(compiler);
				write_uint8_t(compiler, FUNK_INSTRUCTION_RETURN);

				compiledBody = true;
			}
		}
	}

	if (!compiledBody) {
		consume_token(compiler, FUNK_TOKEN_LEFT_BRACE, "Expected '{' after function arguments");

		while (!match_token(compiler, FUNK_TOKEN_RIGHT_BRACE)) {
			compile_declaration(compiler);
		}

		write_uint8_t(compiler, FUNK_INSTRUCTION_PUSH_NULL);
		write_uint8_t(compiler, FUNK_INSTRUCTION_RETURN);
	}

	FunkObject* newFunction = (FunkObject*) compiler->function;
	compiler->function = oldFunction;

	return (FunkFunction*) newFunction;
}

static void compile_expression(FunkCompiler* compiler) {
	if (match_token(compiler, FUNK_TOKEN_RETURN)) {
		compile_expression(compiler);
		write_uint8_t(compiler, FUNK_INSTRUCTION_RETURN);

		return;
	}

	if (compiler->current.type == FUNK_TOKEN_LEFT_PAREN || compiler->current.type == FUNK_TOKEN_LEFT_BRACE) {
		char buffer[256];
		sprintf(buffer, "lambda %s %u", compiler->function->parent.name->chars, compiler->previous.line);

		FunkString* name = funk_create_string(compiler->vm, buffer, strlen(buffer));
		FunkFunction* lambda = compile_function(compiler, name, true);

		write_uint8_t(compiler, FUNK_INSTRUCTION_PUSH_CONSTANT);
		write_uint16_t(compiler, funk_add_constant(compiler->vm, compiler->function, (FunkObject*) lambda));

		return;
	}

	consume_token(compiler, FUNK_TOKEN_NAME, "Function name expected");

	uint16_t name = add_string_constant_for_previous_token(compiler);
	bool isACall = match_token(compiler, FUNK_TOKEN_LEFT_PAREN);

	write_uint8_t(compiler, isACall ? FUNK_INSTRUCTION_GET : FUNK_INSTRUCTION_GET_STRING);
	write_uint16_t(compiler, name);

	while (isACall) {
		uint8_t argumentCount = 0;

		if (!match_token(compiler, FUNK_TOKEN_RIGHT_PAREN)) {
			do {
				compile_expression(compiler);
				argumentCount++;
			} while (match_token(compiler, FUNK_TOKEN_COMMA));

			consume_token(compiler, FUNK_TOKEN_RIGHT_PAREN, "')' expected after function arguments");
		}

		write_uint8_t(compiler, FUNK_INSTRUCTION_CALL);
		write_uint8_t(compiler, argumentCount);

		isACall = match_token(compiler, FUNK_TOKEN_LEFT_PAREN);
	}
}

static void compile_declaration(FunkCompiler* compiler) {
	if (match_token(compiler, FUNK_TOKEN_FUNCTION)) {
		consume_token(compiler, FUNK_TOKEN_NAME, "Expected function name");

		FunkVm* vm = compiler->vm;
		FunkString* name = funk_create_string(vm, compiler->previous.start, compiler->previous.length);

		FunkFunction* newFunction = compile_function(compiler, name, false);

		write_uint8_t(compiler, FUNK_INSTRUCTION_DEFINE);
		write_uint16_t(compiler, funk_add_constant(vm, compiler->function, newFunction));

		return;
	}

	compile_expression(compiler);
	write_uint8_t(compiler, FUNK_INSTRUCTION_POP);
}

FunkFunction* funk_compile_string(sFunkVm* vm, const char* name, const char* string) {
	FunkString* string_name = funk_create_string(vm, name, strlen(name));
	FunkBasicFunction* function = funk_create_basic_function(vm, string_name);

	FunkScanner scanner;
	funk_init_scanner(&scanner, string);

	FunkCompiler compiler;

	compiler.vm = vm;
	compiler.hadError = false;
	compiler.scanner = &scanner;
	compiler.function = function;

	advance_token(&compiler);

	while (compiler.current.type != FUNK_TOKEN_EOF) {
		compile_declaration(&compiler);
	}

	write_uint8_t(&compiler, FUNK_INSTRUCTION_RETURN);

	return compiler.hadError ? NULL : &function->parent;
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

bool funk_table_delete(FunkTable* table, FunkString* key) {
	if (table->count == 0) {
		return false;
	}

	FunkTableEntry* entry = find_entry(table->entries, table->capacity, key);

	if (entry->key == NULL) {
		return false;
	}

	entry->key = NULL;
	entry->value = NULL;

	return true;
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
	vm->callFrame = NULL;

	funk_init_table(&vm->strings);
	funk_init_table(&vm->globals);

	return vm;
}

void funk_free_vm(FunkVm* vm) {
	if (vm == NULL) {
		return;
	}

	funk_free_table(vm, &vm->strings);
	funk_free_table(vm, &vm->globals);

	FunkObject* object = vm->objects;

	while (object != NULL) {
		FunkObject* next = object->next;
		funk_free_object(vm, object);
		object = next;
	}

	vm->freeFn((void*) vm);
}

FunkFunction* funk_run_function(FunkVm* vm, FunkFunction* function, uint8_t argCount) {
	if (function == NULL) {
		return function;
	}

	if (function->object.type == FUNK_OBJECT_NATIVE_FUNCTION) {
		FunkNativeFunction* nativeFunction = (FunkNativeFunction*) function;
		return nativeFunction->fn(vm, nativeFunction, vm->stackTop + 1, argCount);
	}

	FunkBasicFunction* fn = (FunkBasicFunction*) function;

	if (fn->codeLength == 0) {
		return function;
	}

	register uint8_t* ip = fn->code;
	FunkObject** constants = fn->constants;
	FunkFunction** initialStackTop = vm->stackTop;

	FunkCallFrame callFrame;

	callFrame.function = fn;
	callFrame.previous = vm->callFrame;

	funk_init_table(&callFrame.variables);

	for (uint8_t i = 0; i < fn->argumentCount; i++) {
		funk_table_set(vm, &callFrame.variables, fn->argumentNames[i], (FunkObject*) *(vm->stackTop + 1 + i));
	}

	vm->callFrame = &callFrame;

	#ifdef FUNK_TRACE_STACK
		printf("\n=+ %s +=\n", function->name->chars);

		for (uint16_t i = 0; i < fn->constantsLength; i++) {
			FunkObject* object = fn->constants[i];
			printf("%i: %s\n", i, object->type == FUNK_OBJECT_STRING ? ((FunkString*) object)->chars : funk_to_string(object));
		}
	#endif

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
		#endif

		switch (*ip++) {
			case FUNK_INSTRUCTION_RETURN: {
				FunkFunction* value = POP();

				vm->callFrame = callFrame.previous;
				vm->stackTop = initialStackTop;

				funk_free_table(vm, &callFrame.variables);
				return value;
			}

			case FUNK_INSTRUCTION_CALL: {
				uint8_t argumentCount = READ_UINT8();

				FunkFunction** stackTop = vm->stackTop - argumentCount - 1;
				FunkFunction* callee = *stackTop;
				FunkFunction* result;

				#ifdef FUNK_TRACE_STACK
					printf(" %s %i", funk_to_string(callee), argumentCount);
				#endif

				if (callee == NULL) {
					vm->errorFn(vm, "Attempt to call a null value");
					vm->stackTop = initialStackTop;

					return NULL;
				}

				if (callee->object.type == FUNK_OBJECT_NATIVE_FUNCTION) {
					FunkNativeFunction* nativeFunction = (FunkNativeFunction*) callee;
					result = nativeFunction->fn(vm, nativeFunction, (vm->stackTop - argumentCount), argumentCount);
				} else {
					FunkBasicFunction* basicFunction = (FunkBasicFunction*) callee;
					vm->stackTop = stackTop;

					for (uint8_t i = argumentCount; i < basicFunction->argumentCount; i++) {
						vm->stackTop[i + 1] = NULL;
					}

					result = funk_run_function(vm, callee, argumentCount);
				}

				#ifdef FUNK_TRACE_STACK
					printf("\n== %s ==\n", function->name->chars);
				#endif

				vm->stackTop = stackTop;
				PUSH(result);

				break;
			}

			case FUNK_INSTRUCTION_GET: {
				FunkString* name = (FunkString*) READ_CONSTANT();
				FunkFunction* result = NULL;
				FunkCallFrame* currentFrame = &callFrame;

				bool hadResult = false;

				do {
					hadResult = funk_table_get(currentFrame == NULL ? &vm->globals : &currentFrame->variables, name, (FunkObject**) &result);

					if (currentFrame == NULL) {
						break;
					}

					currentFrame = currentFrame->previous;
				} while (!hadResult);

				PUSH(result);

				#ifdef FUNK_TRACE_STACK
					printf(" %s => %s", name->chars, funk_to_string(result));
				#endif

				break;
			}

			case FUNK_INSTRUCTION_GET_STRING: {
				FunkString* name = (FunkString*) READ_CONSTANT();
				FunkFunction* result = NULL;
				FunkCallFrame* currentFrame = &callFrame;

				bool hadResult = false;

				do {
					hadResult = funk_table_get(currentFrame == NULL ? &vm->globals : &currentFrame->variables, name, (FunkObject**) &result);

					if (currentFrame == NULL) {
						break;
					}

					currentFrame = currentFrame->previous;
				} while (!hadResult);

				if (!hadResult) {
					result = (FunkFunction*) funk_create_basic_function(vm, name);
				}

				#ifdef FUNK_TRACE_STACK
					printf(" %s => %s", name->chars, funk_to_string(result));
				#endif

				PUSH(result);
				break;
			}

			case FUNK_INSTRUCTION_POP: {
				POP();
				break;
			}

			case FUNK_INSTRUCTION_DEFINE: {
				FunkBasicFunction* basicFunction = (FunkBasicFunction*) READ_CONSTANT();
				funk_table_set(vm, &callFrame.variables, basicFunction->parent.name, (FunkObject*) basicFunction);

				#ifdef FUNK_TRACE_STACK
					printf(" %s", funk_to_string((FunkFunction*) basicFunction));
				#endif

				break;
			}

			case FUNK_INSTRUCTION_PUSH_NULL: {
				PUSH(NULL);
				break;
			}

			case FUNK_INSTRUCTION_PUSH_CONSTANT: {
				PUSH((FunkFunction*) READ_CONSTANT());
				break;
			}

			default: {
				vm->errorFn(vm, "Unknown instruction");
				vm->stackTop = initialStackTop;

				return NULL;
			}
		}

		#ifdef FUNK_TRACE_STACK
				printf("\n");
		#endif
	}

	#undef READ_UINT8
	#undef READ_UINT16
	#undef PUSH
	#undef POP
}

FunkFunction* funk_run_string(FunkVm* vm, const char* name, const char* string) {
	FunkFunction* function = funk_compile_string(vm, name, string);
	return funk_run_function(vm, function, 0);
}

FunkFunction* funk_run_function_arged(FunkVm* vm, FunkFunction* function, FunkFunction** args, uint8_t argCount) {
	for (uint8_t i = 0; i < argCount; i++) {
		vm->stackTop[i + 1] = args[i];
	}

	return funk_run_function(vm, function, argCount);
}

FunkFunction* funk_run_string_arged(FunkVm* vm, const char* name, const char* string, FunkFunction** args, uint8_t argCount) {
	FunkFunction* function = funk_compile_string(vm, name, string);
	return funk_run_function_arged(vm, function, args, argCount);
}

void funk_set_global(FunkVm* vm, const char* name, FunkFunction* function) {
	funk_table_set(vm, &vm->globals, funk_create_string(vm, name, strlen(name)), (FunkObject*) function);
}

FunkFunction* funk_get_global(FunkVm* vm, const char* name) {
	FunkFunction* result = NULL;
	funk_table_get(&vm->globals, funk_create_string(vm, name, strlen(name)), (FunkObject**) &result);

	return result;
}

void funk_define_native(FunkVm* vm, const char* name, FunkNativeFn fn) {
	FunkString* nameString = funk_create_string(vm, name, strlen(name));
	funk_table_set(vm, &vm->globals, nameString, (FunkObject*) funk_create_native_function(vm, nameString, fn));
}

void funk_set_variable(FunkVm* vm, const char* name, FunkFunction* function) {
	if (vm->callFrame == NULL) {
		funk_set_global(vm, name, function);
		return;
	}

	FunkCallFrame* currentFrame = vm->callFrame;
	FunkString* nameString = funk_create_string(vm, name, strlen(name));
	FunkFunction* result = NULL;

	do {
		FunkTable* table = currentFrame == NULL ? &vm->globals : &currentFrame->variables;

		if (funk_table_get(table, nameString, (FunkObject**) &result)) {
			funk_table_set(vm, table, nameString, (FunkObject*) function);
			return;
		}

		if (currentFrame == NULL) {
			funk_table_set(vm, &vm->callFrame->variables, nameString, (FunkObject*) function);
			break;
		}

		currentFrame = currentFrame->previous;
	} while (true);
}

FunkFunction* funk_get_variable(FunkVm* vm, const char* name) {
	if (vm->callFrame == NULL) {
		return funk_get_global(vm, name);
	}

	FunkCallFrame* currentFrame = vm->callFrame;
	FunkString* nameString = funk_create_string(vm, name, strlen(name));
	FunkFunction* result = NULL;

	bool hadResult = false;

	do {
		hadResult = funk_table_get(currentFrame == NULL ? &vm->globals : &currentFrame->variables, nameString, (FunkObject**) &result);

		if (currentFrame == NULL) {
			break;
		}

		currentFrame = currentFrame->previous;
	} while (!hadResult);

	return result;
}

void funk_error(FunkVm* vm, const char* error) {
	vm->errorFn(vm, error);
}

bool funk_function_has_code(FunkFunction* function) {
	return function->object.type == FUNK_OBJECT_NATIVE_FUNCTION ? true : ((FunkBasicFunction*) function)->codeLength > 0;
}

bool funk_is_true(FunkVm* vm, FunkFunction* function) {
	if (funk_function_has_code(function)) {
		function = funk_run_function(vm, function, 0);
	}

	return strcmp(function->name->chars, "true") == 0;
}

static uint32_t parse_roman_numeral(const char* string, uint16_t length) {
	if (memcmp(string, "NULLA", length) == 0) {
		return 0;
	}

	uint32_t value = 0;

	for (uint16_t i = 0; i < length; i++) {
		if (string[i] == 'I' && string[i + 1] == 'V') {
			value += 4;
		} else if (string[i] == 'I' && string[i + 1] == 'X') {
			value += 9;
		} else if (string[i] == 'X' && string[i + 1] == 'L') {
			value += 40;
		} else if (string[i] == 'X' && string[i + 1] == 'C') {
			value += 90;
		} else if (string[i] == 'C' && string[i + 1] == 'D') {
			value += 400;
		} else if (string[i] == 'C' && string[i + 1] == 'M') {
			value += 900;
		} else {
			switch (string[i]) {
				case 'I': value += 1; break;
				case 'V': value += 5; break;
				case 'X': value += 10; break;
				case 'L': value += 50; break;
				case 'C': value += 100; break;
				case 'D': value += 500; break;
				case 'M': value += 1000; break;

				default: break;
			}

			continue;
		}

		i++;
	}

	return value;
}

static uint16_t calculate_number_of_places(uint32_t n) {
	uint16_t r = 1;

	while (n > 9) {
		n /= 10;
		r++;
	}

	return r;
}

static uint16_t calculate_number_of_places_after_dot(double n) {
	uint16_t places = 0;

	while (true) {
		n *= 10;

		if ((int) fmod(n, 10) == 0 && (int) fmod(n * 10, 10) == 0) {
			return places;
		}

		places++;

		if (places >= 5) {
			return places;
		}
	}
}

double funk_to_number(FunkVm* vm, FunkFunction* function) {
	if (funk_function_has_code(function)) {
		function = funk_run_function(vm, function, 0);
	}

	const char* string = function->name->chars;
	uint16_t length = function->name->length;
	bool negative = false;

	if (string[0] == '-') {
		negative = true;
		string++;
		length--;
	}

	const char* dot = strchr(string, '.');
	double value = 0;

	if (dot != NULL) {
		length = dot - string;
		uint32_t afterDot = parse_roman_numeral(dot + 1, function->name->length - length - 1);

		value += (double) afterDot / (pow(10, calculate_number_of_places(afterDot)));
	}

	return (value + parse_roman_numeral(string, length)) * (negative ? -1 : 1);
}

static FunkString* roman_to_string(FunkVm* vm, uint32_t value) {
	uint32_t number = value;
	uint16_t index = 0;

	char buffer[255];

	while (number != 0) {
		if (number >= 1000) {
			buffer[index++] = 'M';
			number -= 1000;
		} else if (number >= 900) {
			memcpy((void*) (buffer + index), "CM", 2);

			index += 2;
			number -= 900;
		} else if (number >= 500) {
			buffer[index++] = 'D';
			number -= 500;
		} else if (number >= 400) {
			memcpy((void*) (buffer + index), "CD", 2);

			index += 2;
			number -= 400;
		} else if (number >= 100) {
			buffer[index++] = 'C';
			number -= 100;
		} else if (number >= 90) {memcpy((void*) (buffer + index), "XC", 2);

			index += 2;
			number -= 90;
		} else if (number >= 50) {
			buffer[index++] = 'L';
			number -= 50;
		} else if (number >= 40) {memcpy((void*) (buffer + index), "XL", 2);

			index += 2;
			number -= 40;
		} else if (number >= 10) {
			buffer[index++] = 'X';
			number -= 10;
		} else if (number >= 9) {memcpy((void*) (buffer + index), "IX", 2);

			index += 2;
			number -= 9;
		} else if (number >= 5) {
			buffer[index++] = 'V';
			number -= 5;
		} else if (number >= 4) {memcpy((void*) (buffer + index), "IV", 2);

			index += 2;
			number -= 4;
		} else {
			buffer[index++] = 'I';
			number -= 1;
		}
	}

	return funk_create_string(vm, buffer, index);
}

FunkFunction* funk_number_to_string(FunkVm* vm, double number) {
	bool negative = number < 0;
	number = fabs(number);

	uint32_t whole = floor(number);
	double fraction = number - whole;
	FunkString* fractionString = fraction > 0 ? roman_to_string(vm, (uint32_t) (fraction * pow(10, calculate_number_of_places_after_dot(fraction)))) : NULL;

	FunkString* digitsString = roman_to_string(vm, whole);
	uint16_t length = digitsString->length + (uint16_t) negative;

	if (fractionString != NULL) {
		length += fractionString->length + 1;
	}

	char buffer[length + 1];
	uint16_t index = 0;

	if (negative) {
		buffer[0] = '-';
		index++;
	}

	memcpy((void*) (buffer + index), digitsString->chars, digitsString->length);
	index += digitsString->length;

	if (fractionString != NULL) {
		buffer[index++] = '.';
		memcpy((void*) (buffer + index), fractionString->chars, fractionString->length);
	}

	buffer[length] = '\0';
	return (FunkFunction*) funk_create_empty_function(vm, buffer);
}