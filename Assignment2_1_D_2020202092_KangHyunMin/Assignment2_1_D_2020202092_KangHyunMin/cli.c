///////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                 //
// Date : 2024/04/30                                                 //
// OS : Ubuntu 20.04.6 LTS 64bits                                    //
//                                                                   //
// Author : Kang Hyun Min                                            //
// Student ID : 2020202092                                           //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #2-1 ( ftp server )         //
// Description : This file contains the code for the FTP client      //
//               program that connects to an FTP server, sends       //
//               commands, and receives responses.                   //
///////////////////////////////////////////////////////////////////////
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>

// Buffer size for data communication
#define BUFFSIZE    4096

/////////////////////////////////////////////////////////////////////////
// Function: conv_cmd                                                  //
// ====================================================================//
// Input: const char* buff -> Original command from user input         //
//        char* cmd_buff -> Buffer to store the converted command      //
// Output: None                                                        //
// Purpose: Converts user command to FTP commands ("quit" to "QUIT",   //
//  "ls" to "NLST"). This conversion is used to communicate            //
// with the FTP server.                                                //
/////////////////////////////////////////////////////////////////////////
void conv_cmd(const char *buff, char *cmd_buff) {
    // Conver quit to FTP command 'QUIT'
    if (strncmp(buff, "quit", 4) == 0) {
        strcpy(cmd_buff, "QUIT");

        // If there are additional characters after "quit", append them
        if (strlen(buff) > 2) {
            strcat(cmd_buff, buff + 4);
        }
    } 
    // If the command is "ls", convert to "NLST"
    else if (strncmp(buff, "ls", 2) == 0) {
        strcpy(cmd_buff, "NLST"); 

        // Append any additional characters after "ls"
        if (strlen(buff) > 2) {
            strcat(cmd_buff, buff + 2); 
        }
    } 
    // Otherwise, copy the command and all argument
    else {
        strcpy(cmd_buff, buff);
    }
}

////////////////////////////////////////////////////////////////////////////
// Function: main                                                         //
// =======================================================================//
// Input: int argc -> Number of command-line arguments                    //
//        char* argv[] -> Array of command-line arguments                 //
// Output: int -> Return code (0 for success, non-zero for fail)          //
// Purpose: Main function for the client-side code. Connects to a server  //
// with given IP address and port number. It reads user input from        //
// standard input, converts commands, and sends them to the server.       //
// It also reads and displays server responses.                           //
////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]){

    //If IP address and Port number are not inputted
    if (argc != 3) { 
        write(1, "IP address and Port number required.\n", strlen("IP address and Port number required.\n"));
        return 1;
    }

    const char *ip_addr = argv[1]; //IP ADDRESS
    int port = atoi(argv[2]);  //PORT NUMBER

    int socket_fd, len;
    struct sockaddr_in server_addr;
    char buf[BUFFSIZE] ={}; // Buffer for reading and writing data
    char cmd_buff[BUFFSIZE] = {}; // Buffer for command conversion

    // Create a socket and check for errors
    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        write(1, "can't create socket.\n", strlen("can't create socket.\n"));
        return -1;
    }

    // Configure server address
    bzero (buf, sizeof(buf));
    bzero ((char*)&server_addr, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = inet_addr(ip_addr); // Convert IP address
    server_addr.sin_port = htons(port); // Convert port to network format
    
    // Connect to the server
    if(connect(socket_fd, (struct sockaddr*)&server_addr, sizeof (server_addr)) < 0) {
        write(1, "can't connect.\n", strlen("can't connect.\n"));
        return -1;
    }

    // Prompt for user input
    write(1, "> ", 2);

    // Read user input and process it
    while((len = read(0, buf, sizeof(buf))) > 0){
            buf[len] = '\0';

            // Convert the command to FTP command
            conv_cmd(buf, cmd_buff);

            // Send the converted command to the server
            if(write(socket_fd, cmd_buff, strlen(cmd_buff)) != strlen(cmd_buff)){
                write(1, "write to server error\n", strlen("write to server error\n"));
                return 0;
            }

            // Clear the buffer and read the server's response
            bzero(buf, sizeof(buf));
            if((len = read(socket_fd, buf, sizeof(buf))) > 0){
                // If server sends a quit message, exit
                if (strncmp(buf, "Program quit!!\n", 15) == 0) {
                    write(1, buf, strlen(buf));
                    exit(1); //exit
                }

                // Write the server's response to the console
                write(1, buf, strlen(buf));
                
                bzero(buf, sizeof(buf)); // Clear the buffer
    }   
        // Prompt for more input
        write(1, "> ", 2);
    }
    // Close the socket and exit
    close(socket_fd);
    return 0;

}