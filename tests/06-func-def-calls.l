func add(a: int, b: int) {
	dump a + b;
}

func print(s: string) {
	syscall(1, 1, s, 30);
}

func main() {
	let num = 35;
	add(num, 34);
	add(380, 40);
	print("Hello world!\n");
}
