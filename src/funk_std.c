#include "funk_std.h"
#include <stdio.h>

FUNK_NATIVE_FUNCTION_DEFINITION(print) {
	for (uint8_t i = 0; i < argCount; i++) {
		printf("%s\n", funk_to_string(vm, args[i]));
	}

	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(set) {
	if (argCount != 2) {
		funk_error(vm, "Expected 2 arguments");
		return NULL;
	}

	funk_set_variable(vm, args[0]->name->chars, args[1]);
	return NULL;
}

void funk_open_std(FunkVm* vm) {
	FUNK_DEFINE_FUNCTION("print", print);
	FUNK_DEFINE_FUNCTION("set", set);
}