func print(buf: i8[]) {
	syscall(1, 1, buf[0], buf.len);
}

func printChar(ch: i8) {
	syscall(1, 1, ch, 1);
}

func main() {
	const ROWS = 100;
	let next = i8[100];
	next[0] = ' ';
	next[next.len-1] = ' ';
	let display = i8[100];

	let mut i = 0;
	while i < display.len {
		display[i] = ' ';
		i = i + 1;
	}
	
	display[97] = '*';
	display[display.len-1] = '\n';

	i = 0;
	while i < ROWS {
		print(display);

		let mut j = 1;

		while j < display.len-2 {
			if '*' == display[j-1] && '*' == display[j] && '*' != display[j+1] {
				 next[j] = '*';
			} else if '*' == display[j-1] && '*' != display[j] && '*' == display[j+1] {
				 next[j] = '*';
			} else if '*' != display[j-1] && '*' == display[j] && '*' == display[j+1] {
				 next[j] = '*';
			} else if '*' != display[j-1] && '*' == display[j] && '*' != display[j+1] {
				 next[j] = '*';
			} else if '*' != display[j-1] && '*' != display[j] && '*' == display[j+1] {
				 next[j] = '*';
			} else {
				next[j] = ' ';
			}

			j = j + 1;
		}

		let mut h = 0;
		while h < next.len-1 {
			display[h] = next[h];
			h = h + 1;
		}
		i = i + 1;
	}
}
