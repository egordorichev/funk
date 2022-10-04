#include "funk.h"
#include "funk_std.h"

#include <stdio.h>
#include <stdlib.h>

const char* read_file(const char* path) {
	FILE* file = fopen(path, "rb");

	if (file == NULL) {
		return NULL;
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	char* buffer = (char*) malloc(fileSize + 1);
	size_t bytes_read = fread(buffer, sizeof(char), fileSize, file);
	buffer[bytes_read] = '\0';

	fclose(file);
	return (const char*) buffer;
}

void print_error(FunkVm* vm, const char* error) {
	fprintf(stderr, "%s\n", error);
}

int run_file(const char* file) {
	const char* source = read_file(file);
	FunkVm* vm = funk_create_vm(malloc, free, print_error);

	funk_open_std(vm);
	funk_run_string(vm, file, source);

	funk_free_vm(vm);
	free((void*) source);

	return 0;
}

int main(int argc, const char** argv) {
	if (argc == 2) {
		return run_file(argv[1]);
	}

	printf("funk [file]\n");
	return 0;
}