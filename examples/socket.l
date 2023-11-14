const AF_INET = 2;
const SOCK_STREAM = 1;
const INADDR_ANY = 0;

struct ClientSocket {
	sockfd: int;
	addrSize: int;
}

struct Socket {
	sockfd: int;
	domain: int;
	type: int;
	protocol: int;
	backlog: int;
}

struct SocketAddressIn {
	family: int;
	addr: int;
	port: int;
}

func print(str: string, len: int) {
	syscall(1, 1, str, len);
}

func closeSocket(sockfd: int) {
	syscall(3, sockfd);
}

func write(sockfd: int, str: string, len: int) {
	print("Write\n", 6);
	syscall(1, sockfd, str, len);
}

func read(sockfd: int, buf: array, bufSize: int) {
	print("Read\n", 5);
	let err = syscall(0, sockfd, buf, bufSize-1);

	if err < 0 {
		print("Read failed.\n", 13);
		syscall(60, 1);
	}

	syscall(1, 1, buf, buf.len);
}

func accept(client: ClientSocket, sockfd: int, addr: SocketAddressIn) {
	print("Accepting:\n", 11);
	client.sockfd = syscall(43, sockfd, addr, client.addrSize);
	if client.sockfd < 0 {
		print("Connecting failed\n", 18);
		syscall(60, 1);
	}

	print("Client connected!\n", 18);
}

func listen(sockfd: int, backlog: int) {
	print("Listen\n", 7);
	let err = syscall(50, sockfd, backlog);

	if err < 0 {
		print("Listen failed.\n", 15);
		syscall(60, 1);
	}
}

func bind(sockfd: int, addr: SocketAddressIn) {
	print("Bind\n", 5);
	let err = syscall(49, sockfd, addr, 16);

	if err < 0 {
		print("Bind socket failed.\n", 20);
		syscall(60, 1);
	}
}

func openSocket(socket: Socket) {
	print("Open socket\n", 12);
	let res = syscall(41, socket.domain, socket.type, socket.protocol);

	if res < 0 {
		print("Open socket failed.\n", 20);
		syscall(60, 1);
	}

	socket.sockfd = res;
}

func main() {
	let socket = Socket { sockfd: 0, domain: AF_INET, type: SOCK_STREAM, protocol: 0, backlog : 3 };

	openSocket(socket);

	let address = SocketAddressIn { family: AF_INET, addr: INADDR_ANY, port: 36895 };
	bind(socket.sockfd, address);

	listen(socket.sockfd, socket.backlog);

	let client = ClientSocket { sockfd: 0, addrSize: 16 };
	accept(client, socket.sockfd, address);

	let buf = int[100];
	read(client.sockfd, buf, buf.len);

	write(client.sockfd, "Connection accepted.\n", 22);
	
	closeSocket(client.sockfd);

	closeSocket(socket.sockfd);
}
