func main() {
	let arr = i8[100];
	let mut i = 0;
	while i < arr.len {
		arr[i] = i + 1;
		dump arr[i];
		i = i + 1;
	}
}
