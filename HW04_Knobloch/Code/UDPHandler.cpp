#include "UDPHandler.h"

extern "C" {
#include "crc16speed.h"
}

std::mutex mtx_active;
std::mutex mtx_socket;

//helper method to get if str1 startswith const_str
bool compare(char* str1, char* const_str) {
	while (*const_str != '\0') {
		if (*str1 != *const_str)
			return false;

		str1++; const_str++;
	}
	return true;
}

void UDPHandler::listen()
{
	struct sockaddr_in from;
	int addr_len = sizeof(from);
	bool sent = false;
	while (!sent) {
		int len = recvfrom(socketS, (char*)ack_buffer, ack_buffer_len, 0, (sockaddr*)&from, &addr_len);
		if (len == SOCKET_ERROR) { //if there's nocommunication in 10s trigger error
			if (all_packets_sent) {
				printf("FILE NOT ACKNOWLEDGED\nAll packets have been sent successfully, but file confirmation wasn't recieved,\nthe file was probably recieved well, but there's no guarantee.\n");
				sent = true;
				continue;
			}
			else {
				printf("Socket error. Stopped recieving acknowledgements.\n");
				break;
			}			
		}

		//recieved file acknowledgement
		if (compare((char*)ack_buffer, (char*)FILE_ACKNOWLEDGE)) {
			printf("FILE ACKNOWLEDGED\n");
			sent = true;
			continue;
		}

		//recieved negative file acknowledgement
		if (compare((char*)ack_buffer, (char*)FILE_NOT_ACKNOWLEDGE)) {
			printf("FILE NOT ACKNOWLEDGED\nSending failed, try again...\n");
			sent = true;
			continue;
		}

		//ckeck acknowledgements checksum
		short crc = crc16(0, ack_buffer, len - sizeof(short));
		if (!(crc == *(short*)&ack_buffer[len - sizeof(short)])) {
			printf("Recieved ACK has wrong checksum.\n");
			continue;
		}

		//the recieved packet is acknowledgement
		if (compare((char*)ack_buffer, (char*)ACKNOWLEDGE_STRING)) {
			int ack_len = sizeof(short) + 1; //size of one acknowledgement
			const int acks_recieved = (len - sizeof(short) - strlen(ACKNOWLEDGE_STRING)) / ack_len; //number of acks recieved
			//iterate through recieved acknowledgements
			int start_index = strlen(ACKNOWLEDGE_STRING);
			for (int i = 0; i < acks_recieved; i++) {
				//extract packet number
				unsigned short packet_num = *(unsigned short*)&ack_buffer[start_index + i * ack_len];
				packet_num = ntohs(packet_num);
				//extract ack byte
				bool ack_ok = *(&ack_buffer[sizeof(short) + start_index + i * ack_len]) == 0xff;
				if (ack_ok) {
					//confirm packet if acknowledge is OK
					mtx_active.lock();
					if (confirm_packet(packet_num));
					printf("Packet %3d ACK SUCCESS\n", packet_num);
					mtx_active.unlock();
				}
				else {
					//resend packet if acknowledge fails
					printf("Packet %3d ACK FAIL, resending...\n", packet_num);
					mtx_active.lock();
					resend_packet(packet_num);
					mtx_active.unlock();
				}

			}
			continue;
		}

		//the recieved packet has unknown prefix
		printf("Recieved unrecognized packet.\n");
	}
	stop_resending = true;
}

void UDPHandler::auto_resend() {
	while (!stop_resending) {
		mtx_active.lock();
		resend_timedout(DEFAULT_TIMEOUT);

		mtx_active.unlock();
	}
}

bool UDPHandler::confirm_packet(unsigned short packet_num)
{
	//iterate through active packets
	for (int i = 0; i < active_packets; i++) {
		//if packet num matches
		if (active[i].get_packet_num() == packet_num) {
			active[i].acknowledge();
			//update timeout
			if (!active[i].get_resent()) {
				roundtrips[roundtrip_counter++] = active[i].get_ping();
				if (roundtrip_counter >= ROUNDTRIP_AVERAGE_COUNT) {
					update_timeout();
					roundtrip_counter = 0;
				}
			}
			//delete packet from active packets
			active[i] = active[--active_packets];
			return true;
		}
	}
	return false;
}

bool UDPHandler::resend_packet(unsigned short packet_num)
{
	for (int i = 0; i < active_packets; i++) {
		if (active[i].get_packet_num() == packet_num) {
			active[i].set_resent();
			send_packet_socket(active[i]);
			return true;
		}
	}
	return false;
}

bool UDPHandler::resend_timedout(double timeout)
{
	bool ret = false;
	for (int i = 0; i < active_packets; i++) {
		if (active[i].is_timed_out(timeout)) {
			printf("Packet %3d timed out, resending...\n", active[i].get_packet_num());
			active[i].set_resent();
			send_packet_socket(active[i]);
			ret = true;
		}
	}
	return ret;
}

void UDPHandler::send_packet_socket(Packet& p)
{
	if (!p.is_active())
		return;
	mtx_socket.lock();
	while ((clock() - clock_last_sent) < minimal_clocks_wait) {
		continue;
	}
	clock_last_sent = clock();
	p.set_time_sent();
	sendto(socketS, p.get_buffer(), p.get_size(), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	mtx_socket.unlock();
}

void UDPHandler::update_timeout()
{
	double sum = 0;
	for (int i = 0; i < ROUNDTRIP_AVERAGE_COUNT; i++) {
		sum += roundtrips[i];
	}
	timeout = (sum / ROUNDTRIP_AVERAGE_COUNT) * 2;
	printf("Timeout updated: %.3f\n", timeout);
}

void UDPHandler::everything_sent()
{
	all_packets_sent = true;
}

void UDPHandler::wait_for_all()
{
	while (!stop_resending && active_packets > 0) {
		continue;
	}
}

UDPHandler::UDPHandler(PCWSTR target_ip, u_short target_port, u_short local_port, int max_active_packets)
{
	//initialize winsock
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	//add local address and port
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(local_port);
	local.sin_addr.s_addr = INADDR_ANY;

	//create socket
	socketS = socket(AF_INET, SOCK_DGRAM, 0);

	//set rcvfrom timeout
	DWORD read_timeout = CONNECTION_TIMEOUT;
	setsockopt(socketS, SOL_SOCKET, SO_RCVTIMEO, (char*)&read_timeout,
		sizeof read_timeout);

	//bind socket
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0) {
		printf("Binding error!\n");
		getchar();
		exit(-1);
	}

	//setup active packets, resending, listening
	this->active_packets = 0;
	this->max_active_packets = max_active_packets;
	this->active = (Packet*)malloc(sizeof(Packet) * max_active_packets);
	this->all_packets_sent = false;
	this->stop_resending = false;

	//setup timeout
	this->timeout = DEFAULT_TIMEOUT;
	this->clock_last_sent = 0;
	this->minimal_clocks_wait = CLOCKS_PER_SEC / SPEEDCAP;
	this->roundtrips = new double[ROUNDTRIP_AVERAGE_COUNT];
	this->roundtrip_counter = 0;

	//setup recieving buffer
	this->ack_buffer_len = strlen(ACKNOWLEDGE_STRING) +
		max_active_packets * (1 + sizeof(unsigned short)) + sizeof(unsigned short);
	this->ack_buffer = (unsigned char*)malloc(ack_buffer_len);

	if (!active || !ack_buffer) {
		printf("Malloc error\n");
		exit(-3);
	}

	//add target adress and port
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(target_port);
	InetPton(AF_INET, target_ip, &addrDest.sin_addr.s_addr);
}

UDPHandler::~UDPHandler()
{
	free(ack_buffer);
	free(active);
	delete this->roundtrips;
	closesocket(socketS);
}

bool UDPHandler::send_packet(Packet p)
{
	//block until there is room for another active packet
	while (active_packets >= max_active_packets) {
		if (stop_resending)
			return false;
	}
	send_packet_socket(p);
	mtx_active.lock();
	active[active_packets++] = p;
	mtx_active.unlock();
	return true;
}

std::thread UDPHandler::start_listening()
{
	return std::thread(&UDPHandler::listen, this);
}

std::thread UDPHandler::start_resending()
{
	return std::thread(&UDPHandler::auto_resend, this);
}
