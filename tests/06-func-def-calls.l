const sys_write = 1;
const std_out = 1;

func print(s: string) {
	syscall(sys_write, std_out, s, 30);
}

func add(a: int, b: int) {
	let num = a+b;
	print("num from add: ");
	dump num;
}

func main() {
	let num1 = 35;
	let num2 = 34;
	add(num1, num2);
	print("num2 from main: ");
	dump num2;
}
