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
	FUNK_RETURN_BOOL(!funk_is_true(args[0]));
}

FUNK_NATIVE_FUNCTION_DEFINITION(_if) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);

	if (funk_is_true(args[0])) {
		return funk_run_function(vm, args[1]);
	} else if (argCount > 2) {
		return funk_run_function(vm, args[2]);
	}

	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(_while) {
	FUNK_ENSURE_ARG_COUNT(2);

	while (funk_is_true(args[0])) {
		funk_run_function(vm, args[1]);
	}

	return NULL;
}

void funk_open_std(FunkVm* vm) {
	FUNK_DEFINE_FUNCTION("print", print);
	FUNK_DEFINE_FUNCTION("set", set);
	FUNK_DEFINE_FUNCTION("equal", equal);
	FUNK_DEFINE_FUNCTION("notEqual", notEqual);
	FUNK_DEFINE_FUNCTION("not", not);

	FUNK_DEFINE_FUNCTION("if", _if);
	FUNK_DEFINE_FUNCTION("while", _while);
}