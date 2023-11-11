struct User {
	id: int;
	name: string;
}

func main() {
	let user = User{ id = 1, name = "Tobias" };

	dump user.id;
	dump user.name;
}
