func main() {
	let abc = i8[4];
	abc[0] = 65;
	abc[1] = 66;
	abc[2] = 67;
	abc[3] = 10;

	if abc[0] == 69 {
		syscall(60, 69);
	} else {
		syscall(1, 1, abc[0], 4*8);
	}
}
