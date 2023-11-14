const AF_INET = 2;
const SOCK_STREAM = 1;
const LOCALHOST = 2130706433;

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

func openSocket(socket: Socket) {
	print("Open socket\n", 12);
	let res = syscall(41, socket.domain, socket.type, socket.protocol);

	if res < 0 {
		print("Open socket failed.\n", 20);
		syscall(60, 1);
	}

	socket.sockfd = res;
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
}

func connect(sockfd: int, addr: SocketAddressIn, sockLen: int) {
	print("Connecting:\n", 11);
	let err = syscall(42, sockfd, addr, sockLen);
	if err < 0 {
		print("Connecting failed\n", 18);
		syscall(60, 1);
	}

	print("Connected!\n", 18);
}

func main() {
	let socket = Socket { sockfd: 0, domain: AF_INET, type: SOCK_STREAM, protocol: 0, backlog : 3 };

	openSocket(socket);

	let address = SocketAddressIn { family: AF_INET, addr: LOCALHOST, port: 36895 };
	connect(socket.sockfd, address, 16);
	
	write(socket.sockfd, "Hello from client\n", 19);
	print("Message sent to server\n", 24);

	let buf = int[100];
	read(socket.sockfd, buf, buf.len);
	syscall(1, 1, buf, buf.len);
}
