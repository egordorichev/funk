#ifndef FUNK_FUNK_H
#define FUNK_FUNK_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define UNREACHABLE assert(false);
#define FUNK_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

typedef struct sFunkVm sFunkVm;

typedef enum {
	FUNK_TOKEN_NAME,
	FUNK_TOKEN_LEFT_PAREN, FUNK_TOKEN_RIGHT_PAREN,
	FUNK_TOKEN_LEFT_BRACE, FUNK_TOKEN_RIGHT_BRACE,
	FUNK_TOKEN_COMMA,

	FUNK_TOKEN_EOF
} FunkTokenType;

typedef struct {
	FunkTokenType type;

	uint16_t line;
	uint16_t length;

	const char* start;
} FunkToken;

typedef struct {
	const char* start;
	const char* current;

	uint16_t line;
} FunkScanner;

void funk_init_scanner(FunkScanner* scanner, const char* code);
FunkToken funk_scan_token(FunkScanner* scanner);

typedef enum {
	FUNK_OBJECT_BASIC_FUNCTION,
	FUNK_OBJECT_NATIVE_FUNCTION,
	FUNK_OBJECT_STRING
} FunkObjectType;

typedef struct FunkObject {
	FunkObjectType type;
	struct FunkObject* next;
} FunkObject;

void funk_free_object(sFunkVm* vm, FunkObject* object);

typedef struct FunkString {
	FunkObject object;

	const char* chars;
	uint16_t length;
	uint32_t hash;
} FunkString;

FunkString* funk_create_string(sFunkVm* vm, const char* chars, uint16_t length);

typedef struct FunkFunction {
	FunkObject object;
	FunkString* name;
} FunkFunction;

const char* funk_to_string(sFunkVm* vm, FunkFunction* function);

typedef enum {
	FUNK_INSTRUCTION_RETURN,
	FUNK_INSTRUCTION_CALL,
	FUNK_INSTRUCTION_GET,
	FUNK_INSTRUCTION_GET_STRING,
	FUNK_INSTRUCTION_POP,
	FUNK_INSTRUCTION_DEFINE,
	FUNK_INSTRUCTION_PUSH_NULL
} FunkInstruction;

// #define FUNK_TRACE_STACK

#ifdef FUNK_TRACE_STACK
static const char* funkInstructionNames[] = {
	"RETURN",
	"CALL",
	"GET",
	"GET_STRING",
	"POP",
	"DEFINE",
	"PUSH_NULL"
};
#endif

typedef struct FunkBasicFunction {
	FunkFunction parent;
	uint8_t argCount;

	uint8_t* code;
	uint16_t codeAllocated;
	uint16_t codeLength;

	FunkObject** constants;
	uint16_t constantsAllocated;
	uint16_t constantsLength;
} FunkBasicFunction;

FunkBasicFunction* funk_create_basic_function(sFunkVm* vm, FunkString* name);
void funk_write_instruction(sFunkVm* vm, FunkBasicFunction* function, uint8_t instruction);
uint16_t funk_add_constant(sFunkVm* vm, FunkBasicFunction* function, FunkObject* constant);

typedef FunkFunction* (*FunkNativeFn)(sFunkVm*, FunkFunction**, uint8_t);

typedef struct FunkNativeFunction {
	FunkFunction parent;
	FunkNativeFn fn;
} FunkNativeFunction;

FunkNativeFunction* funk_create_native_function(sFunkVm* vm, FunkString* name, FunkNativeFn fn);

typedef struct FunkCompiler {
	FunkScanner* scanner;
	sFunkVm* vm;

	FunkToken previous;
	FunkToken current;
	FunkBasicFunction* function;
} FunkCompiler;

FunkFunction* funk_compile_string(sFunkVm* vm, const char* name, const char* string);

#define TABLE_MAX_LOAD 0.75

typedef struct {
	FunkString* key;
	FunkObject* value;
} FunkTableEntry;

typedef struct FunkTable {
	int count;
	int capacity;

	FunkTableEntry* entries;
} FunkTable;

void funk_init_table(FunkTable* table);
void funk_free_table(sFunkVm* vm, FunkTable* table);
bool funk_table_set(sFunkVm* vm, FunkTable* table, FunkString* key, FunkObject* value);
bool funk_table_get(FunkTable* table, FunkString* key, FunkObject** value);
FunkString* funk_table_find_string(FunkTable* table, const char* chars, uint16_t length, uint32_t hash);

typedef void* (*FunkAllocFn)(size_t);
typedef void (*FunkFreeFn)(void*);
typedef void (*FunkErrorFn)(sFunkVm*, const char*);

#define FUNK_STACK_SIZE 256

typedef struct sFunkVm {
	FunkAllocFn allocFn;
	FunkFreeFn freeFn;
	FunkErrorFn errorFn;

	FunkTable globals;
	FunkTable strings;
	FunkObject* objects;

	FunkFunction* stack[FUNK_STACK_SIZE];
	FunkFunction** stackTop;
} FunkVm;

FunkVm* funk_create_vm(FunkAllocFn allocFn, FunkFreeFn freeFn, FunkErrorFn errorFn);
void funk_free_vm(FunkVm* vm);
FunkFunction* funk_run_function(FunkVm* vm, FunkFunction* function);
FunkFunction* funk_run_string(FunkVm* vm, const char* name, const char* string);

#define FUNK_NATIVE_FUNCTION_DEFINITION(name) FunkFunction* name(FunkVm* vm, FunkFunction** args, uint8_t argCount)
#define FUNK_DEFINE_FUNCTION(string_name, name) funk_define_native(vm, string_name, name)

void funk_define_native(FunkVm* vm, const char* name, FunkNativeFn fn);

#endif