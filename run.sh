#!/bin/sh

echo "./less $2 -o $1"
./less $2 -o $1
retval=$?
if [ $retval -ne 0 ] ; then
	echo "Compilation exited with non zero exit code: '$retval'"
	exit $retval
fi

echo "nasm -felf64 -g $1.asm"
nasm -felf64 -g $1.asm
retval=$?
if [ $retval -ne 0 ] ; then
	echo "Assembler exited with non zero exit code: '$retval'"
	exit $retval
fi

echo "ld -g -o $1 $1.o"
ld -g -o $1 $1.o
exit $?
