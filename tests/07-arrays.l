func main() {
	let arr = int[100];
	let mut i = 0;
	while i < arr.len {
		arr[i] = i + 1;
		dump arr[i];
		i = i + 1;
	}
}
