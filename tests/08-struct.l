struct User {
	id: int;
	name: string;
}

func print(str: string, len: int) {
	syscall(1, 1, str, len);
}

func main() {
	let user = User{ id = 1, name = "Tobias\n" };

	dump user.id;
	print(user.name, 8);
}
