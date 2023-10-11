# Less programming language #

A toy language developed for fun and as a way to learn about building compilers.
Less only supports linux, and will probably only support linux.

## Usage

To compile and assemble a Less file (suffixed '.l'), for now a shell script is needed.
You can run the following commands to compile one of the example scripts in the 'tests' directory:

```console
make
./run.sh output/program tests/02-conditionals.l
```

The above command runs the 'less' executable, produced by the make command, and then assembles, and links the outputed assembly.

You can also compile the program to assembly via the 'less' executable. However, you will for now have to call the assembler and linker manually.

To see the usage of the executeable, simply execute the command with no arguments.

```console
./less
```

## Dependencies

The above mentioned command relies on nasm to do the assembling. [Heres a link to the official website](https://www.nasm.us/).

## Syntax

C-style syntax

## Features

- [x] Basic integer arithmetic
- [x] Conditionals (if-else statements)
- [ ] Loops
- [ ] Memory access and storage
- [ ] Support for linux syscalls

