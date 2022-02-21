#include "Packet.h"
#include <string.h>
#include <WinSock2.h>

extern "C" {
#include "crc16speed.h"
}

//initialize packet num to zero
unsigned short Packet::static_packet_num = 0;

Packet::Packet(unsigned int max_lenght)
{
	capacity = max_lenght;
	buffer = (char*)malloc(max_lenght);
	if (!buffer) {
		printf("Allocation error\n");
		exit(-2);
	}
	size = 0;
	crc = 0;
	packet_num = static_packet_num++;
	time_sent = 0;
	active = true;
	resent = false;

	//add packet number to the start
	const unsigned short packet_num_big_endian = htons(packet_num);
	append((char*)&packet_num_big_endian, sizeof(packet_num));
}

Packet::~Packet() {
	
}

bool Packet::append(const char* data, const unsigned int len)
{
	//check if there's room for the data
	if (len > free_capacity()) {
		return false;
	}

	//copy data to packet buffer
	memcpy(&buffer[size], data, len);
	size += len;

	return true;
}

unsigned int Packet::free_capacity()
{
	return capacity - size - sizeof(crc);
}

void Packet::set_time_sent()
{
	time_sent = clock();
}

bool Packet::is_timed_out(double timeout)
{
	const double timespan = (clock() - time_sent) / (double)CLOCKS_PER_SEC;
	return timespan > timeout;
}

bool Packet::is_active() {
	return active;
}

void Packet::acknowledge()
{	
	active = false;
	free(buffer);
	buffer = nullptr;
		
}

const char* Packet::get_buffer()
{
	crc = crc16(0, buffer, size);
	*(uint16_t*)&buffer[size] = crc;
	return buffer;
}

unsigned int Packet::get_size()
{
	return size + sizeof(crc);
}

unsigned short Packet::get_packet_num()
{
	return packet_num;
}

double Packet::get_ping()
{
	return (clock() - time_sent) / (double)CLOCKS_PER_SEC;
}

bool Packet::get_resent()
{
	return resent;
}

void Packet::set_resent()
{
	resent = true;
}
