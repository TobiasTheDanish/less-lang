const sys_write = 1;
const std_out = 1;

func print(s: string) {
	syscall(sys_write, std_out, s, 30);
}

func add(a: int, b: int) {
	let num = a+b;
	dump num;
}

func main() {
	let mut num1 = 35;
	let mut num2 = 34;
	add(num1, num2);
	num1 = 80;
	num2 = 340;
	add(num1, num2);
}
