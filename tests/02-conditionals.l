func booleanFunction(): bool {
	return 69 < 420;
}

func main() {
	let true = booleanFunction();
	let thirty: i32 = 30;
	let cond = thirty + 34;
	let mut res: i32 = 0;

	if true && cond == 69 || cond == 420 {
		res = 500 - 20;
	} else if cond == "string" {
		res = 500 - 200;
	} else {
		res = 207 / 3;
	}

	dump res;
}
