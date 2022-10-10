# funk

A language with the only data type being a function!
Here is a 'Hello, world', written in funk:

```js
// ascii 44 (XLIV) is ,
// ascii 33 (XXXIII) is !
print(join(Hello, char(XLIV), space(), world, char(XXXIII)))
```

Looks pretty complex, huh? Let's unpack it step-by-step.
First of all, funk has functions, that look just like JavaScript:

```js
function greet() {
	print(hello)
}

greet()
```

Seems simple enough, but what is `hello`? Is it a string? Didn't I tell you, that this language has only functions?
Well, you see, `hello` is actually the opposite of an anonymous function: it has a name, but it has no code/body.
And since, like in JS, you can pass functions as arguments, here we just pass function `hello` as an argument to `print`.
We didn't define `hello`? No problem, the language will do it for us, creating it at runtime, but it won't be defined
globaly, so you won't be able to call `hello()` from anywhere in the code, like you would be able to call that sweet
sweet `greet` function, we just created.

Okay, but what is `XLIV` you may ask? These are [roman literals](https://en.wikipedia.org/wiki/Roman_numerals).
Since function names can't start with a digit (tho, they can with a minus or they can have a dot inside of their name...),
funk uses roman literals for representing numbers. For example, X is 10, and XLIV is 44.

And by calling the function `char` with argument `XLIV` (44), we create a function named `,`. Wild, I know.
Same story happens with `XXXIII`, that becomes a function called `!`, and `space()` returns a function named ` `.
Then all of these functions are passed as arguments to `join()`, that creates a new function with the name consisting
from all of these arguments combined, and finally, it is all printed...

Phew.

Obviously, this is not a very easy language to use. It is far from being as unusable, as [BF](https://en.wikipedia.org/wiki/Brainfuck) (by the way, [here is a BF interpreter in funk](https://github.com/egordorichev/funk/blob/master/demos/bf.funk)),
but still even working with numbers gets pretty complex fast... (for example, `2 + 2 * 2` is `add(2, multiply(2, 2))`).

It is created for a fun design challenge, and it's also a pretty small nice implementation to learn language development on...
But it is actually very easy to embed, just like so:

```c
void print_error(FunkVm* vm, const char* error) {
	funk_print_stack_trace(vm);
	fprintf(stderr, "%s\n", error);
}


int main(int argc, const char** argv) {
	FunkVm* vm = funk_create_vm(malloc, free, print_error);

	funk_open_std(vm);
	funk_run_file(vm, "test.funk");
	funk_free_vm(vm);
	
	return 0;
}
```

[MORE DOCS IN PROGRESS]

### Possible future improvements

* Try/catch
* Using custom callframe stack instead of the C one
* goto jumping in the instruction loop
* single arg lambdas `a => print(a)`
* Exception system
* Pass command line arguments to the program
* Dispose of unreachable strings (right now they all are marked via the vm->strings)
* Automatic garbage collection (tricky, need to make sure no objects are hanging without a root on the heap, otherwise they will be picked up at random times and deleted)