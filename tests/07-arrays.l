func main() {
	let arr = i8[10];
	let mut i = 0;
	while i < arr.len {
		arr[i] = 'a' + i;
		dump arr[i];
		i = i + 1;
	}
}
