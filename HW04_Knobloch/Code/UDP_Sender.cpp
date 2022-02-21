// UDP_Sender.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <thread>
#include "Packet.h"
#include "UDPHandler.h"
#include "FileHandler.h"

#define TARGET_IP   L"127.0.0.1"
#define TARGET_PORT 14000
#define LOCAL_PORT  14001

#define MAX_PACKET_LEN 1024
#define MAX_ACTIVE_PACKETS 15
#define START_STRING "START"
#define END_STRING "END"

int main(int argc, char* argv[])
{
    //read parameter or get file from input
    const char* filename;
    std::string line;
    if (argc > 1) {
        filename = argv[1];
    }
    else {
        std::cout << "Enter path and name of file to send:" << std::endl;        
        std::getline(std::cin, line);
        filename = line.c_str();
    }

    //Open file
    printf("Opening file.\n");
    FileHandler fh = FileHandler(filename);

    //Initialize UDP
    printf("Creating socket.\n");
    UDPHandler udph = UDPHandler(TARGET_IP, TARGET_PORT, LOCAL_PORT, MAX_ACTIVE_PACKETS);
    bool error = false;
    //start listening for acknowledgements and resending timed-out packets
    std::thread resend_thread = udph.start_resending();
    std::thread listen_thread = udph.start_listening();

    //Send header packet
    printf("Sending file.\n");
    Packet startPacket = Packet(MAX_PACKET_LEN);
    startPacket.append("START", strlen(START_STRING));
    const unsigned int filesize_big_endian = htonl(fh.get_filesize());
    startPacket.append((char*)&filesize_big_endian, sizeof(unsigned int));
    startPacket.append(fh.get_name(), strlen(fh.get_name())+1);
    udph.send_packet(startPacket);
    //wait for acknowledgment of initial packet
    udph.wait_for_all();

    clock_t start = clock();

    //Send file packets
    const unsigned int max_payload_len = MAX_PACKET_LEN - sizeof(unsigned int) - 2*sizeof(unsigned short);
    unsigned int next_size = fh.get_next_size(max_payload_len);
    while (next_size > 0U) {
        Packet p = Packet(MAX_PACKET_LEN);
        const unsigned int pos = htonl(fh.get_cur_pos());
        p.append((char*)&pos, sizeof(unsigned int));    
        char* payload = fh.get_next(next_size);
        p.append(payload, next_size);

        delete[] payload;
        
        if (!udph.send_packet(p)) {
            error = true;
            break;
        }
        next_size = fh.get_next_size(max_payload_len);        
    }

    clock_t end = clock();

    if (!error) {
        //wait for acknowledgement of all data packets
        udph.wait_for_all();
        //Send terminating packet
        printf("Sending termination packet and checksum.\n");
        Packet endPacket = Packet(MAX_PACKET_LEN);
        endPacket.append(END_STRING, strlen(END_STRING));
        endPacket.append(fh.get_hash_sum(), 16);
        udph.send_packet(endPacket);
        //wait for acknowledgement of terminating packet
        udph.wait_for_all();
    }    

    //end listening and resending threads
    udph.everything_sent();
    listen_thread.join();
    resend_thread.join();

    clock_t len = end - start;
    double time = len / (double)CLOCKS_PER_SEC;
    printf("The file was sent with the speed of %f kB/s\n", (fh.get_filesize() / 1024) / time);

    printf("Press any key to end program...\n");
    getchar();
}
