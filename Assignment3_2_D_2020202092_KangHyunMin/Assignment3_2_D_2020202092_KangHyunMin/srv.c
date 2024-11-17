//////////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                    //
// Date : 2024/05/29                                                    //
// OS : Ubuntu 20.04.6 LTS 64bits                                       //
//                                                                      //
// Author : Kang Hyun Min                                               //
// Student ID : 2020202092                                              //
// -----------------------------------------------------------------    //
// Title : System Programming Assignment #3-2 (ftp server)              //
// Description : This file is a dual connection type ftp server.        //
//               When a command is received from a client through       //
//               a control connection, the result is transmitted        //
//               to the client through a data connection.               //
//////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <time.h>

#define BUFFSIZE 2048

///////////////////////////////////////////////////////////////////////
// NLST                                                              //
// ================================================================= //
// Input: char *argv[128] -> Array of command-line arguments,        //
//        int aflag -> Flag indicating whether to include hidden     //
//                      files (not implemented),                     //
//        int lflag -> Flag indicating whether to use long listing   //
//                      format (not implemented),                    //
//        char *result_buff -> Buffer to store the result of NLST    //
//                             command.                              //
//                                                                   //
// Output: None                                                      //
//                                                                   //
// Purpose: Executes the NLST (Name List) command. This function     //
//          retrieves the list of files and directories in the       //
//          current working directory and formats them into a        //
//          newline-separated list, which is stored in result_buff.  //
///////////////////////////////////////////////////////////////////////
void NLST(char *argv[128], int aflag, int lflag, char *result_buff);



///////////////////////////////////////////////////////////////////////
// convert_str_to_addr                                              //
// ================================================================= //
// Input: char *str -> String containing IP address and port in the  //
//                     format "a,b,c,d,p1,p2",                       //
//        unsigned int *port -> Pointer to an unsigned int variable //
//                              to store the extracted port number.  //
//                                                                   //
// Output: char* -> Pointer to a dynamically allocated string        //
//                  containing the IP address extracted from str.    //
//                                                                   //
// Purpose: Extracts IP address and port number from the given       //
//          string and returns the IP address as string              //
//          while storing the port number in port.                   //
///////////////////////////////////////////////////////////////////////
char *convert_str_to_addr(char *str, unsigned int *port);

///////////////////////////////////////////////////////////////////////
// cmd_process                                                       //
// ================================================================= //
// Input: const char *buff -> Command string received from the       //
//                             client,                               //
//        char *result_buff -> Buffer to store the result of command //
//                             processing.                           //
//                                                                   //
// Output: None                                                      //
//                                                                   //
// Purpose: Processes the command received from the client. It       //
//          parses the command string, executes the appropriate      //
//          action based on the command, and stores the result or    //
//          error message in result_buff.                            //
///////////////////////////////////////////////////////////////////////
void cmd_process(const char *buff, char *result_buff);


///////////////////////////////////////////////////////////////////////////////
// main                                                                      //
// ==========================================================================//
// Input: int argc - the number of command line arguments                    //
//        char *argv[] - the array of command line arguments                 //
// Output: int - 0 on success, 1 or -1 on failure                            //
// Purpose: When a command is received from a client through                 //
//          a control connection, the result is transmitted to the client    //
//          through a data connection. Additionally, success and failure     //
//          messages are transmitted through the control connection.         //
///////////////////////////////////////////////////////////////////////////////
void main(int argc, char **argv) {
    //Declare variables and struct
    unsigned int port_num;
    int control_sock, data_sock, client_sock;
    struct sockaddr_in server_addr, client_addr, data_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buff[BUFFSIZE] = {0}, result_buff[BUFFSIZE] = {0};
    unsigned int data_port;

    // Check if the port number is provided as a command line argument
    if (argc != 2) {
        fprintf(stderr, "Port number required\n");
        exit(1);
    }

    // Create a socket for the server (COntrol connection)
    control_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (control_sock < 0) {
        perror("socket");
        exit(1);
    }
    // Initialize the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));    // Set the server port number
    server_addr.sin_addr.s_addr = INADDR_ANY;       // Accept any incoming IP address

    // Bind the socket to the address
    if (bind(control_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(control_sock);
        exit(1);
    }

    // Listen for incoming connections
    if (listen(control_sock, 1) < 0) {
        perror("listen");
        close(control_sock);
        exit(1);
    }

    while(1){
        // Accept an incoming connection
    client_sock = accept(control_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock < 0) {
        perror("accept");
        close(control_sock);
        exit(1);
    }

    while (1) {
        //Initialize buffers
        memset(buff, 0, sizeof(buff));
        memset(result_buff, 0, sizeof(result_buff));
        if(read(client_sock, buff, sizeof(buff)) < 1)
            break;  //read command from client (control connection)



        //if command is "PORT", open data connection
        if (strncmp(buff, "PORT", 4) == 0) {

            printf("%s", buff);
            char *client_ip = convert_str_to_addr(buff + 5, &data_port);    //convert form of IP address to four integer 

            // Setting data connection socket
            data_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (data_sock < 0) {
                perror("socket");
                exit(1);
            }
            
            // Initialize the client address structure
            data_addr.sin_family = AF_INET;
            inet_pton(AF_INET, client_ip, &data_addr.sin_addr);
            data_addr.sin_port = htons(data_port);

            // Attempt to connect to the client
            if (connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
                memset(buff, 0, sizeof(buff));
                strcpy(buff, "550 Failed to access\n");
                printf("%s", buff);
                write(client_sock, buff, sizeof(buff));
                close(data_sock);
                exit(1);
            }

            free(client_ip);

            // Response to PORT command
            strcpy(buff, "200 Port command performed successfully.\n");
            printf("%s", buff);
            write(client_sock, buff, strlen(buff));

            // Waiting for NLST command
            memset(buff, 0, sizeof(buff));
            read(client_sock, buff, sizeof(buff));

            printf("%s", buff);
            cmd_process(buff, result_buff);     //Process "NLST" command behavior

            if (strncmp(buff, "NLST", 4) == 0) {
                strcpy(buff, "150 Opening data connection for directory list.\n");
                printf("%s", buff);
                write(client_sock, buff, strlen(buff));   
                memset(buff, 0, sizeof(buff));

                // Sending directory listing over data connection
                if(write(data_sock, result_buff, strlen(result_buff)) <0){
                    memset(buff, 0, sizeof(buff));
                    strcpy(buff, "550 Failed transmission.\n");
                    printf("%s", buff);
                    write(client_sock, buff, sizeof(buff));
                    memset(buff, 0, sizeof(buff));
                }
                close(data_sock);
                
                
                 
                //Data connection success (COde 226)
                strcpy(buff, "226 Complete transmission.\n");
                printf("%s", buff);
                write(client_sock, buff, strlen(buff));
                
            } else {
                // Sending error message
                strcpy(buff, result_buff);
                write(client_sock, buff, strlen(buff));
            }

        //If command is not equal to "PORT" (Unknown command)
        }
        
        else if (strncmp(buff, "QUIT", 4) == 0)  {
            
            // Check whether there is an argument ignoring spaces after the QUIT command
            char *argument = buff + 4;
            while (*argument == ' ') {
            argument++;
            }

            if (*argument != '\0') { // If there are arguments
                memset(buff, 0, sizeof(buff));    
                strcat(buff, "501 Syntax error in parameters or arguments.\n");
                printf("%s", buff);
                write(client_sock, buff, strlen(buff));
                continue;
            }
            memset(buff, 0, sizeof(buff));
            strcpy(buff, "221 Goodbye.\n");  // Send quit command success message to server
            printf("%s", buff);
            write(client_sock, buff, strlen(buff));
        
        } 

        else {
            memset(buff, 0, sizeof(buff));
            strcpy(buff, "500 Syntax error, command unrecognized.\n");  //Error code 500 (Unknown command)
            printf("%s", buff);
            write(client_sock, buff, strlen(buff));
            
        }


    }
    }
    
    //CLose control connection
    close(client_sock);
    close(control_sock);
    exit(1);         //exit program
}

//Process command behavior
void cmd_process(const char *buff, char *result_buff) {
    memset(result_buff, 0, BUFFSIZE); // Clear the result buffer

    char *buff_copy = strdup(buff); // Create a copy of buff
    int argc = 0;
    char *argv[128] = {NULL}; // Array to hold command-line arguments

    // Tokenize buff to create an array of arguments
    char *token = strtok(buff_copy, " \n");
    while (token != NULL && argc < sizeof(argv) / sizeof(argv[0])) {
        argv[argc++] = token;
        token = strtok(NULL, " \n");
    }

    argv[argc] = NULL; // Terminate the array with NULL

    if (strncmp(argv[0], "NLST", 4) == 0) {
        if (argc > 1) {
            // If NLST command has options or arguments, return an error
            strcpy(result_buff, "501 Syntax error in parameters or arguments.\n");
            printf("%s", result_buff);
        } else {
            NLST(argv, 0, 0, result_buff);
        }
    } else {
        // Unknown command response
        strcpy(result_buff, "500 Syntax error, command unrecognized.\n");
        printf("%s", result_buff);
    }

    free(buff_copy); // Free the copy of buff
}

/////////////////////////////////////////////////////////////////////////
// Function: bubblesort                                                //
// =================================================================== //
// Input: struct dirent** namelist -> Array of directory entries       //
//        int n -> Number of entries to sort                           //
// Output: None                                                        //
// Purpose: Sorts the array of directory entries in alphabetical order.//
/////////////////////////////////////////////////////////////////////////
void bubblesort(struct dirent **namelist, int n) {
    int i, j;
    struct dirent *temp;

    //sort namelist through loop
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - i - 1; j++) {
            if (strcmp(namelist[j]->d_name, namelist[j + 1]->d_name) > 0) {
                temp = namelist[j];
                namelist[j] = namelist[j + 1];
                namelist[j + 1] = temp;
            }
        }
    }
}

//Process NLST command behavior
void NLST(char *argv[128], int aflag, int lflag, char *result_buff) {
    memset(result_buff, 0, BUFFSIZE); // Initialize the result buffer
    char *directory = getcwd(NULL, 0); // Use the current directory

    struct stat file_stat;

    // Check if the specified path is a valid directory or file
    if (stat(directory, &file_stat) == -1) {
        strcat(result_buff, "550 No such file or directory.\n");
        return;
    }

    if (S_ISDIR(file_stat.st_mode)) {
        // The input is a directory
        DIR *dir;
        struct dirent **namelist;
        struct dirent *entry;

        // Check if the directory can be accessed
        if (access(directory, R_OK) == -1) {
            strcat(result_buff, "550 Cannot access directory.\n");
            printf("%s", result_buff);
            return;
        }

        // Open the directory
        dir = opendir(directory);
        if (dir == NULL) {
            strcat(result_buff, "550 Failed to open directory.\n");
            printf("%s", result_buff);
            return;
        }

        // Count the number of valid entries
        int n = 0;
        while ((entry = readdir(dir)) != NULL) {
            n++;
        }

        // Allocate memory for the directory entry list
        namelist = (struct dirent **)malloc(n * sizeof(struct dirent *));
        if (namelist == NULL) {
            closedir(dir);
            return;
        }

        // Populate namelist with valid entries
        rewinddir(dir); // Reset the directory read pointer
        int i = 0;
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".." entries
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                namelist[i++] = entry; // Add to the list
            }
        }

        // Sort the entries
        bubblesort(namelist, i);

        // Iterate through the sorted entries to build the result buffer
        for (int j = 0; j < i; j++) {
            char *name = namelist[j]->d_name;
            strcat(result_buff, name);
            strcat(result_buff, "\n");
        }


        free(namelist);
        closedir(dir);
    } else {
        // If the input is a file
        strcat(result_buff, directory);
        strcat(result_buff, "\n");
    }
}

//COnvert string to address and port number
char *convert_str_to_addr(char *str, unsigned int *port) {
    char *addr_str = malloc(16);
    unsigned int a, b, c, d, p1, p2;
    //four integer (IP address) and two 8-bit binary (Port number)
    sscanf(str, "%u,%u,%u,%u,%u,%u", &a, &b, &c, &d, &p1, &p2);
    snprintf(addr_str, 16, "%u.%u.%u.%u", a, b, c, d);
    *port = p1 * 256 + p2;  // Two 8-bit binary
    return addr_str;
}

