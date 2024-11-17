///////////////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                         //
// Date : 2024/05/18                                                         //
// OS : Ubuntu 20.04.6 LTS 64bits                                            //
//                                                                           //
// Author : Kang Hyun Min                                                    //
// Student ID : 2020202092                                                   //
// --------------------------------------------------------------------------//
// Title : System Programming Assignment #3-1 (FTP Client)                   //
// Description : This file attempts to connect to an FTP server and         //
//  if the IP address is correct, connects to the server successfully.        //
// Additionally, the ID and password are sent to the server through          //
// user input, and if the information is correct, login is successful.       //
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

#define MAX_BUF     20
#define CONT_PORT   20001

///////////////////////////////////////////////////////////////////////////////
// log_in                                                                    //
// ==========================================================================//
// Input: int sockfd - the socket file descriptor for communication with the  //
//                     server                                                //
// Output: None                                                              //
// Purpose: Handles the login process by prompting the user for credentials, //
//          sending them to the server, and processing the server's response.//
///////////////////////////////////////////////////////////////////////////////
void log_in(int sockfd);

///////////////////////////////////////////////////////////////////////////////
// main                                                                      //
// ==========================================================================//
// Input: int argc - the number of command line arguments                    //
//        char *argv[] - the array of command line arguments                 //
// Output: int - 0 on success, 1 or -1 on failure                            //
// Purpose: Initializes the client, establishes a connection with the server,//
//          and initiates the login process.                                 //
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]){

    //If IP address and Port number are not inputted
    if (argc != 3) { 
        write(1, "IP address and Port number required.\n", strlen("IP address and Port number required.\n"));
        return 1;
    }

    int sockfd, n, p_pid;
    struct sockaddr_in servaddr;

    // Create a socket for communication with the server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Initialize server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);  // Server IP address
    servaddr.sin_port = htons(atoi(argv[2])); // Server port number

    // Attempt to connect to the server
    if(connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1){
        printf("can't connect\n");
        return -1;
    }
    else printf("** It is connected to Server **\n");

    // Perform the login process
    log_in(sockfd);

    // Close the socket after communication
    close(sockfd);
    return 0;

}

//A function to handle the login process
void log_in(int sockfd){
    int n;
    char user[MAX_BUF] = {0}, *passwd = NULL, buf[MAX_BUF] = {0};
    
    // Read the initial response from the server
    n = read(sockfd, buf, MAX_BUF);
    buf[n] ='\0';

    // If the server sends a rejection message
    if(strncmp(buf, "REJECTION", 9) == 0){
        printf("** Connection refused **\n");
        exit(1);
    }

    // If the server accepts the connection
    else if(strncmp(buf, "ACCEPTED", 8) == 0){

    char username[MAX_BUF];
    char password[MAX_BUF];

    // Allow up to three login attempts
    for(int attempt = 0; attempt < 3; ++attempt){
        // Initialize the buffers to zero before writing
        memset(username, 0, MAX_BUF);
        memset(password, 0, MAX_BUF);

        // Prompt the user for their username
        printf("Input ID : ");
        scanf("%s", username);
        write(sockfd, username, strlen(username));
        
        // Prompt the user for their username           
        printf("Input Password : ");
        scanf("%s", password);
        write(sockfd, password, strlen(password));

        // Read the server's response to the login attempt
        n = read(sockfd, buf, MAX_BUF);
        buf[n] = '\0';

        // If login is successful
        if (strcmp(buf, "OK") == 0) {
            printf("** User '%s' logged in **\n", username);
            return;
        }

        // If login fails
        else if (strcmp(buf, "FAIL") == 0) {
            printf("** Log-in failed **\n");
        } 
        
        // If the server closes the connection after three failed attempts
        else if (strcmp(buf, "DISCONNECTION") == 0) {
            printf("** Connection closed **\n");
            exit(1);
        
        }
    }
    }
}