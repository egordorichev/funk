#include "funk_std.h"

#include <stdio.h>
#include <math.h>
#include <time.h>

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
		printf("%.6g\n", funk_to_number(vm, args[i]));
	}

	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(_clock) {
	clock_t time = clock();
	double inSeconds = (double) time / (double) CLOCKS_PER_SEC;

	FUNK_RETURN_NUMBER(inSeconds);
}

FUNK_NATIVE_FUNCTION_DEFINITION(readLine) {
	char* line = NULL;
	size_t length = 0;

	length = getline(&line, &length, stdin);
	FunkString* string = funk_create_string(vm, line, length);

	free(line);
	return (FunkFunction *) funk_create_basic_function(vm, string);
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
	FUNK_RETURN_BOOL(!funk_is_true(vm, args[0]));
}

FUNK_NATIVE_FUNCTION_DEFINITION(_if) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);

	if (funk_is_true(vm, args[0])) {
		return funk_run_function(vm, args[1], 0);
	} else if (argCount > 2) {
		return funk_run_function(vm, args[2], 0);
	}

	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(_while) {
	FUNK_ENSURE_ARG_COUNT(2);

	while (funk_is_true(vm, args[0])) {
		funk_run_function(vm, args[1], 0);
	}

	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(_for) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);

	if (argCount == 2) {
		FunkString* string = args[0]->name;
		char buffer[2] = " \0";

		for (uint16_t i = 0; i < string->length; i++) {
			buffer[0] = string->chars[i];

			FunkFunction* function = (FunkFunction*) funk_create_empty_function(vm, buffer);
			funk_run_function_arged(vm, args[1], &function, 1);
		}

		return NULL;
	}

	FunkFunction* to = args[1];
	double from = funk_to_number(vm, args[0]);
	bool increment = from < funk_to_number(vm, to);

	for (double i = funk_to_number(vm, args[0]);
			increment == (i < funk_to_number(vm, to));
			i += (increment ? 1 : -1)) {

		FunkFunction* indexFunction = funk_number_to_string(vm, i);
		funk_run_function_arged(vm, args[2], &indexFunction, 1);
	}

	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(space) {
	// TODO: optional argument for how many spaces
	FUNK_RETURN_STRING(" ");
}

FUNK_NATIVE_FUNCTION_DEFINITION(substring) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);

	FunkFunction* client = args[0];

	uint16_t from = (uint16_t) fmax(0, funk_to_number(vm, args[1]));
	uint16_t length = argCount > 2 ? (uint16_t) fmin(client->name->length - 1 - from, funk_to_number(vm, args[2])) : 1;

	if (length < 1) {
		FUNK_RETURN_STRING("");
	}

	char string[length + 1];

	memcpy((void*) string, (void*) (client->name->chars + from), length);
	string[length] = '\0';

	FUNK_RETURN_STRING(string);
}

FUNK_NATIVE_FUNCTION_DEFINITION(_char) {
	FUNK_ENSURE_ARG_COUNT(1);

	uint8_t byte = (uint8_t) funk_to_number(vm, args[0]);
	char buffer[2] = { (char) byte, '\0'};

	FUNK_RETURN_STRING(buffer);
}

FUNK_NATIVE_FUNCTION_DEFINITION(add) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);
	FUNK_RETURN_NUMBER(funk_to_number(vm, args[0]) + funk_to_number(vm, args[1]));
}

FUNK_NATIVE_FUNCTION_DEFINITION(subtract) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);
	FUNK_RETURN_NUMBER(funk_to_number(vm, args[0]) - funk_to_number(vm, args[1]));
}

FUNK_NATIVE_FUNCTION_DEFINITION(multiply) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);
	FUNK_RETURN_NUMBER(funk_to_number(vm, args[0]) * funk_to_number(vm, args[1]));
}

FUNK_NATIVE_FUNCTION_DEFINITION(divide) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);
	FUNK_RETURN_NUMBER(funk_to_number(vm, args[0]) / funk_to_number(vm, args[1]));
}

FUNK_NATIVE_FUNCTION_DEFINITION(greater) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);
	FUNK_RETURN_BOOL(funk_to_number(vm, args[0]) > funk_to_number(vm, args[1]));
}

FUNK_NATIVE_FUNCTION_DEFINITION(greaterEqual) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);
	FUNK_RETURN_BOOL(funk_to_number(vm, args[0]) >= funk_to_number(vm, args[1]));
}

FUNK_NATIVE_FUNCTION_DEFINITION(less) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);
	FUNK_RETURN_BOOL(funk_to_number(vm, args[0]) < funk_to_number(vm, args[1]));
}

FUNK_NATIVE_FUNCTION_DEFINITION(lessEqual) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);
	FUNK_RETURN_BOOL(funk_to_number(vm, args[0]) <= funk_to_number(vm, args[1]));
}

typedef struct FunkArrayData {
	FunkFunction** data;
	uint16_t length;
	uint16_t allocated;
} FunkArrayData;

static FunkArrayData* extract_array_data(FunkVm* vm, FunkFunction* function) {
	if (function->object.type != FUNK_OBJECT_NATIVE_FUNCTION) {
		funk_error(vm, "Expected an array as argument");
		return NULL;
	}

	return (FunkArrayData*) ((FunkNativeFunction*) function)->data;
}

static void cleanup_array_data(FunkVm* vm, FunkNativeFunction* function) {
	if (function->data != NULL) {
		vm->freeFn(function->data);
		function->data = NULL;
	}
}

FUNK_NATIVE_FUNCTION_DEFINITION(arrayCallback) {
	FunkArrayData* data = extract_array_data(vm, (FunkFunction *) self);

	if (argCount == 1) {
		if (funk_function_has_code(args[0])) {
			for (uint16_t i = 0; i < data->length; i++) {
				funk_run_function_arged(vm, args[0], &data->data[i], 1);
			}

			return NULL;
		}

		int32_t index = (int32_t) funk_to_number(vm, args[0]);

		if (index < 0 || index >= data->length) {
			return NULL;
		}

		return data->data[index];
	}

	FUNK_ENSURE_ARG_COUNT(2);
	int32_t index = (int32_t) funk_to_number(vm, args[0]);

	if (index < 0 || index >= data->length) {
		return NULL;
	}

	data->data[index] = args[1];
	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(array) {
	FunkNativeFunction* function = funk_create_native_function(vm, funk_create_string(vm, "$arrayData", 10), arrayCallback);
	FunkArrayData* data = (FunkArrayData*) vm->allocFn(sizeof(FunkArrayData));

	data->length = argCount;
	data->allocated = argCount;

	if (argCount > 0) {
		size_t size = sizeof(FunkFunction*) * argCount;
		data->data = (FunkFunction**) vm->allocFn(size);

		memcpy((void*) data->data, args, size);
	}

	function->cleanupFn = cleanup_array_data;
	function->data = (void*) data;

	return (FunkFunction *) function;
}

static inline bool is_array(FunkFunction* argument) {
	return argument->object.type == FUNK_OBJECT_NATIVE_FUNCTION && ((FunkNativeFunction*) argument)->cleanupFn == cleanup_array_data;
}

FUNK_NATIVE_FUNCTION_DEFINITION(length) {
	FUNK_ENSURE_ARG_COUNT(1);
	FunkFunction* argument = args[0];

	if (is_array(argument)) {
		FunkArrayData* data = extract_array_data(vm, argument);
		FUNK_RETURN_NUMBER(data->length);
	}

	FUNK_RETURN_NUMBER(argument->name->length);
}

FUNK_NATIVE_FUNCTION_DEFINITION(join) {
	uint16_t length = 0;

	if (argCount == 1 && is_array(args[0])) {
		FunkArrayData* data = extract_array_data(vm, args[0]);
		args = data->data;
		argCount = data->length;
	}

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

void funk_open_std(FunkVm* vm) {
	FUNK_DEFINE_FUNCTION("NULLA", nulla);

	FUNK_DEFINE_FUNCTION("print", print);
	FUNK_DEFINE_FUNCTION("printNumber", printNumber);
	FUNK_DEFINE_FUNCTION("clock", _clock);
	FUNK_DEFINE_FUNCTION("readLine", readLine);

	FUNK_DEFINE_FUNCTION("set", set);
	FUNK_DEFINE_FUNCTION("equal", equal);
	FUNK_DEFINE_FUNCTION("notEqual", notEqual);
	FUNK_DEFINE_FUNCTION("not", not);

	FUNK_DEFINE_FUNCTION("if", _if);
	FUNK_DEFINE_FUNCTION("while", _while);
	FUNK_DEFINE_FUNCTION("for", _for);

	FUNK_DEFINE_FUNCTION("space", space);
	FUNK_DEFINE_FUNCTION("join", join);
	FUNK_DEFINE_FUNCTION("substring", substring);
	FUNK_DEFINE_FUNCTION("char", _char);
	FUNK_DEFINE_FUNCTION("length", length);

	FUNK_DEFINE_FUNCTION("add", add);
	FUNK_DEFINE_FUNCTION("subtract", subtract);
	FUNK_DEFINE_FUNCTION("multiply", multiply);
	FUNK_DEFINE_FUNCTION("divide", divide);
	FUNK_DEFINE_FUNCTION("greater", greater);
	FUNK_DEFINE_FUNCTION("greaterEqual", greaterEqual);
	FUNK_DEFINE_FUNCTION("less", less);
	FUNK_DEFINE_FUNCTION("lessEqual", lessEqual);

	FUNK_DEFINE_FUNCTION("array", array);
}