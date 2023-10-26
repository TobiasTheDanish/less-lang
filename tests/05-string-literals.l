func main() {
	let str = "\t\"Hello \vworld!\"\n";

	// syscall 1 is the write syscall
	// fd 1 is stdout
	syscall(1, 1, str, 17);
}
