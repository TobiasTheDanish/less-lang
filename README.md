# less-lang #

A toy language developed for fun and as a way to learn about building compilers.
Less only supports linux, and will probably only support linux (x86_64 to be more precise).



---



## Usage

To compile and assemble a less-lang file (suffixed '.l'), for now a shell script is needed.
You can run the following commands to compile one of the example scripts in the 'tests' directory:

```bash
make #builds the compiler

./lessl examples/fizzbuzz.l -o output/program #compiles one of the test programs
```

The 'lessl' executable, produced by the make command, first compiles the program to assembly, then assembles and links that assembly.

To see the usage of the executable, use one of the following commands:

```bash
./lessl

./lessl -h
```



---



## Dependencies

The above-mentioned command relies on nasm to do the assembling. [Here's a link to the official website](https://www.nasm.us/).



---



## Syntax

C-style syntax



---



## Features

- [x] Basic integer arithmetic
- [x] Conditionals (if-else statements)
- [x] Loops
- [x] Memory access and storage (Primitive types)
- [x] Support for Linux syscalls
- [x] String literals
- [x] Functions
- [x] Identifier properties
- [x] Arrays
- [x] Structs
- [x] Character literals
- [ ] Import of other source files
- [ ] Ways of allocating memory



---



## Documentation

### Variables

A variable is declared with the 'let' keyword.

Variables are by default immutable, if you wish to reassign a variable, add the 'mut' keyword to the declaration, example:

```rust
let num = 42;
num = 34 + 35; // This doesn't compile, since num is attempted to be reassigned, but not marked as mutabled

let mut mut_num = 42;
mut_num = 34 + 35; // This compiles and reassigns mut_num;
```
For now a variable has to be assigned a value at declaration.

Variables can have type declarations, or you can let the compiler infer the type, example:

```rust
let char = 42; // Here the compiler infers the variable to be of type i8, simply based on the value being less than 255.
let int: i32 = 42; // Here the variables type if i32, even if the value is smaller.
```

Current built-in types:

- string
  - props:
    - len: i32 - the length of the string
    - chars: i8[] - the actual array of characters
- i8
- i16
- i32
- i64

---



### Constants

A constant is, like immutable variables, not able to be reassigned. 

A constant is stored in the data segment of the program's memory while a variable is stored on the stack.

Constants also has to be assigned values that is known at compile time.

```javascript
const someConst = someVar; // This is invalid, since someVar is not a value known at compile time
const stdout = 1;

...

stdout = 2; // Also invalid, compilation will fail.
```



---



### Arithmetic

less-lang supports basic arithmetic, '+', '-', '*', '/'. However, the mathematical order of precedence i currently not followed.

```javascript
let fortyTwo = 22 + 20;

//Will set wrong to 64:
let wrong = 2 * 10 + 22;
```


---



### Comments

Less supports two types of comments, single and multi line comments.
```go
//This is a comment

/* 
   This
   is
   a
   multiline
   comment
*/
```



---



### Linux syscalls

Linux syscalls are support. You can trigger a syscall via:  `syscall('params');`

Follow this [link](https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#x86_64-64_bit) for a table over the different linux syscalls



---



### String literals

String literals are denoted by double quotes.

```go
"Hello world"
```

The following escape characters are support:

- \n: Newline (Line Feed)
- \t: Tab
- \r: Carriage Return
- \b: Backspace
- \f: Form Feed
- \\: Backslash
- \": Double Quote
- \': Single Quote (not available in all languages)
- \0: Null character
- \v: Vertical Tab

An example program using string literals with some of the different escape characters.

```go
func main() {
   let str = "\t\"Hello \vworld!\"\n";

   // the next line writes the content of the 'str' variable to stdout
   syscall(1, 1, str.chars[0], str.len);
   // syscall 1 is the write syscall
   // fd 1 is stdout
}
```


---



### Functions

Functions are declared by the 'func' keyword, followed by the name of the function,
the functions parameters enclosed by parentheses, and the function block itself, enclosed in curly brackets.

Any less-lang program needs a main function to allow it to be compiled and executed, e.g:
```go
func print(str: string) {
    syscall(1,1,str.chars[0],str.len);
}

func main() {
    print("Hello world!\n");
}
```

NOTE: For now any function referenced in another function have to be declared first, reading from top to bottom.


---


### Properties

Properties are data stored in built-in types and structs.

They can be accessed like so: 

```rust
let bytes = i8[1024];

// syscall to read from stdin
let bytesRead = syscall(0, 0, bytes, bytes.len);

// some error handling...

// Write the input back to stdout
syscall(1, 1, bytes, bytesRead);
```


---


### Arrays

An array is a contiguous block of memory.

In less you can initialize an array like this:

```rust
let bytes = i8[1024];
```

You access an element in the array like so: 

```rust
let bytes = i8[1024];

bytes[0] = 42;
```

Arrays have the following properties:

- len : i32 representing the total amount of elements the array can store.


---


### Structs

Structs are the way to group data. 

You first have to declare a struct, e.g:

```c
struct Foo {
    bar: i32;
    baz: i16;
}
```

Foo can now be used as a type. 

A variable of type foo will have access to properties 'bar' and 'baz'.

For example, in a function declaration: 

```go
func fooFunc(foo: Foo) {
    let sum = foo.bar + foo.baz;

    dump sum; // Prints numbers to stdout;
}
```

To be able to pass a variable of a custom struct, you have to initialize it first.

To initialize a struct you do the following: 

```go
func main() {
    let foo = Foo { bar: 42, baz 69 };

    fooFunc(foo);
}
```

The example in its entirety would look like this:

```c
struct Foo {
    bar: i32;
    baz: i16;
}

func fooFunc(foo: Foo) {
    let sum = foo.bar + foo.baz;

    dump sum; // Prints numbers to stdout;
}

func main() {
    let foo = Foo { bar: 42, baz 69 };

    fooFunc(foo);
}
```

---


## Control flow


### If/else blocks

TODO

---

### Loops

TODO


