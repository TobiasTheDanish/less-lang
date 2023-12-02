func print(str: string) {
	syscall(1, 1, str.chars[0], str.len);
}

func main() {
	let mut i = 1;

	while i < 26 {
		if i % 3 == 0 && i % 5 == 0 {
			print("FIZZ BUZZ\n");
		} else if i % 3 == 0 {
			print("FIZZ\n");
		} else if i % 5 == 0 {
			print("BUZZ\n");
		} else {
			dump i;
		}

		i = i + 1;
	}
}
