#!/bin/sh

echo "./less $2 -o $1"
./less $2 -o $1

echo "nasm -felf64 -g $1.asm"
if ! nasm -felf64 -g $1.asm; then
	exit $?
fi

echo "ld -g -o $1 $1.o"
ld -g -o $1 $1.o
exit $?
