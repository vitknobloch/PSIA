#pragma once
#include <cstdio>

class FileHandler
{
private:

	FILE* file; //file stream
	unsigned int filesize; //filesize in bytes
	const char* fullname; //filename including path
	const char* name; //filename without path
	char* md5sum;

	/**
	 * Returns pointer to the beginning of pure filename (without path) within the fullname string.
	 * 
	 * \param fullname string with filename including path
	 * \return char* filename without path
	 */
	const char* find_short_name(const char* fullname);

public:
	/**
	 * FileHandler constructor.
	 * 
	 * \param filename name of the file we want to open (including path if necessary)
	 */
	FileHandler(const char* filename);
	~FileHandler();

	/**
	 * Short name getter.
	 * 
	 * \return char* - name of the opened file without path
	 */
	const char* get_name();

	/**
	 * Filesize getter.
	 * 
	 * \return unsigned int - filesize of the opened file in Bytes
	 */
	unsigned int get_filesize();

	/**
	 * Load num_bytes yet unread bytes from the file, store them in buffer
	 * and return the buffer.
	 * 
	 * \param num_bytes size of the buffer and maximal number of bytes to load from file
	 * \return char* - buffer with data loaded from file
	 */
	char* get_next(unsigned int num_bytes);

	/**
	 * Returns either max_size or number of bytes left unread in the file,
	 * whichever is bigger.
	 * 
	 * \param max_size maximal return value
	 * \return unsigned int - size of the next area to be loaded without overflowing filesize
	 */
	unsigned int get_next_size(unsigned int max_size);

	/**
	 * Load num_bytes bytes starting at the start_pos byte of the file, store them in a buffer
	 * and return the buffer.
	 * 
	 * \param start_pos index of the first byte to be read
	 * \param num_bytes size of the buffer and maximal number of bytes to load from file
	 * \return char* - buffer with data loaded from file
	 */
	char* get_on_pos(unsigned int start_pos, unsigned int num_bytes);

	/**
	 * Get position of the last read part of file.
	 * 
	 * \return unsigned int - position of next byte to be read
	 */
	unsigned int get_cur_pos();

	/**
	 * Returns 128bit hash sum of the opened file.
	 * 
	 * \return char* - 16byte hash sum of file
	 */
	char* get_hash_sum();


	/**
	 * Calculates 128bit hash sum of the opened file.
	 *
	 * \return char* - 16byte hash sum of file
	 */
	void calculate_hash_sum();
};

