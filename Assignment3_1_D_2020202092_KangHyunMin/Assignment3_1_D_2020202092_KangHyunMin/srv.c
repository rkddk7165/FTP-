//////////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                    //
// Date : 2024/05/18                                                    //
// OS : Ubuntu 20.04.6 LTS 64bits                                       //
//                                                                      //
// Author : Kang Hyun Min                                               //
// Student ID : 2020202092                                              //
// -----------------------------------------------------------------    //
// Title : System Programming Assignment #3-1 (ftp server)              //
// Description : This program acts as an FTP server that authenticates  //
//               clients based on their IP address and credentials.     //
//               It handles client connections and manages login        //
//               attempts, providing access only to authenticated       //
//               users.                                                 //
//////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <pwd.h>

#define MAX_BUF 20

///////////////////////////////////////////////////////////////////////
// log_auth                                                          //
// ==================================================================//
// Input: int connfd - the socket file descriptor for communication  //
//                     with the client                               //
// Output: int - 1 on successful authentication, 0 on failure        //
// Purpose: Handles the login authentication process by reading the  //
//          client's credentials and checking them against stored    //
//          values. Allows up to three attempts before disconnection.//
///////////////////////////////////////////////////////////////////////
int log_auth(int connfd);

///////////////////////////////////////////////////////////////////////
// user_match                                                        //
// ==================================================================//
// Input: char *user - the username provided by the client           //
//        char *passwd - the password provided by the client         //
// Output: int - 1 if the username and password match the stored     //
//               credentials, 0 otherwise                            //
// Purpose: Compares the provided username and password against the  //
//          stored credentials to verify the client's identity.      //
///////////////////////////////////////////////////////////////////////
int user_match(char *user, char *passwd);

///////////////////////////////////////////////////////////////////////
// client_info                        				//
// Output: None                                                      //
// Purpose: Prints the client's IP address and port number when they //
//          attempt to connect to the server.                        //
///////////////////////////////////////////////////////////////////////
void client_info(const struct sockaddr_in *cliaddr);


int main(int argc, char* argv[]) {
    // Check if the port number is provided as a command line argument
    if (argc != 2) {
        fprintf(stderr, "Port number required.\n");
        return 1;
    }

    int listenfd, connfd, server_fd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    FILE *fp_checkIP;   //FILE stream to check client's IP

    

    
    // Create a socket for the server
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }
    // Initialize the server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept any incoming IP address
    servaddr.sin_port = htons(atoi(argv[1])); // Set the server port number

    // Bind the socket to the address
    if(bind(server_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        write(1, "Server: Can't bind local address.\n", strlen("Server: Can't bind local address.\n"));
        return 0;
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) < 0) {
        write(1, "Server: Listen failed.\n", strlen("Server: Listen failed.\n"));
        close(server_fd);
        return -1;
    }

    while(1){
        // Accept an incoming connection
        connfd = accept(server_fd, (struct sockaddr*) &cliaddr, &clilen);
        if (connfd < 0) {
            perror("Accept failed");
            continue;
        }
        // Display client information
        client_info(&cliaddr);
        
        char client_ip[MAX_BUF];
        // Convert client IP address to string and store it in client_ip
        snprintf(client_ip, sizeof(client_ip), "%s", inet_ntoa(cliaddr.sin_addr));

        // Open the access.txt file to check if the client's IP is allowed
        FILE *fp_checkIP = fopen("access.txt", "r");

        if (fp_checkIP == NULL) {
            perror("Failed to open access.txt");
            close(connfd);
            continue;
        }

        char line[MAX_BUF];
        int access_granted = 0;

        // Check each line in the access.txt file
        while (fgets(line, sizeof(line), fp_checkIP)) {
            line[strcspn(line, "\n")] = 0; // Remove newline

            // Check if the line is a wildcard for any IP
            if(strncmp(line, "*.*.*.*", strlen("*.*.*.*")) == 0){
                access_granted = 1;
                break;
            }

            // Check if the line matches the client's IP
            if (strcmp(line, client_ip) == 0) {
                access_granted = 1;
                break;
            }

            char *temp = strtok(line, "*");
            // Check for wildcard at the end of the line
            if(strncmp(temp, client_ip, strlen(temp)) == 0){
                access_granted = 1;
            }
        }
        fclose(fp_checkIP);

        // If the client IP is not allowed, send rejection and close the connection
        if (!access_granted) {
            write(connfd, "REJECTION", MAX_BUF);
            printf("** It is NOT authenticated client **\n");
            close(connfd);
            continue;
        } else {
            // If the client IP is allowed, send acceptance
            write(connfd, "ACCEPTED", MAX_BUF);
            printf("** Client is connected **\n");
        }
        // Handle login authentication
        if(log_auth(connfd) == 0){  //if 3 times fail (ok : 1, fail : 0)
            printf("** Fail to log-in **\n");
            close(connfd);
            continue;

        }
        close(connfd);
    }
    close(server_fd);
    return 0;
}
// Function to handle login authentication
int log_auth(int connfd){
    char user[MAX_BUF] ={0}, passwd[MAX_BUF] ={0};
    int n, count = 0;

    //Three tiems attempts
    while (count < 3) {

        // Initialize the buffers to zero before reading new data
        memset(user, 0, MAX_BUF);
        memset(passwd, 0, MAX_BUF);

        // Read username and password from client
        read(connfd, user, MAX_BUF);
        read(connfd, passwd, MAX_BUF);

        printf("**User is trying to log-in (%d/3)**\n", count + 1);

        // Check if the username and password match
        if (user_match(user, passwd) == 1) {
            write(connfd, "OK", MAX_BUF);
            printf("**Success to log-in**\n");
            return 1;
        } else {    //If the username or password does not match
            count++;
            printf("**Log-in failed**\n");
            if (count < 3) {
                write(connfd, "FAIL", MAX_BUF);
            } else {
                write(connfd, "DISCONNECTION", MAX_BUF);
                return 0;
            }
        }
    }
    return 0;
}

// Function to check if the username and password match the stored credentials
int user_match(char *user, char *passwd){
    FILE *fp;
    
    // Open the passwd.txt file
    if((fp = fopen("passwd.txt", "r")) == NULL){
        perror("Failed to open passwd");
        return 0;
    }

    struct passwd *pw; 

    char stored_user[MAX_BUF] = {0}, stored_passwd[MAX_BUF] = {0};
    // Check each entry in the passwd.txt file
    while ((pw = fgetpwent(fp)) != NULL) {
        snprintf(stored_user, MAX_BUF, "%s", pw->pw_name);      //get userID from passwd.txt
        snprintf(stored_passwd, MAX_BUF, "%s", pw->pw_passwd);  //get passwd from passwd.txt
        if (strcmp(user, stored_user) == 0 && strcmp(passwd, stored_passwd) == 0) {
            fclose(fp);
            return 1;   //Success
        }
    }

    fclose(fp);
    return 0;   //Fail
   
}
//A function to print client information
void client_info(const struct sockaddr_in *cliaddr) {
    printf("** Client is trying to connect **\n");
    printf(" - IP: %s\n", inet_ntoa(cliaddr->sin_addr));
    printf(" - Port: %d\n", ntohs(cliaddr->sin_port));
}
