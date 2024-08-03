func booleanFunction(num: i32): bool {
	return num < 420;
}

func main() {
	let funcRes = booleanFunction(69);
	let thirty: i32 = 30;
	let cond = thirty + 34;
	let mut res: i32 = 0;

	if funcRes && cond == 69 || cond == 420 {
		res = 500 - 20;
	} else if cond == 60 {
		res = 500 - 200;
	} else {
		res = 207 / 3;
	}

	dump res;
}
