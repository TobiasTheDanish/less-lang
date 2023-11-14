struct User {
	id: int;
	name: string;
}

func print(str: string, len: int) {
	syscall(1, 1, str, len);
}

func printUser(user: User) {
	dump user.id;
	print(user.name, 8);
}

func main() {
	let user = User{ id = 1, name = "Tobias\n" };
	printUser(user);
}
