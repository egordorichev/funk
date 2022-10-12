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
funk uses roman literals for representing numbers. For example, X is 10, and XLIV is 44. What about 0? It's NULLA.

And by calling the function `char` with argument `XLIV` (44), we create a function named `,`. Wild, I know.
Same story happens with `XXXIII`, that becomes a function called `!`, and `space()` returns a function named ` `.
Then all of these functions are passed as arguments to `join()`, that creates a new function with the name consisting
from all of these arguments combined, and finally, it is all printed...

Phew.

And if you don't really like debugging with output looking something like MMMCMXCIX, there is a handy function, that
outputs an actual human-readable number to the terminal:
```
printNumber(MMMCMXCIX) // 3999
```

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

	funk_open_std(vm); // Include the standard library
	funk_run_file(vm, "test.funk");
	funk_free_vm(vm);
	
	return 0;
}
```

### Building (!!!)

Before we continue, here are the steps to build the project:

```bash
git clone https://github.com/egordorichev/funk && cd funk
cmake . && make && sudo make install
```

And then, you can run any funk code with:

```bash
funk demos/hello_world.funk
```

### Functions in funk

So, as you would imagine, being built on top of functions, they play a huge part in the language.
So there are quite a few ways you can go about defining them. Lets look at them all:

```js
function someFunction(callback) {
	print(callback(I, II))
}

function a(arg0, arg1) {
	return arg0 + arg1
}

someFunction(a)

someFunction((arg0, arg1) => {
	return arg0 * arg1
})

// As in most languages, if your anonymous function body is a single expression,
// you can remove the {}
someFunction((arg0, arg1) => arg0 * arg1)

// But going futher than that, if you don't take any arguments, the function
// can be just a block of code:
someFunction({
  print(hi)
  return XX
})
```

Under the hood, functions have two different types: basic & native.
This is only relevant for you in the context of embedding the language.

Basic functions are the functions defined in the code, they contain compiled code,
constants and their variables. Native functions have a pointer to a C function, a general purpose pointer to store extra data
and a cleanup function, for freeing the data.

### Control flow

Funk is turing complete. It has if-statements, while & for loops. **The only value, that is evaluated
as true, is a function with the name "true".**
Ifs take a condition function, an if branch and an optional else branch:

```js
if(condition(), {
	print(true)
}, {
	// else
    print(false)
})
```

While loops work on the same premise:

```js
while(condition(), {
	doSomething()
})
```

For loops are a bit smarter, you can iterate a "string", "array" or "map" with them,
as well as just go from number A to B:

```js
for(NULLA, X, (i) => {
	printNumber(i)
})
	
for(hello, (letter) => {
	print(letter)
})
```

I put "string", "array" and "map" into quotes, because in the reality, they are still
functions. We will talk about them a bit later, tho.

### Standard library

And if we are talking about functions anyway, lets look at what the language provides out of the box: the standard library.

#### Comparison operations

`equal(a, b)`, `notEqual(a, b)`, `not(a)`, 
`less(a, b)`, `lessEqual(a, b)`, `greater(a, b)`, `greaterEqual(a, b)`, 
`and(a, b)` and `or(a, b)`

all replace the operators, you would expect to see in any normal language.
Just for the sake of example, `greaterEqual(X, I)` in funk is the same as `10 >= 1` in JS.

#### Control flow operations

`if(condition, then, [else])` is equal to `if (condition()) { then() } else if (else != null) { else() }`

`while(condition, do)` is equal to `while (condition()) { do() }`

`for(from, to, callback)` calls the callback with the value starting at `from` and going to `to` (non-inclusive)

`for(iteratable, callback)` calls the callback with all the values of the iteratable ("string", "array" or "map") 

#### Math operations

`add(a, b)`, `subtract(a, b)`, `multiply(a, b)`, `divide(a, b)`, `cos(a)` and `sin(a)`
are also binding of simple operators and math functions. Nothing complex here.

#### String operations

`length(string)` returns the length of a "string", "array" or a "map"

`join(a, b, c ... n)` glues together all the provided "strings" into a single one

`substring(string, from, [to])` returns a part of the string from the index `from` to `to` (indexes start at 0). If the argument `to` is not provided,
it becomes equal to `from`, and the function returns a single char at that index

`char(byte)` returns a "string" with the name being a single char with the ascii code equal to the argument

`space()` returns a "string" with a name made of a single space

`space()` returns a "string" with a name made of a single dot

`space()` returns a "string" with a name made of a single slash

#### Array operations

In funk, "arrays" are functions, that return different values based on the index you give them, and you can change the output of the function at any given index with a new value

`array(a, b, c .. n)` returns an "array" from the given elements. The returned function has a special name `$arrayData`, that you need to call to access the "array" elements

```js
set(myArray, array(a, b))
print(myArray(NULLA)) // a
```

`$arrayData(index)` returns "array" element at given index

`$arrayData(index, value)` sets the "array" element at given index to the provided value

`length(a)` returns the length of a "string", "array" or a "map"

`push(array, value)` appends the value to the end of an "array"

`remove(array, index)` removes a value at the given from the "array"

`pop(array)` removes and returns the last element of the "array"

#### Map operations

"Maps" operate on the same premise, as arrays, but instead of treating "strings" as numbers, they actually use "strings" as dictionary/table/map/object keys, call that however you want


`array(keyA, valueA, keyB, valueB, ... keyN, valueN)` returns a "map" from the given elements. Each key is followed by a corresponding value. The returned function has a special name `$mapData`, that you need to call to access the "map" elements

```js
set(myMap, map(a, I, b, X))
print(myMap(b)) // X
```

`$mapData(key)` returns "map" element at given key

`$mapData(key, value)` sets the "map" element at given key to the provided value

`remove(map, key)` removes the given value from the "map"

`length(a)` returns the length of a "string", "array" or a "map"

#### IO operations

`print(a, b, ... n)` prints given values to the terminal, **each followed by a new line**

`printNumber(a, b, ... n)` same as `print`, but tries to convert "strings" to numbers

`printChar(byte)` prints a single char (byte is converted to a number) to the terminal, **without a new line**

`readLine()` reads and returns a single line from the `stdin` (awaits for user to press ENTER)

`file(path)` returns a special function with the name `$fileData`, that is used for reading a file at the given path. Returns null, if the file does not exist or it fails to open it

`$fileData()` reads the whole file and returns it as a "string"

`readLine(fileData)` reads a single line from the given "file" (must be created with a call to the `file()` function)

`close(fileData)` closes the given "file"

`clock()` returns time since the program start, in seconds

#### Modules

`require(path)` attempts to run a file, with the name `path + '.funk'`. The path is relative. If it is successful, it returns the value, returned by the file (yes, you can have a top-level return statement).
**It is also being registered internally, and the next time you require the same module, it just returns the old return value**

It also treats dots as separators, so that you don't have to glue together the names of modules:

```js
print(require(tests.module)) // XI
```

The top-level functions are declared globally, so any function
defined in any module after it being executed becomes aviable to be used anywhere

#### Variables

This part is a bit hard to understand, but stick with me.
Say, we want a variable `food`. Okay, easy enough:

```js
print(food) // food (returns the function name, because it doesn't exist)
set(food, bread)
// And we can now print it:
print(food) // bread
```

But what if we want to change it from `bread` to `pizza`?

```js
set(food, bread)
set(food, pizza)
print(food) // bread
```

Why is it so? Lets analyze it step by step:

```js
// Here, food == food
set(food, bread)
// Now, food == bread
print(food)
// So, when we go to set the variable to pizza
// We set bread == pizza, instead of food == pizza
set(food, pizza)
print(food) // bread
print(bread) // pizza
```

So how do we solve that? Well, the most simple solution, that I came up with, is wrapping
the variable with a function, that modifies it name (adds a `$` before it), so that we never actually
set the variable named `food`, we set a variable called `$food`:

```js
// Here, food == food
set(variable(food), bread)
// Now, food == bread
print(get(variable(food)))
// So, when we go to set the variable to pizza
// We set bread == pizza, instead of food == pizza
set(variable(food), pizza)
print(get(variable(food))) // pizza
```

`set(var, value)` sets `var` to `value` in the scope `var` was first mentioned in, or if it wasn't, in the current one

`get(var)` returns the value of variable called `var`

`variable(name)` returns `$ + name`

#### Garbage collection

Garbage collection in funk doesn't trigger, unless you explicitly tell it to do so.
It is not optimal by any means, but if it can run at any memory allocation, it brings in so many possible bugs,
and I'm not willing to make this implementation that much complex.

`collectGarbage()` runs the garbage collector

### Possible future improvements

The implementation of funk is much more simple, that the most of the languages out there,
lit is probably at least 10 times more complex, than funk, so funk is a
really nice playground to try and learn some stuff about language development,
for example you can try adding these features:

* Try/catch
* Using custom call frame stack instead of the C one
* Optimization: goto jumping in the instruction loop
* Single arg lambdas `a => print(a)`
* Exception system
* Pass command line arguments to the program
* Dispose of unreachable strings (right now they all are marked via the vm->strings)
* Automatic garbage collection (tricky, need to make sure no objects are hanging without a root on the heap, otherwise they will be picked up at random times and deleted)

### Embedding

Funk is designed to not be only used as a standalone language,
but also as a scripting language. We already showed the basic usage example above,
you just add `funk.c` and `funk.h` to you project, and you are good to go.

If you want the standard library, you will also need `funk_std.h` and `funk_std.c`.
These files are actually a great example, of how to bind your own functions to C, but here is a quick demo:

_mylib.h_
```c
#ifndef MYLIB_H
#define MYLIB_H

#include "funk.h"
void open_mylib(FunkVm* vm);

#endif
```

_mylib.c_
```c
#include "mylib.h"

// Creates a function FunkFunction* myFunction(FunkVm* vm, FunkFunction* self, FunkFunction** args, uint8_t argCount)
FUNK_NATIVE_FUNCTION_DEFINITION(myFunction) {
  // Do stuff		
  return NULL;
}

void funk_open_std(FunkVm* vm) {
  // Create a global variable	
  funk_set_global(vm, "myVariable", (FunkFunction *) funk_create_empty_function(vm, "XI"));
  // Create a global function 
  FUNK_DEFINE_FUNCTION("myFunction", myFunction);
}
```

You can return functions from your created functions with these helpers:

`FUNK_RETURN_STRING(string)` returns a function with the name `string`

`FUNK_RETURN_NUMBER(number)` returns a function with the name converted to roman from the argument `number`

`FUNK_RETURN_BOOL(value)` returns a function with the name `true` or `false` depending on the input

And these two are helpful for checking upon your aguments:

`FUNK_ENSURE_ARG_COUNT(count)` throws an error, if the `argCount` is not equal to `count`

`FUNK_ENSURE_MIN_ARG_COUNT(count)` throws an error, if the `argCount` is less than `count`