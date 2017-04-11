#ifndef MS1_LIBRARY_
#define MS1_LIBRARY
#define _CRT_SECURE_NO_WARNINGS
#include "../header/library.h"

char message_buffer[128] = "";
bool buffer_full = false;
int senders_socket = 0;

//starts all necessary Windwos dlls
void winsock::start_DLLS() {
	if ((WSAStartup(MAKEWORD(this->version_num1, this->version_num2), &this->wsa_data)) != 0) {
		std::cout << "Could not start DLLs" << std::endl;
		std::cin.get();
		exit(0);
	}
}

//initializes a socket and returns it
SOCKET winsock::initialize_tcp_socket() {
	SOCKET LocalSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (LocalSocket == INVALID_SOCKET) {
		WSACleanup();
		std::cout << "Could not initialize socket" << std::endl;
		std::cin.get();
		exit(0);
	}
	return LocalSocket;
}

//base constructor (called for all objects)
//note that in case multiple socket objects are created, start_DLLS()
//is called multiple times. This, however, does not result in errors
//just a slight redundancy
winsock::winsock() {
	this->version_num1 = 2;
	this->version_num2 = 2;
	this->start_DLLS();
}

//receives messages from the client_socket
char * winsock_client::receive_message() {
	memset(this->rx_buffer, 0, 128);
	recv(this->client_socket, this->rx_buffer, sizeof(this->rx_buffer), 0);
	return this->rx_buffer;
}

//sends messages to the client_socket
void winsock_client::send_message(char * tx_buffer) {
	send(this->client_socket, tx_buffer, strlen(tx_buffer), 0);
}

//connects to a tcp_server, exits in case the server is unavailable
void winsock_client::connect_to_tcp_server() {
	struct sockaddr_in SvrAddr;
	SvrAddr.sin_family = AF_INET; //Address family type internet
	SvrAddr.sin_port = htons(this->port); //port (host to network conversion)
	SvrAddr.sin_addr.s_addr = inet_addr(this->ip.c_str()); //IP address
	if ((connect(this->client_socket, (struct sockaddr *)&SvrAddr, sizeof(SvrAddr))) == SOCKET_ERROR) {
		closesocket(this->client_socket);
		WSACleanup();
		std::cout << "Could not connect to the server" << std::endl;
		std::cin.get();
		exit(0);
	}
}

//connects to a tcp_server, keeps trying at 200ms intervals in case
// the server is unavailable
void winsock_client::connect_to_tcp_server_loop() {
	std::cout << "Trying to connect to the server" << std::endl;

	bool connected = false;
	struct sockaddr_in SvrAddr;
	SvrAddr.sin_family = AF_INET; //Address family type internet
	SvrAddr.sin_port = htons(this->port); //port (host to network conversion)
	SvrAddr.sin_addr.s_addr = inet_addr(this->ip.c_str()); //IP address
	while (!connected) {
		if ((connect(this->client_socket, (struct sockaddr *)&SvrAddr, sizeof(SvrAddr))) == SOCKET_ERROR) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
		else {
			std::cout << "Connection Established\n" << std::endl;
			connected = true;
		}
	}
}

//client constructor that sets up the port number and ip number
//it also initializes the socket
winsock_client::winsock_client(int port, std::string ip) {
	this->port = port;
	this->ip = ip;
	this->client_socket = this->initialize_tcp_socket();
}

//client socket destructor that closes the client_socket
winsock_client::~winsock_client() {
	closesocket(this->client_socket); //closes client socket
}

void winsock_client::get_messages() {
	char incomming_message[128] = "";
	while (true) {
		memset(incomming_message, 0, 128);
		strcpy(incomming_message, this->receive_message());
		for (int i = 0; i < 3; i++) {
			std::cout << "\n";
		}
		std::cout << "Message received: " << incomming_message << std::endl;
	}
}

PktDef::PktDef() {
	cmdPacket.header.pktCount = (char) 0;
	cmdPacket.header.drive = (char) 0;
	cmdPacket.header.status = (char)0;
	cmdPacket.header.sleep = (char)0;
	cmdPacket.header.arm = (char)0;
	cmdPacket.header.claw = (char)0;
	cmdPacket.header.ack = (char)0;
	cmdPacket.header.length = HEADERSIZE + sizeof(cmdPacket.CRC);
	cmdPacket.data = nullptr;
	cmdPacket.CRC = (char)0;
}

PktDef::PktDef(char* rawDataBuffer) {
	//Constructor that takes Raw data Buffer
	//Reconstruct PktDef via data from rawDataBuffer
	char* ptr = rawDataBuffer;
	
	memcpy(&cmdPacket.header, ptr, HEADERSIZE);

	ptr += HEADERSIZE;		//2 extra bytes comes from windows pad bytes

	if (cmdPacket.header.length < 9) {		//Checking for Motorbody data
		// No motorbody, initialize to nullptr
		cmdPacket.data = nullptr;
	}
	else {
		//Motorbody exists
		char motorbodySize = cmdPacket.header.length
							- HEADERSIZE
							- sizeof(cmdPacket.CRC);

		setBodyData(ptr, motorbodySize);
		ptr += motorbodySize;	//Advance ptr past MotorBody
	}
	cmdPacket.CRC = *ptr;
}

void PktDef::setCmd(CmdType type) {
	switch (type) {
	case DRIVE:
		cmdPacket.header.drive = 1;
		break;
  case STATUS:
    cmdPacket.header.status = 1;
    break;
	case SLEEP:
		cmdPacket.header.sleep = 1;
		break;
	case ARM:
		cmdPacket.header.arm = 1;
		break;
	case CLAW:
		cmdPacket.header.claw = 1;
		break;
	}
}

void PktDef::setBodyData(char* rawDataBuffer, int bufferByteSize) {
	cmdPacket.data = new char[bufferByteSize];
	memcpy(cmdPacket.data, rawDataBuffer, bufferByteSize);

	cmdPacket.header.length = HEADERSIZE + sizeof(cmdPacket.CRC) + bufferByteSize;
}

void PktDef::setPktCount(int countNumber) {
	cmdPacket.header.pktCount = countNumber;
}

CmdType PktDef::getCmd() {
	if (cmdPacket.header.drive) {
		return DRIVE;
	}
  else if (cmdPacket.header.status) {
    return STATUS;
  }
	else if (cmdPacket.header.sleep) {
		return SLEEP;
	}
	else if (cmdPacket.header.arm) {
		return ARM;
	}
	else if (cmdPacket.header.claw) {
		return CLAW;
	}
}

bool PktDef::getAck() {
	return cmdPacket.header.ack;
}

int PktDef::getLength() {
	return cmdPacket.header.length;
}

char* PktDef::getBodyData() {
	return cmdPacket.data;
}

int PktDef::getPktCount() {
	return cmdPacket.header.pktCount;
}

bool PktDef::checkCRC(char* ptr, int bufferSize) {
	int crc = 0;
	for (int i = 0; i < bufferSize; i++) {
		for (int j = 0; j < 8; j++) {		//iterate through 8 bits
			if ((*ptr >> j) & 1) {
				crc++;
			}
		}
		ptr++;
	}
	return crc == cmdPacket.CRC;
}

void PktDef::calcCRC() {
	//Calculates the CRC on the fly and saves it to the current object
	cmdPacket.CRC = 0;
	char* ptr = reinterpret_cast<char*>(&cmdPacket.header);

	for (int i = 0; i < HEADERSIZE; i++) {
		for (int j = 0; j < 8; j++) { //Loop through 8 bits
			if ((*ptr >> j) & 1) {
				cmdPacket.CRC++;
			}
		}
		ptr++;
	}

	if (cmdPacket.data != nullptr) {
		ptr = cmdPacket.data;
		int bodySize = cmdPacket.header.length - HEADERSIZE - sizeof(cmdPacket.CRC);

		for (int i = 0; i < bodySize; i++) {
			for (int j = 0; j < 8; j++) { //Loop through 8 bits
				if ((*ptr >> j) & 1) {
					cmdPacket.CRC++;
				}
			}
			ptr++;
		}
	}
}

char* PktDef::genPacket(){
	//Creates a RawBuffer in the heap and serialize data
	//Return address to RawBuffer
	
	int bufferHeader = 0;	//Buffer header location tracker
	int bodySize = this->getLength() - HEADERSIZE - sizeof(cmdPacket.CRC);
	char* rawBuffer = new char[cmdPacket.header.length];

	char* ptr = reinterpret_cast<char*> (&cmdPacket.header);
	memcpy(rawBuffer, ptr, HEADERSIZE);

	bufferHeader = HEADERSIZE;

	if (cmdPacket.data != nullptr) {		//Motorbody is not empty
		ptr = cmdPacket.data;
		memcpy(rawBuffer + bufferHeader, ptr, bodySize);
		bufferHeader += bodySize;
	}

	ptr = reinterpret_cast<char*>(&cmdPacket.CRC);
	memcpy(rawBuffer + bufferHeader, ptr, sizeof(cmdPacket.CRC));

	return std::ref(rawBuffer);
}
#endif
