let A = 65;
let B = 66;
let C = 67;
let newLine = 10;

if A == 69 {
	syscall(60, 69);
} else {
	syscall(1, 1, &A, 4*8);
}
