func main() {
	let arr = int[4];
	let mut i = 0;
	while i < arr.len {
		arr[i] = 100 * i;
		dump arr[i];
		i = i + 1;
	}
}
