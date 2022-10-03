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
} FunkObject;

void funk_free_object(sFunkVm* vm, FunkObject* object);

typedef struct FunkString {
	FunkObject object;

	const char* chars;
	uint16_t length;
} FunkString;

FunkString* funk_create_string(sFunkVm* vm, const char* chars, uint16_t length);

typedef struct FunkFunction {
	FunkObject object;
	FunkString* name;
} FunkFunction;

typedef enum {
	FUNK_INSTRUCTION_RETURN,
	FUNK_INSTRUCTION_PUSH,
	FUNK_INSTRUCTION_POP,
	FUNK_INSTRUCTION_CALL
} FunkInstruction;

typedef struct FunkBasicFunction {
	FunkFunction parent;

	uint8_t* code;
	uint16_t code_allocated;
	uint16_t code_length;
} FunkBasicFunction;

FunkBasicFunction* funk_create_basic_function(sFunkVm* vm, FunkString* name);
void funk_write_instruction(sFunkVm* vm, FunkBasicFunction* function, FunkInstruction instruction);

typedef void (*Funk_NativeFn)(sFunkVm*, FunkFunction* args, uint8_t arg_count);

typedef struct FunkNativeFunction {
	FunkFunction parent;
	Funk_NativeFn fn;
} FunkNativeFunction;

FunkNativeFunction* funk_create_native_function(sFunkVm* vm, FunkString* name, Funk_NativeFn fn);

typedef void* (*Funk_AllocFn)(size_t);
typedef void (*Funk_FreeFn)(void*);
typedef void (*Funk_ErrorFn)(sFunkVm*, const char*);

typedef struct sFunkVm {
	Funk_AllocFn allocFn;
	Funk_FreeFn freeFn;
	Funk_ErrorFn errorFn;
} FunkVm;

FunkVm* funk_create_vm(Funk_AllocFn allocFn, Funk_FreeFn freeFn, Funk_ErrorFn errorFn);
void funk_free_vm(FunkVm* vm);
bool funk_run_string(FunkVm* vm, const char* string);

#endif