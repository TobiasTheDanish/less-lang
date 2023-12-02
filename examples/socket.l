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

func print(str: string) {
	syscall(1, 1, str.chars[0], str.len);
}

func closeSocket(sockfd: i32) {
	syscall(3, sockfd);
}

func send(sockfd: i32, buf: string, flags: i32) {
	let bytesSend = syscall(44, sockfd, buf.chars[0], buf.len, flags, 0, 0);

	if bytesSend < 0 {
		print("Send failed\n");
		syscall(60, 1);
	}

	//print("Message sent to client\n");
}

func read(sockfd: i32, buf: array, bufSize: i32) {
	let bytesRead = syscall(0, sockfd, buf, bufSize-1);

	if bytesRead < 0 {
		print("Read failed.\n");
		syscall(60, 1);
	}

	print("CLIENT: ");
	syscall(1, 1, buf, bytesRead);
}

func accept(client: ClientSocket, sock: i32, addr: SocketAddressIn) {
	print("Accepting:\n");
	let res = syscall(43, sock, addr, client.addrSize);

	if res < 0 {
		print("Accepting failed\n");
		closeSocket(sock);
		syscall(60, 1);
	}

	client.sockfd = res;
	print("Client connected!\n");
}

func listen(sockfd: i32, backlog: i32) {
	print("Listen\n");
	let err = syscall(50, sockfd, backlog);

	if err < 0 {
		print("Listen failed.\n");
		syscall(60, 1);
	}
}

func bind(sockfd: i32, addr: SocketAddressIn) {
	print("Bind\n");
	let err = syscall(49, sockfd, addr, 16);

	if err < 0 {
		print("Bind socket failed.\n");
		syscall(60, 1);
	}
}

func openSocket(socket: Socket) {
	print("Open socket\n");
	let res = syscall(41, socket.domain, socket.type, socket.protocol);

	if res < 0 {
		print("Open socket failed.\n");
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
	while 1 == 1 {
		let buf = i8[1024];
		let mut i: i32 = 0;
		while i < buf.len {
			buf[i] = 0;
			i = i + 1;
		}

		accept(client, socket.sockfd, clientAddress);

		read(client.sockfd, buf, buf.len);

		send(client.sockfd, "SERVER: Message recieved, closing connection.\n", 0);

		closeSocket(client.sockfd);
	}
}
