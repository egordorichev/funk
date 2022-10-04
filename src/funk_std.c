#include "funk_std.h"
#include <stdio.h>

FUNK_DEFINE_NATIVE_FUNCTION(print) {
	for (uint8_t i = 0; i < arg_count; i++) {
		// todo: instead of this, it should just call the function
		printf("%s\n", funk_to_string(vm, args[i]));
	}

	return NULL;
}

void funk_open_std(FunkVm* vm) {

}