struct String {
	len: i32;
	chars: i8[];
}

func add(str: String, char: i8) {
	str.chars[str.len] = char;
	str.len = str.len + 1;
}

func print(str: String) {
	syscall(1, 1, str.chars[0], str.len);
}

func main() {
	let mut str = String{ len: 0, chars: i8[20] };

	str.add(72);
	str.add(101);
	str.add(108);
	str.add(108);
	str.add(111);
	str.add(10);

	str.print();
}
