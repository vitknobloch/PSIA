// PSIA_sender.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"

#include "file_loader.h"
#include "my_malloc.h"

#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 1024

#define SENDER


#ifdef SENDER
#define TARGET_PORT 5555
#define LOCAL_PORT 8888
#define HEADER_STRING "START"
#define TERMINATE_STRING "END"
#endif // SENDER


void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

//**********************************************************************
int main()
{
	file_t file = get_file();
	if (!file.size) {
		fprintf(stderr, "TERMINATING PROGRAM\n");
		return 100;
	}
		
	printf("file_size: %d\nfile_name: %s\n", file.size, file.name);

	SOCKET socketS;

	InitWinsock();

	struct sockaddr_in local;
	struct sockaddr_in from;

	int fromlen = sizeof(from);
	local.sin_family = AF_INET;
	local.sin_port = htons(LOCAL_PORT);
	local.sin_addr.s_addr = INADDR_ANY;


	socketS = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0) {
		printf("Binding error!\n");
		getchar(); //wait for press Enter
		return 1;
	}
	//**********************************************************************
	char buffer_rx[BUFFERS_LEN];
	char buffer_tx[BUFFERS_LEN];

#ifdef SENDER

	sockaddr_in addrDest;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);

	//SEND HEADER
	const char* header = HEADER_STRING;
	const int start_len = strlen(header);
	strcpy_s(buffer_tx, header);
	strcpy_s(&buffer_tx[start_len], sizeof(int), (char*)&file.size);
	const int name_len = strlen(file.name) + 1;
	strcpy_s(&buffer_tx[start_len + sizeof(int)], name_len, file.name);
	printf("Sending header. Awaiting confirmation...\n");
	sendto(socketS, buffer_tx, 9 + name_len, 0, (sockaddr*)&addrDest, sizeof(addrDest));

	//TODO: AWAIT CONFIRMATION
	getchar();
	printf("Confirmation recieved. Sending file.\n");

	//SEND FILE
	const char* buffer_tx_payload = buffer_tx + sizeof(int);
	const unsigned int payload_size = BUFFERS_LEN - sizeof(int);
	unsigned int cur_pos = 0;
	while(cur_pos < file.size){
		unsigned int load_size = payload_size > file.size - cur_pos ? file.size - cur_pos : payload_size;
		*(int*)buffer_tx = cur_pos;
		fread_s((void*)buffer_tx_payload, payload_size, load_size, 1, file.stream);
		printf("Sending packet.\n");
		sendto(socketS, buffer_tx, load_size + sizeof(int), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		cur_pos += payload_size;
	}

	//SEND TERMINATION
	const char* termination = TERMINATE_STRING;
	const int termination_len = strlen(termination);
	strcpy_s(buffer_tx, termination);
	printf("Sending termination.\n");
	sendto(socketS, buffer_tx, termination_len, 0, (sockaddr*)&addrDest, sizeof(addrDest));

	//CLEAN-UP
	fclose(file.stream);
	free_memory(file.name);

	closesocket(socketS);

#endif // SENDER

	getchar(); //wait for press Enter
	return 0;
}


