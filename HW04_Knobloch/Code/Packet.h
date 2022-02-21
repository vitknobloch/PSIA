#pragma once
#include <ctime>

class Packet
{
private:
	char* buffer; //actual bytes of packet
	unsigned int capacity; //maximal size of packet
	unsigned int size; //current size of packet
	unsigned short crc; //crc sum of the packet
	unsigned short packet_num; //number of packet (used for acknowledgements)
	std::clock_t time_sent; //clock tick of sending
	bool active; //true if packet wasn't acknowledged yet
	bool resent; //true if packet was resent at least once.

	static unsigned short static_packet_num;

public:
	/**
	 * Packet constructor.
	 * 
	 * \param max_lenght maximal size of packet
	 */
	Packet(unsigned int max_lenght);
	~Packet();
	
	/**
	 * Add data to packet if there is room for it.
	 * 
	 * \param data pointer to start of the data area to append
	 * \param len number of bytes to copy from data to packet
	 * \return true in case of success, false otherwise (data don't fit inside packet)
	 */
	bool append(const char *data, const unsigned int len);

	/**
	 * Calculate free capacity of packet.
	 * 
	 * \return unsigned int - number of bytes left free in packet
	 */
	unsigned int free_capacity();

	/**
	 * Sets the packets time sent to current time.
	 * 
	 */
	void set_time_sent();

	/**
	 * Returns true if packet timed out.
	 * 
	 * \param timeout - timeout duration in seconds
	 * \return bool - true if packet is timed out, false otherwise
	 */
	bool is_timed_out(double timeout);

	/**
	 * Active getter, returns true, if buffer is initialized.
	 */
	bool is_active();

	/**
	 * Marks the packet as acknowledged frees allocated memory.
	 * 
	 */
	void acknowledge();

	/**
	 * Buffer getter.
	 * 
	 * \return const char* - pointer to the packets buffer
	 */
	const char* get_buffer();

	/**
	 * Size getter.
	 * 
	 * \return unsigned int - current size of the packet
	 */
	unsigned int get_size();

	/**
	 * Packet number getter.
	 * 
	 * \return unsigned short - this packets number
	 */
	unsigned short get_packet_num();

	/**
	 * Returns time elapsed between sending the packet and current time.
	 * 
	 * \return double - time elapsed since sending in seconds
	 */
	double get_ping();

	/** Sets resent property to true. */
	void set_resent();

	/**
	 * Resent getter.
	 * \return boolean - true if Packet has already been sent at least twice.
	 */
	bool get_resent();

};

