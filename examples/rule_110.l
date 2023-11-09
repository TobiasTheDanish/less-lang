func printChar(ch: int) {
	syscall(1, 1, ch, 1);
}

func printString(str: string) {
	syscall(1, 1, str, 6);
}

func main() {
	const ROWS = 100;
	let next = int[100];
	next[0] = 32;
	next[99] = 32;
	let display = int[100];

	let mut i = 0;
	while i < display.len {
		display[i] = 95;
		i = i + 1;
	}
	
	display[90] = 42;
	display[99] = 10;

	i = 0;
	while i < ROWS {
		let mut g = 0;

		while g < 101 {
			printChar(display[g]);
			g = g + 1;
		}
		let mut j = 1;

		while j < 100-2 {
			if 42 == display[j-1] && 42 == display[j] && 95 == display[j+1] {
				 next[j] = 42;
			} else if 42 == display[j-1] && 42 != display[j] && 42 == display[j+1] {
				 next[j] = 42;
			} else if 42 != display[j-1] && 42 == display[j] && 42 == display[j+1] {
				 next[j] = 42;
			} else if 42 != display[j-1] && 42 == display[j] && 42 != display[j+1] {
				 next[j] = 42;
			} else if 42 != display[j-1] && 42 != display[j] && 42 == display[j+1] {
				 next[j] = 42;
			} else {
				next[j] = 95;
			}

			j = j + 1;
		}

		let mut h = 0;
		while h < 99 {
			display[h] = next[h];
			h = h + 1;
		}
		i = i + 1;
	}
}
