#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "packet.h"

int main(int argc, char const *argv[])
{
    if (argc != 2)
    { // check arguments
        printf("Please use with: server port_number\n");
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    printf("server on: %i\n", port);
    // error check
    if (port <= 0)
    {
        printf("Incorrect range for port\n");
        exit(EXIT_FAILURE);
    }
    // declare socket: domain, type of socket, protocol
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // error check
    if (sockfd < 0)
    {
        printf("Error with creating socket\n");
        exit(EXIT_FAILURE);
    }

    // declare socket address structs
    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind socket file descriptor and address
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Error with Binding\n");
        exit(EXIT_FAILURE);
    }

    char buffer[MESSAGE_SIZE] = {0}; // size chosen at random, longest valid message is 3 chars anyways, equal to client buffer

    socklen_t client_length = sizeof(client_addr);

    // receive from client
    int len_msg;
    int sent;
    Packet out;
    out.total_frag = 1;
    out.frag_no = 1;
    char data[DATA_SIZE]; //for holding extracted data
    FILE * file = NULL;
    
    //check for FTP first
    len_msg = recvfrom(sockfd, (char *)buffer, MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr, &client_length);

    // error check
    if (len_msg < 0)
    {
        printf("Error with recvfrom server\n");
        exit(EXIT_FAILURE);
    }

    buffer[len_msg] = '\0'; // set null char


    // send message back
    if (strcmp(buffer, "ftp") == 0)
    {
        sent = sendto(sockfd, "yes", strlen("yes"), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    }
    else
    {
        sent = sendto(sockfd, "no", strlen("no"), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    }
    // error check
    if (sent < 0)
    {
        printf("Error with sendto server\n");
        exit(EXIT_FAILURE);
    }
    printf("A file transfer can start\n");
    do
    {
        memset(buffer, 0, MESSAGE_SIZE);
        memset(data, 0, DATA_SIZE);
        len_msg = recvfrom(sockfd, (char *)buffer, MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr, &client_length);
        // error check
        if (len_msg < 0)
        {
            printf("Error with recvfrom server\n");
            exit(EXIT_FAILURE);
        }

        unpack(data,buffer, &out); //frag 1 corresponds to element 0
        //also sets frag_no
    if(out.frag_no == 1){
            file = fopen(out.filename,"w");
        } else{
            file = fopen(out.filename,"a+");
        }
        
        if(file != NULL){
            fwrite(data, sizeof(char), out.size, file);
        }

        fclose(file);

        // send message back
        sent = sendto(sockfd, "ACK", strlen("ACK"), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
        // error check
        if (sent < 0)
        {
            printf("Error with sendto server\n");
            exit(EXIT_FAILURE);
        }else{
            printf("Server: sent ACK for fragment %i/%i\n\n",out.frag_no,out.total_frag);
        }

    }while(out.frag_no < out.total_frag);
    // close
    close(sockfd);
    return EXIT_SUCCESS;
}
