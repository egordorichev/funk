#ifndef FUNK_FUNK_H
#define FUNK_FUNK_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

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

typedef struct sFunkVm sFunkVm;

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