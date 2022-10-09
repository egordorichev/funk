#include "funk_std.h"

#include <stdio.h>
#include <math.h>
#include <time.h>

static inline bool is_array(FunkFunction* argument);
static inline bool is_map(FunkFunction* argument);
static inline bool is_file(FunkFunction* argument);

typedef struct FunkArrayData {
	FunkFunction** data;
	uint16_t length;
	uint16_t allocated;
} FunkArrayData;

static FunkArrayData* extract_array_data(FunkVm* vm, FunkFunction* function) {
	if (!is_array(function)) {
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
	FunkNativeFunction* function = funk_create_native_function(vm, funk_create_string(vm, "$arrayData", 10),(FunkNativeFn) arrayCallback);
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
	return argument != NULL && argument->object.type == FUNK_OBJECT_NATIVE_FUNCTION && ((FunkNativeFunction*) argument)->cleanupFn == cleanup_array_data;
}

FUNK_NATIVE_FUNCTION_DEFINITION(push) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);

	if (!is_array(args[0])) {
		funk_error(vm, "Expected an array as the first argument");
		return NULL;
	}

	FunkArrayData* data = extract_array_data(vm, args[0]);
	uint16_t newLength = data->length + argCount - 1;

	if (newLength > data->allocated) {
		uint16_t newSize = FUNK_GROW_CAPACITY(data->allocated);
		size_t totalSize = sizeof(FunkFunction*) * newSize;
		FunkFunction** newData = (FunkFunction**) vm->allocFn(totalSize);

		memcpy((void*) newData, (void*) data->data, sizeof(FunkFunction) * data->allocated);
		vm->freeFn((void*) data->data);

		data->data = newData;
		data->allocated = newSize;
	}

	memcpy((void*) (data->data + data->length), args + 1, sizeof(FunkFunction*) * (argCount - 1));
	data->length = newLength;

	return NULL;
}

typedef struct FunkMapData {
	FunkTable table;
} FunkMapData;

static FunkMapData* extract_map_data(FunkVm* vm, FunkFunction* function) {
	if (!is_map(function)) {
		funk_error(vm, "Expected a map as argument");
		return NULL;
	}

	return (FunkMapData*) ((FunkNativeFunction*) function)->data;
}

static void cleanup_map_data(FunkVm* vm, FunkNativeFunction* function) {
	if (function->data != NULL) {
		FunkMapData* data = extract_map_data(vm, (FunkFunction *) function);

		funk_free_table(vm, &data->table);
		vm->freeFn(function->data);

		function->data = NULL;
	}
}

FUNK_NATIVE_FUNCTION_DEFINITION(mapCallback) {
	FunkMapData* data = extract_map_data(vm, (FunkFunction *) self);

	if (argCount > 0 && args[0] == NULL) {
		return NULL;
	}

	if (argCount == 1) {
		if (funk_function_has_code(args[0])) {
			for (int32_t i = 0; i <= data->table.capacity; i++) {
				FunkTableEntry* entry = &data->table.entries[i];
				if (entry->key != NULL) {
					FunkFunction* buffer[2] = {
						(FunkFunction*) funk_create_basic_function(vm, entry->key), // Bye-bye efficiency!
						(FunkFunction *) entry->value
					};

					funk_run_function_arged(vm, args[0], (FunkFunction **) &buffer, 2);
				}
			}

			return NULL;
		}

		FunkFunction* result = NULL;
		funk_table_get(&data->table, args[0]->name, (FunkObject **) &result);

		return result;
	}

	FUNK_ENSURE_ARG_COUNT(2);

	funk_table_set(vm, &data->table, args[0]->name, (FunkObject *) args[1]);
	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(map) {
	FunkNativeFunction* function = funk_create_native_function(vm, funk_create_string(vm, "$mapData", 8),(FunkNativeFn) mapCallback);
	FunkMapData* data = (FunkMapData*) vm->allocFn(sizeof(FunkMapData));

	funk_init_table(&data->table);

	if (argCount > 0) {
		for (uint8_t i = 0; i < argCount; i += 2) {
			if (i + 1 >= argCount) {
				break;
			}

			if (args[i] == NULL) {
				continue;
			}

			funk_table_set(vm, &data->table, args[i]->name, (FunkObject *) args[i + 1]);
		}
	}

	function->cleanupFn = cleanup_map_data;
	function->data = (void*) data;

	return (FunkFunction *) function;
}

static inline bool is_map(FunkFunction* argument) {
	return argument != NULL && argument->object.type == FUNK_OBJECT_NATIVE_FUNCTION && ((FunkNativeFunction*) argument)->cleanupFn == cleanup_map_data;
}

FUNK_NATIVE_FUNCTION_DEFINITION(_remove) {
	FUNK_ENSURE_ARG_COUNT(2);

	if (args[1] == NULL) {
		return NULL;
	}

	FunkFunction* argument = args[0];

	if (is_map(argument)) {
		FunkMapData* data = extract_map_data(vm, argument);
		funk_table_delete(&data->table, args[1]->name);

		return NULL;
	} else if (!is_array(argument)) {
		funk_error(vm, "Expected an array as the first argument");
		return NULL;
	}

	FunkArrayData* data = extract_array_data(vm, argument);
	int32_t index = (int32_t) funk_to_number(vm, args[1]);

	if (index < 0 || index >= data->length) {
		return NULL;
	}

	if (index == data->length - 1) {
		data->length--;
		return NULL;
	}

	memcpy((void*) (data->data + index), data->data + index + 1, sizeof(FunkFunction*) * (data->length - index - 1));
	data->length--;

	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(nulla) {
	FUNK_RETURN_STRING("NULLA");
}

FUNK_NATIVE_FUNCTION_DEFINITION(variable) {
	FUNK_ENSURE_ARG_COUNT(1);

	if (args[0] == NULL) {
		return NULL;
	}

	uint16_t length = args[0]->name->length + 1;
	char string[length + 1];

	string[0] = '$';
	memcpy((void*) (string + 1), args[0]->name->chars, args[0]->name->length);
	string[length] = '\0';

	FUNK_RETURN_STRING(string);
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

FUNK_NATIVE_FUNCTION_DEFINITION(set) {
	FUNK_ENSURE_ARG_COUNT(2);

	if (args[0] == NULL) {
		return NULL;
	}

	funk_set_variable(vm, args[0]->name->chars, args[1]);
	return NULL;
}

FUNK_NATIVE_FUNCTION_DEFINITION(get) {
	FUNK_ENSURE_ARG_COUNT(1);

	if (args[0] == NULL) {
		return NULL;
	}

	return funk_get_variable(vm, args[0]->name->chars);
}

FUNK_NATIVE_FUNCTION_DEFINITION(equal) {
	FUNK_ENSURE_ARG_COUNT(2);

	if (args[0] == NULL || args[1] == NULL) {
		FUNK_RETURN_BOOL(args[0] == args[1]);
	}

	FUNK_RETURN_BOOL(args[0]->name == args[1]->name);
}

FUNK_NATIVE_FUNCTION_DEFINITION(notEqual) {
	FUNK_ENSURE_ARG_COUNT(2);

	if (args[0] == NULL || args[1] == NULL) {
		FUNK_RETURN_BOOL(args[0] != args[1]);
	}

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
		FunkFunction* argument = args[0];

		if (argument == NULL) {
			return NULL;
		}

		if (is_array(argument)) {
			FunkArrayData* data = extract_array_data(vm, argument);

			for (uint16_t i = 0; i < data->length; i++) {
				funk_run_function_arged(vm, args[1], &data->data[i], 1);
			}

			return NULL;
		} else if (is_map(argument)) {
			FunkMapData* data = extract_map_data(vm, argument);

			for (int32_t i = 0; i <= data->table.capacity; i++) {
				FunkTableEntry* entry = &data->table.entries[i];

				if (entry->key != NULL) {
					FunkFunction* buffer[2] = {
						(FunkFunction*) funk_create_basic_function(vm, entry->key), // Bye-bye efficiency!
						(FunkFunction *) entry->value
					};

					funk_run_function_arged(vm, args[1], (FunkFunction **) &buffer, 2);
				}
			}

			return NULL;
		}

		FunkString* string = argument->name;
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
	FUNK_RETURN_STRING(" ");
}

FUNK_NATIVE_FUNCTION_DEFINITION(separator) {
	FUNK_RETURN_STRING("/");
}

FUNK_NATIVE_FUNCTION_DEFINITION(dot) {
	FUNK_RETURN_STRING(".");
}

FUNK_NATIVE_FUNCTION_DEFINITION(substring) {
	FUNK_ENSURE_MIN_ARG_COUNT(2);

	FunkFunction* client = args[0];

	if (client == NULL) {
		return NULL;
	}

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

FUNK_NATIVE_FUNCTION_DEFINITION(length) {
	FUNK_ENSURE_ARG_COUNT(1);
	FunkFunction* argument = args[0];

	if (argument == NULL) {
		FUNK_RETURN_NUMBER(0);
	}

	if (is_array(argument)) {
		FunkArrayData* data = extract_array_data(vm, argument);
		FUNK_RETURN_NUMBER(data->length);
	} else if (is_map(argument)) {
		FunkMapData* data = extract_map_data(vm, argument);
		FUNK_RETURN_NUMBER(data->table.count);
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
		length += args[i] == NULL ? 4 : args[i]->name->length;
	}

	char string[length + 1];
	uint16_t index = 0;

	for (uint8_t i = 0; i < argCount; i++) {
		FunkFunction* arg = args[i];
		uint16_t nameLength = arg == NULL ? 4 : arg->name->length;

		memcpy((void*) (string + index), arg == NULL ? "null" : arg->name->chars, nameLength);
		index += nameLength;
	}

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

FUNK_NATIVE_FUNCTION_DEFINITION(require) {
	FUNK_ENSURE_ARG_COUNT(1);
	FunkString* path = args[0]->name;

	if (path == NULL) {
		return NULL;
	}

	FunkFunction* existingResult;

	if (funk_table_get(&vm->modules, path, (FunkObject **) &existingResult)) {
		return existingResult;
	}

	uint16_t length = path->length + 6;
	char buffer[length];

	memcpy((void*) buffer, path->chars, path->length);

	for (uint16_t i = 0; i < path->length; i++) {
		if (buffer[i] == '.') {
			buffer[i] = '/';
		}
	}

	memcpy((void*) (buffer + path->length), ".funk\0", 6);
	FunkFunction* result = funk_run_file(vm, buffer);

	funk_table_set(vm, &vm->modules, path, (FunkObject *) result);
	return result;
}

typedef struct FunkFileData {
	FILE* file;
	char* path;
} FunkFileData;

static FunkFileData* extract_file_data(FunkVm* vm, FunkFunction* function) {
	if (!is_file(function)) {
		funk_error(vm, "Expected a file as argument");
		return NULL;
	}

	return (FunkFileData*) ((FunkNativeFunction*) function)->data;
}

static void cleanup_file_data(FunkVm* vm, FunkNativeFunction* function) {
	if (function->data != NULL) {
		FunkFileData* fileData = (FunkFileData*) function->data;

		if (fileData->file != NULL) {
			fclose(fileData->file);
		}

		if (fileData->path != NULL) {
			vm->freeFn(fileData);
		}

		function->data = NULL;
	}
}

FUNK_NATIVE_FUNCTION_DEFINITION(fileCallback) {
	FunkFileData* data = extract_file_data(vm, (FunkFunction *) self);
	const char* string = funk_read_file(data->path);

	if (string == NULL) {
		return NULL;
	}

	FUNK_RETURN_STRING(string);
}

FUNK_NATIVE_FUNCTION_DEFINITION(file) {
	FUNK_ENSURE_ARG_COUNT(1);

	if (args[0] == NULL) {
		return NULL;
	}

	FunkNativeFunction* function = funk_create_native_function(vm, funk_create_string(vm, "$fileData", 9),(FunkNativeFn) fileCallback);
	FunkFileData* data = (FunkFileData*) vm->allocFn(sizeof(FunkFileData));

	uint16_t length = args[0]->name->length;

	data->file = fopen(args[0]->name->chars, "rw");
	data->path = (char*) vm->allocFn(length);

	memcpy((void*) data->path, args[0]->name->chars, length + 1);

	function->data = data;
	function->cleanupFn = cleanup_file_data;

	return (FunkFunction *) function;
}

FUNK_NATIVE_FUNCTION_DEFINITION(close) {
	FUNK_ENSURE_ARG_COUNT(1);
	FunkFileData* data = extract_file_data(vm, args[0]);

	if (data->file != NULL) {
		fclose(data->file);
		data->file = NULL;
	}

	return NULL;
}

static inline bool is_file(FunkFunction* argument) {
	return argument != NULL && argument->object.type == FUNK_OBJECT_NATIVE_FUNCTION && ((FunkNativeFunction*) argument)->cleanupFn == cleanup_file_data;
}

FUNK_NATIVE_FUNCTION_DEFINITION(readLine) {
	void* stream = stdin;

	if (argCount > 0 && is_file(args[0])) {
		stream = extract_file_data(vm, args[0])->file;

		if (stream == NULL) {
			return NULL;
		}
	}

	char* line = NULL;
	size_t length = 0;

	length = getline(&line, &length, stream);
	FunkString* string = funk_create_string(vm, line, length);

	free(line);
	return (FunkFunction *) funk_create_basic_function(vm, string);
}

void funk_open_std(FunkVm* vm) {
	FUNK_DEFINE_FUNCTION("array", array);
	FUNK_DEFINE_FUNCTION("map", map);
	FUNK_DEFINE_FUNCTION("push", push);
	FUNK_DEFINE_FUNCTION("remove", _remove);

	FUNK_DEFINE_FUNCTION("NULLA", nulla);
	FUNK_DEFINE_FUNCTION("variable", variable);

	FUNK_DEFINE_FUNCTION("print", print);
	FUNK_DEFINE_FUNCTION("printNumber", printNumber);
	FUNK_DEFINE_FUNCTION("clock", _clock);
	FUNK_DEFINE_FUNCTION("readLine", readLine);

	FUNK_DEFINE_FUNCTION("set", set);
	FUNK_DEFINE_FUNCTION("get", get);
	FUNK_DEFINE_FUNCTION("equal", equal);
	FUNK_DEFINE_FUNCTION("notEqual", notEqual);
	FUNK_DEFINE_FUNCTION("not", not);

	FUNK_DEFINE_FUNCTION("if", _if);
	FUNK_DEFINE_FUNCTION("while", _while);
	FUNK_DEFINE_FUNCTION("for", _for);

	FUNK_DEFINE_FUNCTION("space", space);
	FUNK_DEFINE_FUNCTION("separator", separator);
	FUNK_DEFINE_FUNCTION("dot", dot);
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

	FUNK_DEFINE_FUNCTION("require", require);

	FUNK_DEFINE_FUNCTION("file", file);
	FUNK_DEFINE_FUNCTION("close", close);
}