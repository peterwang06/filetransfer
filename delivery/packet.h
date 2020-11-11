#include <stdio.h>
#include <string.h> 
#include <stdlib.h>

#define DATA_SIZE 1000
#define RECEIVE_SIZE 100
#define FILENAME_SIZE 100
#define MESSAGE_SIZE DATA_SIZE + 500

typedef struct Packet {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char *filename;
	char filedata[DATA_SIZE]; 
} Packet;

// Converts a Packet into a string formatted acoording to lab handout
int packetToString(Packet* packet, char* outputString)
{
	int chars_written = snprintf(outputString, MESSAGE_SIZE, "%d:%d:%d:%s:", packet->total_frag, packet->frag_no, packet->size, packet->filename);
    memcpy(outputString + chars_written, packet->filedata, packet->size);
    return chars_written + packet->size;
}