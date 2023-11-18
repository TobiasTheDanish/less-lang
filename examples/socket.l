const AF_INET = 2;
const SOCK_STREAM = 1;
const INADDR_ANY = 0;

struct ClientSize {
	size: i32;
}

struct ClientSocket {
	sockfd: i32;
	addrSize: ClientSize;
}

struct Socket {
	sockfd: i32;
	domain: i32;
	type: i32;
	protocol: i32;
	backlog: i32;
}

struct SocketAddressIn {
	family: i16;
	port: i16;
	addr: i32;
}

func print(str: string, len: i8) {
	syscall(1, 1, str, len);
}

func closeSocket(sockfd: i32) {
	syscall(3, sockfd);
}

func send(sockfd: i32, str: string, len: i32, flags: i32) {
	let bytesSend = syscall(44, sockfd, str, len, flags, 0, 0);

	if bytesSend < 0 {
		print("Send failed\n", 12);
		syscall(60, 1);
	}

	print("Message sent to client\n", 23);
}

func read(sockfd: i32, buf: array, bufSize: i32) {
	let err = syscall(0, sockfd, buf, bufSize-1);

	if err < 0 {
		print("Read failed.\n", 13);
		syscall(60, 1);
	}

	syscall(1, 1, buf, buf.len);
}

func accept(client: ClientSocket, sock: i32, addr: SocketAddressIn) {
	print("Accepting:\n", 11);
	let res = syscall(43, sock, addr, client.addrSize);

	if res < 0 {
		print("Accepting failed\n", 18);
		closeSocket(sock);
		syscall(60, 1);
	}

	client.sockfd = res;
	print("Client connected!\n", 18);
}

func listen(sockfd: i32, backlog: i32) {
	print("Listen\n", 7);
	let err = syscall(50, sockfd, backlog);

	if err < 0 {
		print("Listen failed.\n", 15);
		syscall(60, 1);
	}
}

func bind(sockfd: i32, addr: SocketAddressIn) {
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

	let address = SocketAddressIn { family: AF_INET, port: 36895, addr: INADDR_ANY };
	bind(socket.sockfd, address);

	listen(socket.sockfd, socket.backlog);

	let clientAddrSize = ClientSize { size: 16 };
	let client = ClientSocket { sockfd: 0, addrSize: clientAddrSize };
	let clientAddress = SocketAddressIn { family: 2, port: 36895, addr: 0 };
	accept(client, socket.sockfd, clientAddress);

	let buf = i8[1024];
	read(client.sockfd, buf, buf.len);

	send(client.sockfd, "SERVER: Connection accepted.\n", 31, 0);
	
	closeSocket(client.sockfd);

	closeSocket(socket.sockfd);
}
