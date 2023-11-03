func main() {
	let arr = int[10];
	arr[0] = 10000;
	let mut i = 1;
	while i < arr.len {
		arr[i] = arr[i-1] * i;
		dump arr[i];
		i = i + 1;
	}
}
