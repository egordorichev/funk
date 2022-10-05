#include "funk_std.h"
#include <stdio.h>

FUNK_NATIVE_FUNCTION_DEFINITION(print) {
	for (uint8_t i = 0; i < argCount; i++) {
		printf("%s\n", funk_to_string(vm, args[i]));
	}

	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(set) {
	FUNK_ENSURE_ARG_COUNT(2);
	funk_set_variable(vm, args[0]->name->chars, args[1]);

	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(equal) {
	FUNK_ENSURE_ARG_COUNT(2);
	FUNK_RETURN_BOOL(args[0]->name == args[1]->name);
}

FUNK_NATIVE_FUNCTION_DEFINITION(notEqual) {
	FUNK_ENSURE_ARG_COUNT(2);
	FUNK_RETURN_BOOL(args[0]->name != args[1]->name);
}

FUNK_NATIVE_FUNCTION_DEFINITION(not) {
	FUNK_ENSURE_ARG_COUNT(1);
	FUNK_RETURN_BOOL(strcmp(args[0]->name->chars, "true") != 0);
}

void funk_open_std(FunkVm* vm) {
	FUNK_DEFINE_FUNCTION("print", print);
	FUNK_DEFINE_FUNCTION("set", set);
	FUNK_DEFINE_FUNCTION("equal", equal);
	FUNK_DEFINE_FUNCTION("notEqual", notEqual);
	FUNK_DEFINE_FUNCTION("not", not);
}