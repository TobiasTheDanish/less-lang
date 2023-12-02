const sys_write = 1;
const std_out = 1;

func print(s: string) {
	syscall(sys_write, std_out, s, 30);
}

func add(mut a: i32, b: i32) {
	a = a + b;
	dump a;
}

func main() {
	let num1 = 35;
	let num2 = 34;
	num1.add(num2);
	let str = "Hello world\n";
	str.print();
}
