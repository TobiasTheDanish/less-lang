struct User {
	id: i32;
	name: string;
}

func print(str: string) {
	syscall(1, 1, &str.chars[0], str.len);
}

func printUser(user: User) {
	dump user.id;
	print(user.name);
}

func main() {
	let user = User{ id: 1, name: "Tobias\n" };
	printUser(user);
}
