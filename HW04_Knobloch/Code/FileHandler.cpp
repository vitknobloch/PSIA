#include "FileHandler.h"
#include "md5.h"
#include <stdlib.h>

#define MDSUM_CHUNKSIZE 1024

const char* FileHandler::find_short_name(const char* fullname)
{
	const char* ret = fullname;
	const char* ptr = fullname;
	while (*ptr != '\0') {
		if (*ptr == '/' || *ptr == '\\') {
			ret = ptr + 1;
		}
		ptr++;
	}
	return ret;
}

FileHandler::FileHandler(const char* filename)
{
	md5sum = nullptr;

	//open file
	file = nullptr;
	errno_t err = fopen_s(&file, filename, "rb");

	//save name and short name
	fullname = filename;
	name = find_short_name(filename);

	//save file size
	if (err == 0) {
		fseek(file, 0, SEEK_END);
		filesize = ftell(file);
		rewind(file);
	}
	//handle error (terminate program)
	else {
		filesize = 0;
		printf("Error opening file %s\n", filename);
		getchar();
		exit(100);
	}
	
}

FileHandler::~FileHandler()
{
	fclose(file);
}


const char* FileHandler::get_name()
{
	return name;
}

unsigned int FileHandler::get_filesize()
{
	return filesize;
}

char* FileHandler::get_next(unsigned int num_bytes)
{
	//check for end of file closer than num_bytes
	num_bytes = get_next_size(num_bytes);
	//init buffer
	char* ret = new char[num_bytes];	
	
	//read file
	int r = fread_s(ret, num_bytes, 1, num_bytes, file);

	return ret;
}

unsigned int FileHandler::get_next_size(unsigned int max_size)
{
	//get how much remains from the file
	const unsigned int remain = filesize - get_cur_pos();

	if (max_size < remain) {
		return max_size;
	}
	else {
		return remain;
	}
}

char* FileHandler::get_on_pos(unsigned int start_pos, unsigned int num_bytes)
{
	//save curent position and set cursor to start_pos
	unsigned int cur_pos = ftell(file);
	fseek(file, start_pos, SEEK_SET);

	//read using get_next
	char* ret = get_next(num_bytes);

	//reset cursor to previous position
	fseek(file, cur_pos, SEEK_SET);

	return ret;
}

unsigned int FileHandler::get_cur_pos()
{
	return ftell(file);
}

char* FileHandler::get_hash_sum()
{
	if (md5sum == nullptr) {
		calculate_hash_sum();
	}

	return md5sum;
}

void FileHandler::calculate_hash_sum()
{
	//save curent position and rewind
	unsigned int cur_pos = ftell(file);
	rewind(file);

	//calculate md5sum in chunks
	MD5 md_obj = MD5();
	unsigned int next_size;
	while ((next_size = get_next_size(MDSUM_CHUNKSIZE)) > 0) {
		const char* chunk = get_next(next_size);
		md_obj.update(chunk, next_size);
		delete[] chunk;
	}
	md_obj.finalize();

	md5sum = new char[16];
	memcpy_s(md5sum, 16, md_obj.get_digest(), 16);

	//reset cursor to previous position
	fseek(file, cur_pos, SEEK_SET);
}
