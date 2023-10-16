let i = 1;

while i < 26 {

	if i % 3 == 0 {
		if i % 5 == 0 {
			syscall(1, 1, "FIZZ BUZZ\n", 11);
		} else {
			syscall(1, 1, "FIZZ\n", 6);
		}
	} else {
		if i % 5 == 0 {
			syscall(1, 1, "BUZZ\n", 6);
		} else {
			dump i;
		}
	}

	i = i + 1;
}
