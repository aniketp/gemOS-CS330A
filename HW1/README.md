### Maximum Prime calculator
This is a demontration of how `C` and `Assembly` functions can be called from each other, by creating a sample Maxium prime calculator.

### Functionality
* `isPrime()` is written in x86-64 assembly, and is called from main funtion in `main.c`
* `find_square_root()` function, which is written in C, is called from `isPrime()`.

### Usage
* Run `make` to compile and assemble the final executable. <br>
* Run `make run` to execute the `prime` binary
