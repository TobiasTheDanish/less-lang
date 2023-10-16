# Less programming language #

A toy language developed for fun and as a way to learn about building compilers.
Less only supports linux, and will probably only support linux (x86_64 to be more precise).



---



## Usage

To compile and assemble a Less file (suffixed '.l'), for now a shell script is needed.
You can run the following commands to compile one of the example scripts in the 'tests' directory:

```console
make
./run.sh output/program tests/02-conditionals.l
```

The above command runs the 'less' executable, produced by the make command, and then assembles, and links the outputted assembly.

You can also compile the program to assembly via the 'less' executable. However, you will for now have to call the assembler and linker manually.

To see the usage of the executable, simply execute the command with no arguments.

```console
./less
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
- [ ] Arrays



---



## Documentation

### Variables

A variable is declared with the 'let' keyword, and (for now) has to be assigned a value at declaration, example:

```
let fortyTwo = 42;
```



---



### Arithmetic

Less supports basic arithmetic, '+', '-', '*', '/'. However, the mathematical order of precedence i currently not followed.

```
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

```
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

```
let str = "\t\"Hello \vworld!\"\n";

// the next line writes the content of the 'str' variable to stdout
syscall(1, 1, str, 17);
// syscall 1 is the write syscall
// fd 1 is stdout
```
