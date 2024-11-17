///////////////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                         //
// Date : 2024/05/08                                                         //
// OS : Ubuntu 20.04.6 LTS 64bits                                            //
//                                                                           //
// Author : Kang Hyun Min                                                    //
// Student ID : 2020202092                                                   //
// --------------------------------------------------------------------------//
// Title : System Programming Assignment #2-2 (FTP Client)                   //
// Description : This program connects to an FTP server, sends messages,     //
//               and receives responses from the server. The user can        //
//               enter messages to send to the server and receive responses. //
//               If the message is "QUIT", the program terminates.           //
// --------------------------------------------------------------------------//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define BUF_SIZE 256

///////////////////////////////////////////////////////////////////////
// Function: main                                                    //
// ================================================================= //
// Input: int argc -> The number of arguments of inputted string     //
//        char **argv -> Array of command-line arguments             //
// Output: int - Returns 0 on success, non-zero on failure           //
// ----------------------------------------------------------------- //
// Purpose: Connects to the server, sends messages from the user,    //
//          and receives responses from the server. If the message   //
//          is "QUIT", it closes the connection and terminates.      //
// ----------------------------------------------------------------- //
///////////////////////////////////////////////////////////////////////
int main(int argc, char **argv){

    //If IP address and Port number are not inputted
    if (argc != 3) { 
        write(1, "IP address and Port number required.\n", strlen("IP address and Port number required.\n"));
        return 1;
    }

    char buff[BUF_SIZE]; // Buffer for communication
    int n;
    int sockfd; // Socket file descriptor
    struct sockaddr_in serv_addr; // Server address structure

    // Create a socket for communication with the server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Initialize server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; // IPv4
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);  // Server IP address
    serv_addr.sin_port = htons(atoi(argv[2])); // Server port number

    // Connect to the server
    connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    // Loop to send and receive messages from the server
    while(1){
        write(1, "> ", 2);
        memset(buff, 0, BUF_SIZE); // Clear buffer
        read(0, buff, BUF_SIZE); // Read user input

        // Send message to the server
        if(write(sockfd, buff, BUF_SIZE) > 0){
            // Receive response from the server
            if(read(sockfd, buff, BUF_SIZE) > 0)
                printf("from server: %s", buff); // Print server's response
            else
                break; // If read fails, exit loop
        }
        else break; // If write fails, exit loop
    }
    close(sockfd); // Close the socket
    return 0;
}