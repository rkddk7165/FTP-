///////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                 //
// Date : 2024/05/29                                                 //
// OS : Ubuntu 20.04.6 LTS 64bits                                    //
//                                                                   //
// Author : Kang Hyun Min                                            //
// Student ID : 2020202092                                           //
// ----------------------------------------------------------------  //
// Title : FTP Client                                                //
// Description : This program is a dual connection type FTP client   //
//               that reads user commands and transmits them         //
//               to the server through a control connection,         //
//               and receives the results from the server through    //
//               a data connection.                                  //
///////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

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
int create_data_socket(struct sockaddr_in *addr);

///////////////////////////////////////////////////////////////////////
// Function Name: main                                               //
// ================================================================= //
// Input: int argc - Number of command-line arguments                //
//        char **argv - Array of command-line arguments              //
// Output: None                                                      //
// Purpose: The main function of the FTP client. It establishes a    //
//          control connection with the server, sends FTP commands,  //
//          and receives responses using data connection with        //
//          the server. It supports command ls.                      //
///////////////////////////////////////////////////////////////////////
void main(int argc, char **argv) {
    int control_sock, data_sock;
    struct sockaddr_in server_addr, data_addr;
    socklen_t data_addr_len;
    char buff[2048] = {0}, cmd_buff[2048] = {0}, msg_buff[2048] = {0};

    // Check if IP address and port number are provided
    if (argc != 3) {
        fprintf(stderr, "IP address and Port number required\n");
        exit(1);
    }

    // Establishing the control connection  
    control_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (control_sock < 0) {
        perror("socket");
        exit(1);
    }

    // Setting up server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    // Connect to the server
    if (connect(control_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(control_sock);
        exit(1);
    }

    srand(time(NULL)); // SEt random seed for port number of data conneciton

    // Main loop for user input and FTP commands
    while (1) {
        memset(buff, 0, sizeof(buff));
        printf("> ");
        if (fgets(buff, sizeof(buff), stdin) == NULL) {
            break;
        }
        buff[strcspn(buff, "\n")] = 0; // Remove newline character

        if (strncmp(buff, "ls", 2) == 0) {
            // Establish data connection
            data_sock = create_data_socket(&data_addr);

            // Generate and send PORT command
            char *port_cmd = convert_addr_to_str(&data_addr, argv[1]);
            snprintf(cmd_buff, sizeof(cmd_buff), "PORT %s\n", port_cmd);
            free(port_cmd);
            write(control_sock, cmd_buff, strlen(cmd_buff));
            memset(cmd_buff, 0, sizeof(cmd_buff));

            // Wait for server response // 200
            read(control_sock, msg_buff, sizeof(msg_buff) - 1);
            printf("%s", msg_buff);
            memset(msg_buff, 0, sizeof(msg_buff));

            // Generate and send NLST command (including ls argument)
            snprintf(cmd_buff, sizeof(cmd_buff), "NLST%s\n", buff + 2);
            write(control_sock, cmd_buff, strlen(cmd_buff));    
            memset(cmd_buff, 0, sizeof(cmd_buff));

            // Wait for server data connection response //150
            read(control_sock, msg_buff, sizeof(msg_buff) - 1);
            printf("%s", msg_buff);
            memset(msg_buff, 0, sizeof(msg_buff));

            // Receive result of command from the server
            data_addr_len = sizeof(data_addr);
            int data_client_sock = accept(data_sock, (struct sockaddr*)&data_addr, &data_addr_len);
            int total_bytes = 0;
            while (1) {
                int bytes = read(data_client_sock, buff, sizeof(buff) - 1);
                if (bytes <= 0) break;
                buff[bytes] = 0;
                printf("%s", buff);
                total_bytes += bytes;   //Calculate bytes of the result data
            }

            
            //CLose data connection
            close(data_client_sock);    
            close(data_sock);   

            // Wait for final server response // 226
            read(control_sock, msg_buff, sizeof(msg_buff) - 1);
            printf("%s", msg_buff);
            memset(msg_buff, 0, sizeof(msg_buff));

            // Print OK message with received bytes count
            printf("OK. %d bytes is received.\n", total_bytes);


        //if command is not equal to ls (Unknown command)
        }
        // Conver quit to FTP command 'QUIT'
        else if (strncmp(buff, "quit", 4) == 0) {
            strcpy(cmd_buff, "QUIT");

            if (strlen(buff) > 2) {
                strcat(cmd_buff, buff + 4);
            }
            write(control_sock, cmd_buff, strlen(cmd_buff));
            memset(cmd_buff, 0, sizeof(cmd_buff));
            read(control_sock, cmd_buff, sizeof(cmd_buff));

            if(strncmp(cmd_buff, "221", 3) == 0){
                printf("%s", cmd_buff);
                break;
            }
        } 
        //If command is not equal to "PORT" or "QUIT"
         else {
            strcpy(cmd_buff, buff);
            strcat(cmd_buff, "\n");
            write(control_sock, cmd_buff, strlen(cmd_buff));
            read(control_sock, buff, sizeof(buff));
            printf("%s", buff);
        }

    }

    close(control_sock);
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
int create_data_socket(struct sockaddr_in *addr) {
    // Set up data connection
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    // Setting up data socket address structure
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(10001 + rand() % (30000 - 10001 + 1));

    // Bind the socket to the address
    if (bind(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("bind");
        close(sock);
        exit(1);
    }

    // Listen for incoming connections
    if (listen(sock, 1) < 0) {
        perror("listen");
        close(sock);
        exit(1);
    }

    return sock;
}
