#include "funk_std.h"

#include <stdio.h>
#include <math.h>

FUNK_NATIVE_FUNCTION_DEFINITION(nulla) {
	FUNK_RETURN_STRING("NULLA");
}

FUNK_NATIVE_FUNCTION_DEFINITION(print) {
	for (uint8_t i = 0; i < argCount; i++) {
		printf("%s\n", funk_to_string(args[i]));
	}

	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(printNumber) {
	for (uint8_t i = 0; i < argCount; i++) {
		printf("%.6g\n", funk_to_number(args[i]));
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

FUNK_NATIVE_FUNCTION_DEFINITION(space) {
	// TODO: optional argument for how many spaces
	FUNK_RETURN_STRING(" ");
}

FUNK_NATIVE_FUNCTION_DEFINITION(join) {
	uint16_t length = 0;

	for (uint8_t i = 0; i < argCount; i++) {
		length += args[i]->name->length;
	}

	char string[length + 1];
	uint16_t index = 0;

	for (uint8_t i = 0; i < argCount; i++) {
		FunkFunction* arg = args[i];

		memcpy((void*) (string + index), arg->name->chars, arg->name->length);
		index += arg->name->length;
	}

	string[length] = '\0';
	FUNK_RETURN_STRING(string);
}

FUNK_NATIVE_FUNCTION_DEFINITION(substring) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);

	FunkFunction* client = args[0];

	uint16_t from = (uint16_t) fmax(0, funk_to_number(args[1]));
	uint16_t length = argCount > 2 ? (uint16_t) fmin(client->name->length - 1 - from, funk_to_number(args[2])) : 1;

	if (length < 1) {
		FUNK_RETURN_STRING("");
	}

	char string[length + 1];

	memcpy((void*) string, (void*) (client->name->chars + from), length);
	string[length] = '\0';

	FUNK_RETURN_STRING(string);
}

void funk_open_std(FunkVm* vm) {
	FUNK_DEFINE_FUNCTION("NULLA", nulla);

	FUNK_DEFINE_FUNCTION("print", print);
	FUNK_DEFINE_FUNCTION("printNumber", printNumber);
	FUNK_DEFINE_FUNCTION("set", set);
	FUNK_DEFINE_FUNCTION("equal", equal);
	FUNK_DEFINE_FUNCTION("notEqual", notEqual);
	FUNK_DEFINE_FUNCTION("not", not);

	FUNK_DEFINE_FUNCTION("if", _if);
	FUNK_DEFINE_FUNCTION("while", _while);

	FUNK_DEFINE_FUNCTION("space", space);
	FUNK_DEFINE_FUNCTION("join", join);
	FUNK_DEFINE_FUNCTION("substring", substring);
}