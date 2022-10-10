# funk

A language with the only data type being a function!

### Todo before release

* docs
* tests?
* check all todos

### Demos

* BF compiler

### Possible future improvements

* Using custom callframe stack instead of the C one
* goto jumping in the instruction loop
* single arg lambdas `a => print(a)`
* Exception system
* Pass command line arguments to the program
* Dispose of unreachable strings (right now they all are marked via the vm->strings)
* Automatic garbage collection (tricky, need to make sure no objects are hanging without a root on the heap, otherwise they will be picked up at random times and deleted)