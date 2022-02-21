#pragma once

#pragma comment(lib, "ws2_32.lib")
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include "ws2tcpip.h"
#include "Packet.h"
#include <thread>
#include <mutex>

#define ACKNOWLEDGE_STRING "AKN"
#define FILE_ACKNOWLEDGE "AKNFILE"
#define FILE_NOT_ACKNOWLEDGE "NOTAKNFILE"
#define DEFAULT_TIMEOUT 0.3 //default single packet timeout in seconds
#define SPEEDCAP 1000 //max number of packets to send per sencond (timed-out included)
#define CONNECTION_TIMEOUT 10000 //connection timeout in miliseconds
#define ROUNDTRIP_AVERAGE_COUNT 10 //number of packets roundtrip time to use to calculate timeout

class UDPHandler
{
private:
	SOCKET socketS;
	sockaddr_in addrDest; //target address

	int max_active_packets; //maximal number of packets waiting for acknowledgement
	int active_packets; //number of packets waiting for acknowledgement
	Packet *active; //array of active packets
	int ack_buffer_len; //size of acknowledgement buffer
	unsigned char* ack_buffer; //acknowledgement buffer

	bool all_packets_sent; //flag that stops listening thread
	bool stop_resending; //flag that stops resending thread

	double timeout; //time to wait before resending unacknowledged packet in seconds
	int roundtrip_counter;
	double *roundtrips;

	clock_t clock_last_sent;
	unsigned int minimal_clocks_wait;

	/**
	 * Recieve callback packets from receiver and proccess them.
	 * Doesn't stop until file is acknowledged, or connection times out.
	 * 
	 */
	void listen();

	/**
	 * Resend timed-out packets.
	 * Doesn't stop until file stop_resending flag is set
	 * 
	 */
	void auto_resend();

	/**
	 * Deletes packet with given num from active packets.
	 * 
	 * \param packet_num packet number of acknowledged packet
	 * \return true if given packet_num is in active packets, false otherwise
	 */
	bool confirm_packet(unsigned short packet_num);

	/**
	 * Resends packet with given num if it is in active packets.
	 * 
	 * \param packet_num
	 * \return 
	 */
	bool resend_packet(unsigned short packet_num);

	/**
	 * Resends all timed out active packets.
	 * 
	 * \param timeout number of seconds before resending the packet
	 * \return true if at least one packet was resent
	 */
	bool resend_timedout(double timeout);

	/**
	 * Sets packets time sent
	 * Sends referenced socket.
	 */
	void send_packet_socket(Packet& p);

	/** Updates timeout according to last 10 roundtrip times. */
	void update_timeout();

public:
	/**
	 * UDPHandler constructor.
	 * Initializes UDP protocol and creates socket, initializes properties for ARQ
	 * 
	 * \param target_ip IP adress of target computer
	 * \param target_port Port on target computer
	 * \param local_port Port on this computer
	 * \param max_active_packets maximal number of packets to wait for acknowledgement
	 */
	UDPHandler(PCWSTR target_ip, u_short target_port, u_short local_port, int max_active_packets);

	/**
	 * Closes socket.
	 */
	~UDPHandler();

	/**
	 * Sends the passed packet.
	 * 
	 * \param p packet to send
	 */
	bool send_packet(Packet p);

	/**
	 * Starts a new thread with listen method.
	 * 
	 * \return std::thread the newly created thread
	 */
	std::thread start_listening();

	/**
	 * Starts a new thread with auto_resend method.
	 * 
	 * \return std::thread the newly created thread
	 */
	std::thread start_resending();

	/**
	 * Sets the flag, which will stop listening and resending thread.
	 * 
	 */
	void everything_sent();

	/**
	 * Blocks current thread until all active packets are acknowledged or stop_resending flag is set.
	 * 
	 */
	void wait_for_all();
};

