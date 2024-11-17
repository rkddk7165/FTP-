///////////////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                         //
// Date : 2024/05/16                                                         //
// OS : Ubuntu 20.04.6 LTS 64bits                                            //
//                                                                           //
// Author : Kang Hyun Min                                                    //
// Student ID : 2020202092                                                   //
// --------------------------------------------------------------------------//
// Title : System Programming Assignment #2-3 (FTP Client)                   //
// Description : This program connects to an FTP server, sends messages,     //
//               and receives responses from the server. The user can        //
//               enter messages to send to the server and receive responses. //
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

#define BUF_SIZE 4096

char send_buff[128];

/////////////////////////////////////////////////////////////////////////
// Function: conv_command                                              //
// ====================================================================//
// Input: const char* buff -> Original command from user input         //
//        char* cmd_buff -> Buffer to store the converted command      //
// Output: None                                                        //
// Purpose: Converts user command to FTP commands (ex: "quit" to "QUIT",//
// and "ls" to "NLST"). This conversion is used to communicate         //
// with the FTP server.                                                //
/////////////////////////////////////////////////////////////////////////
int conv_command(char* buff, char* send_buff);

///////////////////////////////////////////////////////////////////////
// Function: main                                                    //
// ================================================================= //
// Input: int argc -> The number of arguments of inputted string     //
//        char **argv -> Array of command-line arguments             //
// Output: int - Returns 0 on success, non-zero on failure           //
// ----------------------------------------------------------------- //
// Purpose: Connects to the server, sends messages from the user,    //
//          and convert user command to the FTP command. Also,       //
//          receives responses from the server and print the result. //
// ----------------------------------------------------------------- //
///////////////////////////////////////////////////////////////////////
int main(int argc, char **argv){

    //If IP address and Port number are not inputted
    if (argc != 3) { 
        write(1, "IP address and Port number required.\n", strlen("IP address and Port number required.\n"));
        return 1;
    }

    


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
        char buff[BUF_SIZE] = {0};
        
        read(0, buff, BUF_SIZE); // Read user input

        char send_buff[128] = {0};

        //Call conv_command function to convert user command to FTP command
        if (conv_command(buff, send_buff) == -1){
            printf("Unknown Command.\n");
            continue;
        }

        // Send message to the server
        if(write(sockfd, send_buff, strlen(send_buff)) > 0){
            char buff[BUF_SIZE] = {0};

            // Receive response from the server
            if(read(sockfd, buff, BUF_SIZE) > 0){
                printf("%s", buff); // Print server's response
            }
            else
                break; // If read fails, exit loop
        }
        else break; // If write fails, exit loop
    }
    close(sockfd); // Close the socket
    return 0;
}

// Function to convert user input to FTP command
int conv_command(char* buff, char* send_buff) {
    char *token = strtok(buff, " \n");  // Consider the first token as the command
    if (!token) return -1;

    // Convert the command to FTP command
    if (strcmp(token, "ls") == 0) {
        strcpy(send_buff, "NLST");
    } else if (strcmp(token, "dir") == 0) {
        strcpy(send_buff, "LIST");
    } else if (strcmp(token, "cd") == 0) {
        // Extract the argument
        char *arg = strtok(NULL, " \n");
        if (arg && strcmp(arg, "..") == 0) {
            strcpy(send_buff, "CDUP");  // Convert to CDUP if the argument is ".."
            return 0;
        } else {
            strcpy(send_buff, "CWD"); // Convert to CWD for other cases
            if (arg) { // If the argument exists
                strcat(send_buff, " ");
                strcat(send_buff, arg);
                return 0;
            }
        }
    } else if (strcmp(token, "quit") == 0) {
        strcpy(send_buff, "QUIT");
    } else if (strcmp(token, "pwd") == 0) {
        strcpy(send_buff, "PWD");
    } else if (strcmp(token, "mkdir") == 0) {
        strcpy(send_buff, "MKD");
    } else if (strcmp(token, "rmdir") == 0) {
        strcpy(send_buff, "RMD");
    } else if (strcmp(token, "delete") == 0) {
        strcpy(send_buff, "DELE");
    } else if (strcmp(token, "rename") == 0) {
        // Convert "rename old new" command to "RNFR old RNTO new"
        char *old_name = strtok(NULL, " \n");
        char *new_name = strtok(NULL, " \n");

        if (old_name) { // Check if at least one argument exists
            strcpy(send_buff, "RNFR ");
            strcat(send_buff, old_name);
            strcat(send_buff, " RNTO");
            if (new_name) {
                strcat(send_buff, " ");
                strcat(send_buff, new_name);
            }
        } else {
            strcpy(send_buff, "RNFR RNTO"); 
        }
        return 0;
    } else {
        // Pass unknown commands as they are
        strcpy(send_buff, token);
    }

    // Append additional arguments
    char* arg = strtok(NULL, " \n");
    while (arg) {
        strcat(send_buff, " ");
        strcat(send_buff, arg);
        arg = strtok(NULL, " \n");
    }
    return 0;
        
}