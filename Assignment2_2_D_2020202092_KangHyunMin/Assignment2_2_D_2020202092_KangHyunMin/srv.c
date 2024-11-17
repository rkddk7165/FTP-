//////////////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                        //
// Date : 2024/05/08                                                        //
// OS : Ubuntu 20.04.6 LTS 64bits                                           //
//                                                                          //
// Author : Kang Hyun Min                                                   //
// Student ID : 2020202092                                                  //
// -------------------------------------------------------------------------//
// Title : System Programming Assignment #2-2 (FTP Server)                  //
// Description : This program is an FTP server that listens for incoming    //
//               connections from clients. Upon connection, it creates a    //
//               child process to handle the communication with the client. //
//               It echoes the messages back to the client. If the message  //
//               is "QUIT", the child process sets an alarm to terminate    //
//               after 1 second, allowing the server to clean up properly.  //
// -------------------------------------------------------------------------//
//////////////////////////////////////////////////////////////////////////////

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

void sh_chld(int); // Signal handler for SIGCHLD
void sh_alrm(int); // Signal handler for SIGALRM

//////////////////////////////////////////////////////////////////////////
// Function: client_info                                                //
// ===================================================================  //
// Input: struct sockaddr_in* cliaddr -> Client address structure       //
// Output: None                                                         //
// Purpose: Prints the client information, including IP and port number.//
//////////////////////////////////////////////////////////////////////////
void client_info(const struct sockaddr_in *cliaddr) {
    const char* info_header = "=========Client info=========\n";
    write(1, info_header, strlen(info_header));

    char ip_info[50];
    snprintf(ip_info, sizeof(ip_info), "Client IP: %s\n\n", inet_ntoa(cliaddr->sin_addr));
    write(1, ip_info, strlen(ip_info)); // Write client IP address

    char port_info[50];
    snprintf(port_info, sizeof(port_info), "Client port: %d\n", ntohs(cliaddr->sin_port));
    write(1, port_info, strlen(port_info)); // Write client port number

    const char* info_footer = "=============================\n";
    write(1, info_footer, strlen(info_footer));
}

//////////////////////////////////////////////////////////////////////////
// Function: main                                                       //
// =====================================================================//
// Input: int argc -> The number of arguments of inputted string        //
//        char **argv -> Array of command-line arguments                //
// Output: int - Returns 0 on success, non-zero on failure              //
// ---------------------------------------------------------------------//
// Purpose: Starts the FTP server, listens for incoming connections,    //
//          and creates a child process for each connection. The child  //
//          process handles communication with the client. If the       //
//          message is "QUIT", the child process sets an alarm to       //
//          terminate after 1 second.                                   //
// ---------------------------------------------------------------------//
//////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv){
    // Check for port number argument
    if (argc != 2) {
        write(1, "Port Number required.\n", strlen("Port Number required.\n"));
        return 1;
    }

    char buff[BUF_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr; // Address structures
    int server_fd, client_fd;
    int len;
    int port;


    signal(SIGCHLD, sh_chld); // Apply SIGCHLD handler
    signal(SIGALRM, sh_alrm); // Apply SIGALRM handler

    server_fd = socket(PF_INET, SOCK_STREAM, 0); // Create server socket

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on any interface
    server_addr.sin_port = htons(atoi(argv[1])); // Set server port number

    // Bind the socket to the address
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Listen for incoming connections
    listen(server_fd, 5);

    // Main server loop to accept client connections
    while(1){
        pid_t pid;
        len = sizeof(client_addr);
        // Accept incoming connections
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);

        pid = fork(); // Create child process using fork()
        if (pid == 0){ // Child process behavior
            close(server_fd);
            printf("Child process ID: %d\n", getpid()); // Display child process ID

            char buff[BUF_SIZE]; // Communication buffer
            while (1) {
                memset(buff, 0, BUF_SIZE);
                int n = read(client_fd, buff, BUF_SIZE); // Read from client
                if (n <= 0) {
                    break; //If read fails or client disconnects, break loop
                }

                if (write(client_fd, buff, n) <= 0) { // Echo back to client
                    break; // If write fails, break loop
                }

                if (strncmp(buff, "QUIT", 4) == 0) { // If client sends "QUIT"
                    close(client_fd); // Close client connection
                    alarm(1); // Trigger SIGALRM in 1 second
                    while(1); // Wait for alarm signal
                }
            }
            close(client_fd); // Close client connection when loop ends
            exit(0); // Exit child process
        }
        else if (pid > 0) { // Parent process behavior
            client_info(&client_addr); // Display client information
            close(client_fd); 
        } 
        else {
            perror("Fork failed");
            close(client_fd);
        }

        close(client_fd); // Close server socket on exit
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////
// Function: sh_chld                                                 //
// ==================================================================//
// Input: int signum -> Signal number (SIGCHLD)                      //
// Output: None                                                      //
// ----------------------------------------------------------------- //
// Purpose: Handles the SIGCHLD signal when a child process          //
//          terminates. It cleans up the child process using `wait`. //
// ----------------------------------------------------------------- //
///////////////////////////////////////////////////////////////////////
void sh_chld(int signum){
    printf("Status of Child process was changed.\n"); // Notify when a child process status changes
    wait(NULL); // Wait for child process to terminate
}

//////////////////////////////////////////////////////////////////////////
// Function: sh_alrm                                                    //
// =====================================================================//
// Input: int signum -> Signal number (SIGALRM)                         //
// Output: None                                                         //
// ---------------------------------------------------------------------//
// Purpose: Handles the SIGALRM signal, indicating that a child process //
//          should terminate. The function outputs a message and exits  //
//          with a status code of 1, triggering SIGCHLD for the parent  //
//          process to clean up.                                        //
// ---------------------------------------------------------------------//
//////////////////////////////////////////////////////////////////////////
void sh_alrm(int signum){
    // Signal that the child process is terminating
    printf("Child Process(PID : %d) will be terminated.\n", getpid());
    exit(1); // Terminate the process
}