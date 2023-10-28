const sys_write = 1;
const std_out = 1;

func add(a: int, b: int) {
	dump a + b;
}

func print(s: string) {
	syscall(sys_write, std_out, s, 30);
}

func main() {
	let num = 35;
	add(num, 34);
	add(380, 40);
	print("Hello world!\n");
	std_out = 2;
}
