#include <stdio.h>
#include <string.h> 
#include <stdlib.h>

#define DATA_SIZE 1000
#define RECEIVE_SIZE 100
#define FILENAME_SIZE 100
#define MESSAGE_SIZE DATA_SIZE + 500
#define MAX_FRAGS 100

typedef struct Packet {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char *filename;
	char filedata[DATA_SIZE]; 
} Packet;

void unpack(char output[], char input[], Packet* returnPacket) {
    char* token[4];

    token[0] = strtok(input, ":");
    token[1] = strtok(NULL, ":");
    token[2] = strtok(NULL, ":");
    token[3] = strtok(NULL, ":");
    int len = strlen(token[3])+1;
    returnPacket->total_frag = atoi(token[0]);
    returnPacket->frag_no = atoi(token[1]);
    returnPacket->size = atoi(token[2]);
    returnPacket->filename = token[3];
    printf("Received frag_no %i/%i\n",returnPacket->frag_no,returnPacket->total_frag);
    printf("%i:%i:%i:%s\n",returnPacket->total_frag,returnPacket->frag_no,returnPacket->size,returnPacket->filename);
    memcpy(output, (token[3]+len), returnPacket->size);
    return;
}

