const AF_INET = 2;
const SOCK_STREAM = 1;
const LOCALHOST = 2130706433;

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

func openSocket(mut socket: Socket) {
	print("Open socket\n");
	let res = syscall(41, socket.domain, socket.type, socket.protocol);

	if res < 0 {
		print("Open socket failed.\n");
		syscall(60, 1);
	}

	socket.sockfd = res;
}

func send(sockfd: i32, buf: i8[], len: i32, flags: i32) {
	let bytesSend = syscall(44, sockfd, buf, len, flags, 0, 0);

	if bytesSend < 0 {
		print("Send failed\n");
		syscall(60, 1);
	}

	//print("Message sent to server\n");
}

func readAndSend(sockfd: i32, flags: i32) {
	let buf = i8[1024];
	let charsRead = syscall(0, 0, buf, 1023);

	send(sockfd, buf, charsRead, flags);
}

func read(sockfd: i32, buf: array, bufSize: i32) {
	let err = syscall(0, sockfd, buf, bufSize-1);

	if err < 0 {
		print("Read failed.\n");
		syscall(60, 1);
	}

	syscall(1, 1, buf[0], buf.len);
}

func connect(sockfd: i32, addr: SocketAddressIn, sockLen: i32 ) {
	print("Connecting:\n");
	let err = syscall(42, sockfd, addr, sockLen);
	if err < 0 {
		print("Connecting failed\n");
		syscall(60, 1);
	}

	print("Connected!\n");
}

func main() {
	let socket = Socket { sockfd: 0, domain: AF_INET, type: SOCK_STREAM, protocol: 0, backlog : 3 };

	openSocket(socket);

	let address = SocketAddressIn { family: AF_INET, port: 36895, addr: 0 };
	connect(socket.sockfd, address, 16);
	
	//send(socket.sockfd, "CLIENT: Hello from client\n", 28, 0);

	readAndSend(socket.sockfd, 0);

	let buf = i8[1024];
	read(socket.sockfd, buf, buf.len);
}
