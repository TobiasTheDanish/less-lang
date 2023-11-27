struct String {
	len: i32;
	chars: i8[];
}

func add(str: String, char: i8) {
	&str.chars[str.len] = char;
	str.len = str.len + 1;
}

func print(str: String) {
	syscall(1, 1, str.chars[0], str.len);
}

func main() {
	let mut str = String{ len: 0, chars: i8[20] };

	str.add('H');
	str.add('e');
	str.add('l');
	str.add('l');
	str.add('o');
	str.add('\n');

	str.print();
}
