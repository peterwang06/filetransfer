#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include "packet.h"
#include <signal.h>
#include <setjmp.h>
#define timeout 2
sigjmp_buf buf;

void sig_handler(int signum){
    siglongjmp(buf, 1);
}  

// Sends message using send and checks for failure
int sendMessage(int sockfd, char* packet_string, int packet_length, struct sockaddr_in server_addr)
{
    int num_bytes_sent = sendto(sockfd, packet_string, packet_length, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (num_bytes_sent == -1)
    {
        printf("Error sending message %s to server\n", packet_string);
        exit(EXIT_FAILURE);
    }
    return num_bytes_sent;
}

// Receives message using recvfrom and checks for failure
int receiveMessage(int sockfd, char* buffer, int buffer_length, struct sockaddr_in server_addr)
{
    socklen_t server_addr_size = sizeof(server_addr);
    int num_bytes_received = recvfrom(sockfd, buffer, buffer_length, 0, (struct sockaddr *)&server_addr, &server_addr_size);
    if (num_bytes_received == -1)
    {
        printf("Client: Error receiving from server\n");
        exit(EXIT_FAILURE);
    }
    buffer[num_bytes_received] = '\0';  // Terminate c string with null char
    return num_bytes_received;
}

// Calculates the total number of fragments needed to transfer file given a file pointer
int getTotalFrag(FILE *fp)
{
    fseek(fp, 0, SEEK_END);         // move file pointer to the end
    int file_length = ftell(fp);    // calculate position
    int total_frag = (file_length/DATA_SIZE) + 1;
    rewind(fp);                     // move file pointer back to beginnning
    return total_frag;
}

// Sets the fields of a packet
void setPacketFields(Packet* packet, FILE* fp, int total_frag, int frag_no, char* filename)
{
    // Read data into packet.filedata
    fread((void*)packet->filedata, sizeof(char), DATA_SIZE, fp);
    // Update packet info
    packet->total_frag = total_frag;
    packet->frag_no = frag_no;
    strcpy(packet->filename, filename);
    packet->size = DATA_SIZE;
    // Last Packet might not be full
    if(frag_no == total_frag) {
        fseek(fp, 0, SEEK_END);
        packet->size = (ftell(fp) - 1) % 1000 + 1;
    }
}

// Transfering a file to server
void transfer(int sockfd, char* filename, struct sockaddr_in server_addr)
{
    printf("Starting to transfer\n");
    // Open file
    FILE *fp;
    if((fp = fopen(filename, "r")) == NULL) {
        printf("Input file %s cannot be read\n", filename);
        exit(EXIT_FAILURE);
    }

    // Get total fragment number
    int total_frag = getTotalFrag(fp);
    int frag_no = 1;    // Fragment number starts at 1

    printf("Total fragment: %d\n", total_frag);
    signal(SIGALRM, sig_handler);
    // Loop until all packets have been sent
    int j = 0;
    while(feof(fp) == 0 && frag_no <= total_frag){

        // Create Packet
        Packet* packet = (Packet*) malloc(sizeof(Packet));
        packet->filename = (char*) malloc((FILENAME_SIZE)*sizeof(char));
        memset(packet->filedata, 0, sizeof(char) * (DATA_SIZE));
        memset(packet->filename, 0, sizeof(char) * (FILENAME_SIZE));
        // Set packet fields
        setPacketFields(packet, fp, total_frag, frag_no, filename);
        // Convert packet to String
        char packet_string[MESSAGE_SIZE];
        int packet_length = packetToString(packet, packet_string);

        j = sigsetjmp(buf, 1);
        // Send packet (returns number of characters sent, but sizeof(char) = 1 byte)
        sendMessage(sockfd, packet_string, packet_length, server_addr);
        printf("sent frag_no: %i/%i\n", frag_no, total_frag);
        printf("%i:%i:%i:%s\n",packet->total_frag,packet->frag_no,packet->size,packet->filename);
        // Intialize buffer to receive ACK
        char ack_buf[RECEIVE_SIZE];
        memset(ack_buf, 0, sizeof(char) * RECEIVE_SIZE);

        

        alarm(timeout);
        // Wait for ACK
        receiveMessage(sockfd, ack_buf, RECEIVE_SIZE, server_addr);

        // check if "yes" was received
        if (strcmp(ack_buf, "ACK") == 0)
        printf("Client: ACK'ed fragment %d\n\n", frag_no);

        // Move on
        frag_no ++;
        // Free packet
        free(packet->filename);
        free(packet);
    }  
    printf("End of transfer\n");
}


int main(int argc, char const *argv[])
{
    // check number of arguments
    if (argc != 3)
    {
        printf("Client: Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[2]); // get port

    // check port
    if (port < 0)
    {
        printf("Client: Error with port\n");
        exit(EXIT_FAILURE);
    }

    char ftp_cmp[100];
    char filename[100];

    // get file name
    printf("Enter file with format <ftp file_name>: \n");
    scanf("%s %s", ftp_cmp, filename);

    // check ftp
    if (strcmp("ftp", ftp_cmp) != 0)
    {
        printf("Client: Input error\n");
        exit(EXIT_FAILURE);
    }

    // check existence of file
    if (access(filename, F_OK) == -1)
    {
        printf("Client: File doesn't exist\n");
        exit(EXIT_FAILURE);
    }

    int sockfd;
    // open datagram socket with UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
    {
        printf("Client: Error with socket\n");
        exit(EXIT_FAILURE);
    }

    // declare socket address
    struct sockaddr_in server_addr;
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_aton(argv[1], &server_addr.sin_addr) == 0)
    {
        printf("Client: Invalid server address\n");
        exit(EXIT_FAILURE);
    }

    sendMessage(sockfd, "ftp", strlen("ftp"), server_addr);

    char buffer[RECEIVE_SIZE] = {0};
    // receive message
    receiveMessage(sockfd, buffer, RECEIVE_SIZE, server_addr);

    // check if "yes" was received
    if (strcmp(buffer, "yes") == 0)
        printf("Client: A file transfer can start\n");

    // Transfer file
    transfer(sockfd, filename, server_addr);

    close(sockfd);
    return 0;
}