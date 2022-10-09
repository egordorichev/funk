#include "funk.h"
#include "funk_std.h"

#include <stdio.h>
#include <stdlib.h>

void print_error(FunkVm* vm, const char* error) {
	funk_print_stack_trace(vm);
	fprintf(stderr, "%s\n", error);
}

int run_file(const char* file) {
	FunkVm* vm = funk_create_vm(malloc, free, print_error);

	funk_open_std(vm);
	funk_run_file(vm, file);
	funk_free_vm(vm);

	return 0;
}

int main(int argc, const char** argv) {
	if (argc == 2) {
		return run_file(argv[1]);
	}

	printf("funk [file]\n");
	return 0;
}