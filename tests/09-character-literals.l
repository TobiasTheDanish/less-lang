func print(str: string) {
	syscall(1,1,str.chars[0], str.len);
}

func main() {
	print("Please input text:\n");

	let buf = i8[100];
	while 1 == 1 {
		let bytesRead = syscall(0, 0, buf[0], buf.len);

		if buf[0] == 'q' {
			syscall(1, 1, "Quit.\n", 6);
			syscall(60, 0);
		} 

		syscall(1, 1, buf[0], bytesRead);
	}
}
