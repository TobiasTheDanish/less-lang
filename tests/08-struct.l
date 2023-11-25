struct User {
	id: i32;
	name: string;
}

func print(str: string, len: i32) {
	syscall(1, 1, str, len);
}

func printUser(user: User) {
	dump user.id;
	print(user.name, 8);
}

func main() {
	let user = User{ id: 1, name: "Tobias\n" };
	printUser(user);
}
