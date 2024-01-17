const AF_INET = 2;
const SOCK_STREAM = 1;
const INADDR_ANY = 0;
const SOL_SOCKET = 1;
const SO_REUSEADDR = 2;
const SO_REUSEPORT = 15;

struct SocketOptions {
	opt: i32;
}

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

struct HttpRequest {
	method: string;
	path: string;
	protocol: string;
}

struct StringReader {
	i: i32;
	str: i8[];
}

func add(str: string, ch: i8) {
	&str.chars[str.len] = ch;
	str.len = str.len + 1;
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

func isNotWhiteSpace(char: i8): bool {
	let res = char != '\r' && char != ' ' && char != '\n';
	return res;
}

func isWhiteSpace(char: i8): bool {
	let res = char == '\r' || char == ' ' || char == '\n';
	return res;
}

func skipWhiteSpace(reader: StringReader) {
	while reader.str[reader.i].isWhiteSpace() {
		reader.i = reader.i + 1;
	}
}

func readWord(reader: StringReader): string {
	let res = "";
	while reader.str[reader.i].isNotWhiteSpace() {
		res.add(reader.str[reader.i]);
		reader.i = reader.i + 1;
	}

	return res;
}

func parseHttpRequest(request: i8[]): HttpRequest {
	let reader = StringReader { i: 0, str: request };

	let method = readWord(reader);
	skipWhiteSpace(reader);

	let path = readWord(reader);
	skipWhiteSpace(reader);

	let protocol = readWord(reader);

	print(method);
	print(path);
	print(protocol);

	let req = HttpRequest { method: method, path: path, protocol: protocol };

	return req;
}

func read(sockfd: i32, buf: array, bufSize: i32): HttpRequest {
	let bytesRead = syscall(0, sockfd, buf[0], bufSize-1);

	if bytesRead < 0 {
		print("Read failed.\n");
		syscall(60, 1);
	}

	buf.len = bytesRead;
	let req = parseHttpRequest(buf);

	return req;
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

func setSocketOptions(sockfd: i32) {
	print("setSocketOptions\n");
	let opt = SocketOptions { opt: 1 } ;
	let err = syscall(54, sockfd, SOL_SOCKET, SO_REUSEADDR or SO_REUSEPORT, &opt.opt, 4);

	if err < 0 {
		print("setSocketOptions failed.\n");
		syscall(60,1);
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

	setSocketOptions(socket.sockfd);

	let address = SocketAddressIn { family: AF_INET, port: 36895, addr: INADDR_ANY };
	bind(socket.sockfd, address);

	listen(socket.sockfd, socket.backlog);

	let clientAddrSize = ClientSize { size: 16 };
	let client = ClientSocket { sockfd: 0, addrSize: clientAddrSize };
	let clientAddress = SocketAddressIn { family: 2, port: 36895, addr: 0 };
	while 1 == 1 {
		let buf = i8[2048];
		let mut i: i32 = 0;
		while i < buf.len {
			buf[i] = 0;
			i = i + 1;
		}

		accept(client, socket.sockfd, clientAddress);

		let req = read(client.sockfd, buf, buf.len);

		send(client.sockfd, req.protocol, 0);
		send(client.sockfd, " 200\r\n", 0);

		closeSocket(client.sockfd);
	}
}
