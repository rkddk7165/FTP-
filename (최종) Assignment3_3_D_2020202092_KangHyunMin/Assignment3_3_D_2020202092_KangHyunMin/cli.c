///////////////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                         //
// Date : 2024/06/04                                                         //
// OS : Ubuntu 20.04.6 LTS 64bits                                            //
//                                                                           //
// Author : Kang Hyun Min                                                    //
// Student ID : 2020202092                                                   //
// --------------------------------------------------------------------------//
// Title : System Programming Assignment #3-3 (FTP Client)                   //
// Description : This program is client-sie program that                     //
//               connects to an FTP server, sends messages,                  //
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
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE 4096

#define MAX_BUF     20
#define CONT_PORT   20001

void log_in(int sockfd);

char send_buff[128];

void handle_command(int sockfd, char *send_buff, const char *server_ip);

///////////////////////////////////////////////////////////////////////
// Function Name: convert_addr_to_str                                //
// ================================================================= //
// Input: struct sockaddr_in *addr - Pointer to the address structure//
//        const char *ip - IP address                                //
// Output: char* - String representation of the address and port     //
// Purpose: Convert sockaddr_in address to string format for PORT    //
//          command.                                                 //
///////////////////////////////////////////////////////////////////////
char* convert_addr_to_str(struct sockaddr_in *addr, const char *ip);

///////////////////////////////////////////////////////////////////////
// Function Name: create_data_socket                                 //
// ================================================================= //
// Input: struct sockaddr_in *addr - Pointer to the address structure//
// Output: int - Socket file descriptor for data connection          //
// Purpose: Create and bind a socket for data connection             //
///////////////////////////////////////////////////////////////////////
void create_data_socket(int *data_sock, struct sockaddr_in *data_addr);


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
//          receives responses from the server and print the result  //               
// ----------------------------------------------------------------- //
///////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
    if (argc != 3) {
        write(1, "IP address and Port number required.\n", strlen("IP address and Port number required.\n"));
        return 1;
    }

    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Clear the server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // Connect to the server
    connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

     //Perform the login process here if required
    log_in(sockfd);

    while (1) {
        write(1, "> ", 2); //// Prompt the user for input
        char buff[BUF_SIZE] = {0};
        char send_buff[BUF_SIZE] = {0};
        read(0, buff, BUF_SIZE);    //// Read input from the standard input (keyboard)

        // Convert the user input command
        if (conv_command(buff, send_buff) == -1) {
            printf("Invalid command.\n");
            continue;
        }
        // Handle the command by sending it to the server
        handle_command(sockfd, send_buff, argv[1]);
    }
    close(sockfd);
    return 0;
}
//A function to process command and write the result to server 
//(if command is port, create data connection and handle them.)
void handle_command(int sockfd, char *send_buff, const char *server_ip) {
    int data_sock;
    struct sockaddr_in data_addr;
    socklen_t data_addr_len;
    char cmd_buff[BUF_SIZE], msg_buff[BUF_SIZE], buff[BUF_SIZE];
    int n, port = 10001 + rand() % 50000;   // Generate a random port number

    

    // Only create data socket and send PORT command for commands that require data transfer
    if (strncmp(send_buff, "NLST", 4) == 0 || 
        strncmp(send_buff, "STOR", 4) == 0 || strncmp(send_buff, "RETR", 4) == 0) {
        create_data_socket(&data_sock, &data_addr);

        // Generate and send PORT command
        char *port_cmd = convert_addr_to_str(&data_addr, server_ip);
        snprintf(cmd_buff, sizeof(cmd_buff), "PORT %s\n", port_cmd);
        free(port_cmd);
        write(sockfd, cmd_buff, strlen(cmd_buff));
        memset(cmd_buff, 0, sizeof(cmd_buff));

        // Wait for server response
        read(sockfd, msg_buff, sizeof(msg_buff) - 1);
        printf("%s", msg_buff);
        memset(msg_buff, 0, sizeof(msg_buff));

        if (strstr(msg_buff, "550") != NULL) {
            close(data_sock);
            return;
        }
    }

    // Send the FTP command to the server
    snprintf(cmd_buff, sizeof(cmd_buff), "%s\n", send_buff);
    write(sockfd, cmd_buff, strlen(cmd_buff));
    memset(cmd_buff, 0, sizeof(cmd_buff));

    // Wait for server response and handle data transfer if needed
    if (strncmp(send_buff, "NLST", 4) == 0 || 
        strncmp(send_buff, "STOR", 4) == 0 || strncmp(send_buff, "RETR", 4) == 0) {
        // Wait for server data connection response
        read(sockfd, msg_buff, sizeof(msg_buff) - 1);
        printf("%s", msg_buff);
        memset(msg_buff, 0, sizeof(msg_buff));

        // Handle data connection
        data_addr_len = sizeof(data_addr);
        int data_client_sock = accept(data_sock, (struct sockaddr*)&data_addr, &data_addr_len);
        int total_bytes = 0;


        if (strncmp(send_buff, "NLST", 4) == 0) {
            while (1) {
                int bytes = read(data_client_sock, buff, sizeof(buff) - 1);
                if (bytes <= 0) break;
                buff[bytes] = 0;
                printf("%s", buff);
                total_bytes += bytes;
            }
        } 
        //Check if FTP command ie "STOR"
        else if (strncmp(send_buff, "STOR", 4) == 0) {
            char *arg = strchr(send_buff, ' ');
            if (arg) {
                int fd = open(arg + 1, O_RDONLY);
                if (fd < 0) {
                    perror("Failed to open file");
                    close(data_sock);
                    return;
                }
                // read the result repeatedly
                while ((n = read(fd, buff, BUF_SIZE)) > 0) {
                    write(data_client_sock, buff, n);
                    total_bytes += n;   //total bytes of read data
                }
                close(fd);
            }
        } 
        //Check if FTP command ie "STOR"
        else if (strncmp(send_buff, "RETR", 4) == 0) {
            char *arg = strchr(send_buff, ' ');
            if (arg) {
                int fd = open(arg + 1, O_WRONLY | O_CREAT, 0666);
                if (fd < 0) {
                    perror("Failed to open file");
                    close(data_sock);
                    return;
                }
                // read the result repeatedly
                while ((n = read(data_client_sock, buff, BUF_SIZE)) > 0) {
                    write(fd, buff, n);
                    total_bytes += n;   //total bytes of read data
                }
                close(fd);
            }
        }

        // Close data connection
        close(data_client_sock);
        close(data_sock);

        // Wait for final server response
        read(sockfd, msg_buff, sizeof(msg_buff) - 1);
        printf("%s", msg_buff);
        memset(msg_buff, 0, sizeof(msg_buff));

        printf("OK. %d bytes is received.\n", total_bytes);
    } else {
        // For commands that don't require data transfer, just wait for the server response
        read(sockfd, msg_buff, sizeof(msg_buff) - 1);
        printf("%s", msg_buff);
        if(strncmp(msg_buff, "221", 3) == 0){
            exit(0);
        }
        memset(msg_buff, 0, sizeof(msg_buff));
    }
}

//A function to handle the login process
void log_in(int sockfd){
    int n;
    char buf[MAX_BUF] = {0};
    char msg_buff[MAX_BUF] = {0};

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

        char user_input[MAX_BUF] = {0};
        char send_input[MAX_BUF] = {0};
        char user[MAX_BUF] ={0};

        // Allow up to three login attempts
        for(int attempt = 0; attempt < 3; ++attempt){
            // Initialize the buffer to zero before writing
            memset(user_input, 0, MAX_BUF);
            memset(send_input, 0, MAX_BUF);

            // Prompt the user for their username with "USER " prefix
            printf("Name: ");
            scanf("%s", user_input);  // Reads the whole line including spaces
            strcpy(send_input, "USER ");
            strcat(send_input, user_input);
            write(sockfd, send_input, strlen(send_input));
            strcpy(user, user_input);

            // Initialize the buffer to zero before writing
            memset(user_input, 0, MAX_BUF);
            memset(send_input, 0, MAX_BUF);

            printf("331 Password required for %s.\n", user);

            // Prompt the user for their password with "PASS " prefix
            printf("Password: ");
            scanf("%s", user_input);  // Reads the whole line including spaces
            strcpy(send_input, "PASS ");
            strcat(send_input, user_input);
            write(sockfd, send_input, strlen(send_input));

            // Read the server's response to the login attempt
            memset(buf, 0, MAX_BUF);
            n = read(sockfd, buf, MAX_BUF);
            buf[n] = '\0';

            // If login is successful
            if (strcmp(buf, "OK") == 0) {
                printf("220 User %s logged in.\n", user);
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

// Function to convert user input to FTP command
int conv_command(char* buff, char* send_buff) {

    char tmp[30] = {0};
    strcpy(tmp, buff);

    char *token = strtok(buff, " \n");  // Consider the first token as the command
    
    if (!token) return -1;
    // Convert the command to FTP command
    if (strcmp(token, "ls") == 0) {
        strcpy(send_buff, "NLST");
    } else if (strcmp(token, "dir") == 0) {
        strcpy(send_buff, "LIST");
    } else if (strncmp(token, "cd", 2) == 0) {
        // Extract the argument
        char *arg = strtok(NULL, " \n");
        if (arg && strncmp(arg, "..", 2) == 0) {
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
            strcat(send_buff, " RNTO ");
            if (new_name) {
                strcat(send_buff, new_name);
            }
        } else {
            return -1; // Invalid command format
        }
    } else if (strcmp(token, "put") == 0) {
        char *filename = strtok(NULL, " \n");
        if (filename) {
            strcpy(send_buff, "STOR ");
            strcat(send_buff, filename);
        } else {
            return -1; // No filename specified
        }
    } else if (strcmp(token, "get") == 0) {
        char *filename = strtok(NULL, " \n");
        if (filename) {
            strcpy(send_buff, "RETR ");
            strcat(send_buff, filename);
        } else {
            return -1; // No filename specified
        }
    } else if (strcmp(token, "bin") == 0 || strncmp(tmp, "type binary", 11) == 0){
        strcpy(send_buff, "TYPE I");
    } else if (strcmp(token, "ascii") == 0 || strncmp(tmp, "type ascii", 10) == 0){
        strcpy(send_buff, "TYPE A");
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

//COnvert address and port number to string
char* convert_addr_to_str(struct sockaddr_in *addr, const char *ip) {
    char *addr_str = malloc(25);
    unsigned int port = ntohs(addr->sin_port);
    int ip1, ip2, ip3, ip4;
    sscanf(ip, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4); // Split IP address into four integers
    snprintf(addr_str, 25, "%d,%d,%d,%d,%u,%u",
             ip1, ip2, ip3, ip4, (port >> 8) & 0xFF, port & 0xFF);  //Convert port number to binary
    return addr_str;
}

//Create data connection when command is "PORT"
void create_data_socket(int *data_sock, struct sockaddr_in *data_addr) {
    *data_sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(data_addr, 0, sizeof(struct sockaddr_in));
    data_addr->sin_family = AF_INET;
    data_addr->sin_addr.s_addr = inet_addr("127.0.0.1");
    data_addr->sin_port = htons(10001 + rand() % 50000);
    bind(*data_sock, (struct sockaddr *)data_addr, sizeof(struct sockaddr_in));
    listen(*data_sock, 1);
}