///////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                 //
// Date : 2024/06/04                                                 //
// OS : Ubuntu 20.04.6 LTS 64bits                                    //
//                                                                   //
// Author : Kang Hyun Min                                            //
// Student ID : 2020202092                                           //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #3-3 ( ftp server )         //
// Description : The purpose of this file is to read FTP-COMMAND     //
// data from client and perform operations according to this command,//                     
// and send the result to client and write the result into logfile.  //
// Also, this file manages user / passwd information.                //          
///////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <inttypes.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>

#define BUFFSIZE 4096
#define MAX_BUF 128

//ascii / binary mode define
#define FLAGS (O_RDWR | O_CREAT | O_TRUNC)
#define ASCII_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define BIN_MODE (S_IXUSR | S_IXGRP | S_IXOTH)

char reply_buff[128] = {0};
mode_t type;

//Structure that stores information about the currently connected client
typedef struct {
    pid_t pid; // Child Process PID
    int client_port; // Client Port number
    time_t start_time; // Running time
} ProcessInfo;

// Process information array and counters
#define MAX_PROCESS 50
ProcessInfo process_list[MAX_PROCESS];
int process_count = 0;

int server_fd;
char result_buff[BUFFSIZE];

char client_IP[128] ={0};
uint16_t client_PORT = 0;
char client_USER[128] = {0};

void write_log(char message[128]);  //A function to write the result into logfile.


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

///////////////////////////////////////////////////////////////////////////
// Function: cmd_process                                                 //
// ===================================================================   //
// Input: const char* buff -> Command received from client               //
//        char* result_buff -> Buffer to store the command result        //
// Output: None                                                          //
// Purpose: Processes the command received from the client and populates //
//          result_buff with the appropriate response.                   //
///////////////////////////////////////////////////////////////////////////
void cmd_process(const char* buff, char result_buff[BUFFSIZE]); 

/////////////////////////////////////////////////////////////////////////
// Function: bubblesort                                                //
// =================================================================== //
// Input: struct dirent** namelist -> Array of directory entries       //
//        int n -> Number of entries to sort                           //
// Output: None                                                        //
// Purpose: Sorts the array of directory entries in alphabetical order.//
/////////////////////////////////////////////////////////////////////////
void bubblesort(struct dirent **namelist, int n);

//////////////////////////////////////////////////////////////////////////
// Function: NLST                                                       //
// ===================================================================  //
// Input: char* argv[128] -> Array of command-line arguments            //
//        int aflag -> '-a' option flag (include hidden files)          //
//        int lflag -> '-l' option flag (detailed list)                 //
//        char result_buff[] -> Buffer to store the directory listing   //
// Output: None                                                         //
// Purpose: Generates a directory listing based on the provided options.//
//////////////////////////////////////////////////////////////////////////
void NLST(char* argv[128], int aflag, int lflag, char result_buff[BUFFSIZE]);

//////////////////////////////////////////////////////////////////////////
// Function: PWD                                                        //
// ===================================================================  //
// Input: char result_buff[] -> Buffer to store the result              //
// Output: None                                                         //
// Purpose: Returns current working directory path.                     //
//////////////////////////////////////////////////////////////////////////
void PWD(char result_buff[BUFFSIZE]);

//////////////////////////////////////////////////////////////////////////
// Function: RENAME                                                     //
// ===================================================================  //
// Input: char* argv[128] -> Array of command-line arguments            //          
// Output: None                                                         //
// Purpose: Rename a file named 'argv[1]' to 'argv[2]'                  //
//////////////////////////////////////////////////////////////////////////
void RENAME(char * argv[128], char result_buff[BUFFSIZE]);

//////////////////////////////////////////////////////////////////////////
// Function: CD                                                         //
// ===================================================================  //
// Input: char* path[128] -> array of path to change                    //              
//        char result_buff[] -> Buffer to store the result              //
// Output: None                                                         //
// Purpose: change current working directory to inputted path           //
//////////////////////////////////////////////////////////////////////////
void CD(char** path, char result_buff[BUFFSIZE]);

//////////////////////////////////////////////////////////////////////////
// Function: MKD                                                        //
// ===================================================================  //
// Input: char** path -> A path to change current directory             //
//        char result_buff[] -> Buffer to store the result              //
// Output: None                                                         //
// Purpose: Create new directory named 'argv[1]'                        //
//////////////////////////////////////////////////////////////////////////
void MKD(char* argv[128], char result_buff[BUFFSIZE]);

//////////////////////////////////////////////////////////////////////////
// Function: RMD                                                        //
// ===================================================================  //
// Input: char* argv[128] -> Array of command-line arguments            //
//        char result_buff[] -> Buffer to store the result              //
// Output: None                                                         //
// Purpose: Remove a directory named 'argv[1]'                        //
//////////////////////////////////////////////////////////////////////////
void RMD(char **dirnames, char result_buff[BUFFSIZE]);

//////////////////////////////////////////////////////////////////////////
// Function: DELE                                                       //
// ===================================================================  //
// Input: char* filenames[128] -> The name of file to delete            //
//        char result_buff[] -> Buffer to store the result              //
// Output: None                                                         //
// Purpose: Delete a file named 'argv[1]'                               //
//////////////////////////////////////////////////////////////////////////
void DELE(char **filenames, char result_buff[BUFFSIZE]);


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

//////////////////////////////////////////////////////////////////////////////
// Function: create_data_socket                                             //
// ===================================================================      //
// Input: struct sockaddr_in *data_addr -> Pointer to data socket address   //
// Output: int -> File descriptor of the created data socket                //
// Purpose: Create and bind a data socket for data transfer, set up to      //
//          listen for incoming data connections.                           //
//////////////////////////////////////////////////////////////////////////////
int create_data_socket(struct sockaddr_in *data_addr);

//////////////////////////////////////////////////////////////////////////////
// Function: handle_port_command                                            //
// ===================================================================      //
// Input: int client_sock -> Client socket descriptor                       //
//        char *buff -> Buffer containing the PORT command                  //
// Output: void                                                             //
// Purpose: Parse the PORT command, create a data socket, and handle        //
//          subsequent NLST command and data transmission.                  //
//////////////////////////////////////////////////////////////////////////////
void handle_port_command(int client_sock, char *buff);

//////////////////////////////////////////////////////////////////////////////
// Function: handle_get_command                                             //
// ===================================================================      //
// Input: int connfd -> Client connection file descriptor                   //
//        int data_sock -> Data connection file descriptor                  //
//        char *filename -> Name of the file to be transferred              //
// Output: void                                                             //
// Purpose: Handles the FTP GET command, which retrieves a file from the    //
//          server and sends it to the client over the data connection.     //
//////////////////////////////////////////////////////////////////////////////
void handle_get_command(int connfd, int data_sock, char *filename);

//////////////////////////////////////////////////////////////////////////////
// Function: handle_put_command                                             //
// ===================================================================      //
// Input: int connfd -> Client connection file descriptor                   //
//        int data_sock -> Data connection file descriptor                  //
//        char *filename -> Name of the file to be created and written to   //
// Output: void                                                             //
// Purpose: Handles the FTP PUT command, which receives a file from the     //
//          client and saves it on the server.                              //
//////////////////////////////////////////////////////////////////////////////
void handle_put_command(int connfd, int data_sock, char *filename);

//SIGINT signal handler function
void sh_int(int signum) {
    write_log("The server has shut down.\n");

    close(server_fd);
    exit(0); // Server exit
}

int logfile = 0;

// Open the log file
void open_logfile() {
    logfile = open("logfile", O_CREAT|O_APPEND|O_WRONLY, 0751);
    if (logfile == 0) {
        perror("Failed to open logfile");
        exit(1);
    }
}

// Close the log file
void close_logfile() {
    if (logfile != 0) {
        close(logfile);
    }
}

// Get the current date and time as a string
void get_current_time(char *buffer, int size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

// Write a message to the log file
void write_log(char message[128]) {
    char time_buffer[64];
    char tmp_buffer[256];
    get_current_time(time_buffer, sizeof(time_buffer));
    
    sprintf(tmp_buffer, "[%s] %s:%d %s", time_buffer, client_IP, client_PORT, message);
    write(logfile, tmp_buffer, strlen(tmp_buffer));
}

//////////////////////////////////////////////////////////////////////////////
// Function: main                                                           //
// ===================================================================      //
// Input: int argc -> Number of command-line arguments                      //
//        char* argv[] -> Array of command-line arguments                   //
// Output: int -> Return code of the program                                //
// Purpose: Server-side implementation to listen for incoming connections   //
//          and process commands from clients and send the result to client.//
//          Also, stores and prints information about currently connected   //
//          clients and manages signals for specific situations.            //
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Port number required.\n");
        return 1;
    }
    signal(SIGINT, sh_int);

    open_logfile(); //open logfile

    write_log("Server program is started.\n");  // Log server start
    
    struct sockaddr_in server_addr, client_addr, data_addr;
    socklen_t len = sizeof(client_addr);

    

    // Set up the server address structure
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));
    
    
    
    // Bind the socket to the address
    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
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
        int connfd = accept(server_fd, (struct sockaddr*) &client_addr, &len);
        

    
        strcpy(client_IP, inet_ntoa(client_addr.sin_addr));
        client_PORT = server_addr.sin_port;

        // // Display client information
        // client_info(&client_addr);
        
        char client_ip[MAX_BUF];
        // Convert client IP address to string and store it in client_ip
        snprintf(client_ip, sizeof(client_ip), "%s", inet_ntoa(client_addr.sin_addr));
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
            write(connfd, "REJECTION", strlen("REJECTION"));
            printf("** It is NOT authenticated client **\n");
            close(connfd);  
            continue;
        } else {
            // If the client IP is allowed, send acceptance
            write(connfd, "ACCEPTED", strlen("ACCEPTED"));
            printf("** Client is connected **\n");
        }
        // Handle login authentication
        if(log_auth(connfd) == 0){  //if 3 times fail (ok : 1, fail : 0)
            printf("** Fail to log-in **\n");
            close(connfd);
            continue;

        }
        
        pid_t pid = fork();

        ////// Child process behavior //////
        if (pid == 0) { 
            close(server_fd);
            char buff[BUFFSIZE];

            //A loop that process FTP commands and transmits data to and from the client.
            while (1) {
                //Clear buffer to read command from client
                memset(buff, 0, BUFFSIZE);
                
                //Clear result_buff
                char result_buff[BUFFSIZE] = {0};
                
                //Read FTP command from the client
                int n = read(connfd, buff, BUFFSIZE);
                buff[n] = 0;
                strtok(buff,"\r\n");

                //if connection to the client is lost
                if (n <= 0) {
                    
                    printf("Client(%d)'s Release\n", getpid()); 
                    break; //Remove connection from the client
                }

            if (strncmp(buff, "PORT", 4) == 0) {
                unsigned int data_port;
                printf("%s", buff);
                char *client_ip = convert_str_to_addr(buff + 5, &data_port);    //convert form of IP address to four integer 

            // Setting data connection socket
            int data_sock = socket(AF_INET, SOCK_STREAM, 0);
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
                write(connfd, buff, sizeof(buff));
                close(data_sock);
                exit(1);
            }

            free(client_ip);

            // Response to PORT command
            strcpy(buff, "200 Port command performed successfully.\n");
            write_log("200 Port command performed successfully.\n");
            printf("%s", buff);
            write(connfd, buff, strlen(buff));

            // Waiting for NLST command
            memset(buff, 0, sizeof(buff));
            read(connfd, buff, sizeof(buff));

            printf("%s", buff);
            cmd_process(buff, result_buff);     //Process FTP command

            if (strncmp(buff, "NLST", 4) == 0) {
                strcpy(buff, "150 Opening data connection for directory list.\n");
                write_log("150 Opening data connection for directory list.\n");
                printf("%s", buff);
                write(connfd, buff, strlen(buff));   
                memset(buff, 0, sizeof(buff));   


                

                // Sending directory listing over data connection
                if(write(data_sock, result_buff, strlen(result_buff)) <0){
                    memset(buff, 0, sizeof(buff));
                    strcpy(buff, "550 Failed transmission.\n");
                    write_log(buff);
                    printf("%s", buff);
                    write(connfd, buff, sizeof(buff));
                    memset(buff, 0, sizeof(buff));
                }
                close(data_sock);
                
                
                 
                //Data connection success (COde 226)
                strcpy(buff, "226 Complete transmission.\n");
                write_log(buff);
                printf("%s", buff);
                write(connfd, buff, strlen(buff));
                
            }
            else if (strncmp(buff, "RETR", 4) == 0) {
                        char *filename = strtok(buff + 4, " \n");
                        handle_get_command(connfd, data_sock, filename);
                    }
            else if (strncmp(buff, "STOR", 4) == 0) {
                        char *filename = strtok(buff + 4, " \n");
                        handle_put_command(connfd, data_sock, filename);
                    }
            else {
                // Sending error message
                strcpy(buff, result_buff);
                write(connfd, buff, strlen(buff));
            }
        }

        //If command is not equal to "PORT" (not equal to "NLST", "STOR", "RETR")
        else 
        {   cmd_process(buff, result_buff);
            if(strncmp(result_buff, "221", 3) == 0){
                write(connfd, result_buff, strlen(result_buff));
                write_log(result_buff);
                close(connfd);
                exit(0);
            }
            write(connfd, result_buff, strlen(result_buff));
        }
            
            }
        }
    
}
    close_logfile();
    close(server_fd); // Close server socket
    return 0;
}

//A function to create data connection
int create_data_socket(struct sockaddr_in *data_addr) {
    int data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (data_sock < 0) {
        perror("Failed to create data socket");
        exit(EXIT_FAILURE);
    }

    if (bind(data_sock, (struct sockaddr *)data_addr, sizeof(*data_addr)) < 0) {
        perror("Failed to bind data socket");
        close(data_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(data_sock, 1) < 0) {
        perror("Failed to listen on data socket");
        close(data_sock);
        exit(EXIT_FAILURE);
    }

    return data_sock;
}

// Parse PORT command and create data socket
void handle_port_command(int client_sock, char *buff) {
    struct sockaddr_in data_addr;
    socklen_t data_addr_len;
    int data_sock, data_client_sock;
    int total_bytes = 0;
    unsigned int data_port;
    char ip_str[16];
    char port_buff[BUFFSIZE] = {0}, port_result[BUFFSIZE] = {0};
    char msg_buff[128] = {0}, cmd_buff[128] = {0};

    // Convert the string representation of the IP address to an actual address
    char *client_ip = convert_str_to_addr(buff + 5, &data_port);

    // Initialize the client address structure
            data_addr.sin_family = AF_INET;
            inet_pton(AF_INET, client_ip, &data_addr.sin_addr);
            data_addr.sin_port = htons(data_port);

    //Create data connection socket
    data_sock = create_data_socket(&data_addr);
     if (data_sock < 0) {
        strcpy(msg_buff, "425 Can't open data connection.\n");
        write_log(msg_buff);
        write(client_sock, msg_buff, strlen(msg_buff));
        free(client_ip);
        return;
    }


    // Send a 200 response to the client indicating success
    strcpy(msg_buff, "200 PORT command successful.\n");
    write_log(msg_buff);
    write(client_sock, msg_buff, strlen(msg_buff));
    printf("%s", msg_buff);
    memset(msg_buff, 0, BUFFSIZE);
    
    // Receive NLST command from the client
    read(client_sock, cmd_buff, BUFFSIZE - 1);
    printf("%s", cmd_buff);

    // Send a 150 response indicating that the data connection is opening
    strcpy(msg_buff, "150 Opening data connection.\n");
    write(client_sock, msg_buff, strlen(msg_buff));
    write_log(msg_buff);
    memset(msg_buff, 0, BUFFSIZE);

    // Wait for data connection from the client
    data_addr_len = sizeof(data_addr);
    data_client_sock = accept(data_sock, (struct sockaddr*)&data_addr, &data_addr_len);
     if (data_client_sock < 0) {
        perror("Failed to accept data connection");
        close(data_sock);
        free(client_ip);
        return;
    }
        // Process the NLST command and send the result over the data connection
        cmd_process(port_buff, port_result);
        write(data_client_sock, port_result, strlen(port_result));
    

    // Close the data connection
    close(data_client_sock);
    close(data_sock);

    // Send a 226 response indicating that the transfer is complete
    snprintf(msg_buff, BUFFSIZE, "226 Transfer complete.\n%d bytes is received.\n", total_bytes);
    write_log(msg_buff);
    write(client_sock, msg_buff, strlen(msg_buff));
    memset(msg_buff, 0, BUFFSIZE);
}

// Function to handle login authentication
int log_auth(int connfd){
    char user[MAX_BUF] = {0}, passwd[MAX_BUF] = {0};
    int n, count = 0;
    char msg_buff[128] = {0};

    // Three attempts allowed
    while (count < 3) {
        // Clear buffers
        memset(user, 0, MAX_BUF);
        memset(passwd, 0, MAX_BUF);

        // Read username and password from the client
        char user_input[MAX_BUF] = {0};
        char pass_input[MAX_BUF] = {0};

        n = read(connfd, user_input, MAX_BUF);
        user_input[n] = '\0';

        n = read(connfd, pass_input, MAX_BUF);
        pass_input[n] = '\0';

        printf("**User is trying to log-in (%d/3)**\n", count + 1);

        // Extract actual username and password from the input
        if (strncmp(user_input, "USER ", 5) == 0 && strncmp(pass_input, "PASS ", 5) == 0) {
            snprintf(user, MAX_BUF, "%s", user_input + 5);
            snprintf(passwd, MAX_BUF, "%s", pass_input + 5);

            strcpy(client_USER, user);

            // Check if the username and password match
            if (user_match(user, passwd) == 1) {
                write(connfd, "OK", 2);
                printf("**Success to log-in**\n");
                strcpy(msg_buff, "user ");
                strcat(msg_buff, user);
                strcat(msg_buff, " logged in.\n");
                write_log(msg_buff);
                return 1;
            } else {    // If username or password do not match
                count++;
                printf("**Log-in failed**\n");
                if (count < 3) {
                    write(connfd, "FAIL", 4);
                } else {
                    write(connfd, "DISCONNECTION", strlen("DISCONNECTION"));
                    write_log("illegal user connects to server.\n");
                    return 0;
                }
            }
        } else {
            // If the format is invalid
            count++;
            printf("**Invalid format**\n");
            if (count < 3) {
                write(connfd, "FAIL", 4);
            } else {
                write(connfd, "DISCONNECTION", strlen("DISCONNECTION"));
                write_log("illegal user connects to server.\n");
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

//A function to process corresponding FTP commands.
void cmd_process(const char *buff, char result_buff[BUFFSIZE]) {

    int aflag = 0, lflag = 0;
    char *directory = NULL;

    int opt;
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
    int i = 0;

    // Check for QUIT command     
    if (strncmp(argv[0], "QUIT", 4) == 0) {
        // If extra arguments are provided
        if(argc > 1){
            strcat(result_buff, "501 Syntax error in parameters or arguments.\n");
            write_log(result_buff);
            printf("%s", result_buff);
            free(buff_copy);
            return;
        }

        strcpy(result_buff, "221 Goodbye\n");  // Indicate program quit
    } 

    else if(strncmp(argv[0], "NLST", 4) == 0){

        int aflag =0, lflag = 0;  // Flags for options -a and -l


        // Handle 'NLST' command
        if(argv[1] == NULL && argv[2] == NULL){
            // Print the command and call NLST function without any options
            NLST(argv, aflag, lflag, result_buff);
        }
        // If the first argument starts with '-' (indicating an option)
        
        else if(argv[1][0] == '-'){
        // Check for various option combinations
        if(strncmp(argv[1], "-al", 3) == 0 || strncmp(argv[1], "-la", 3) == 0){
            aflag++;
            lflag++;
            NLST(argv, aflag, lflag, result_buff);    // Call NLST function with -a and -lflag
        }
        
        else if(strncmp(argv[1], "-a", 2) == 0){
            aflag++;    // Set -a flag
            NLST(argv, aflag, lflag, result_buff);    // Call NLST function with -a flag
        }
        else if(strncmp(argv[1],"-l", 2) == 0){
            lflag++;    // Set -l flag
            NLST(argv, aflag, lflag, result_buff);    // Call NLST function with -l flag              
        }
        
        // If an invalid option is provided
        else {
            strcpy(result_buff, "501 Syntax error in parameters or arguments.\n");
            write_log(result_buff);
            printf("%s", result_buff);
            free(buff_copy);
            return;
        }
        }
        // If options are not provided and the first argument is not an option
        else if (argv[1] != NULL && argv[1][0] != '-'){
            NLST(argv, aflag, lflag, result_buff);
            
        }
        
    }

    // If the command is "LIST"
    else if(strncmp(argv[0], "LIST", 4) == 0){


        // Check for options
        if (argv[1] != NULL) {
            if(argv[1][0] == '-'){
            // Option provided
            strcpy(result_buff, "501 Syntax error in parameters or arguments.\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
            }
            else {
                NLST(argv, 1, 1, result_buff);    //Call NLST with options of -a and -l
            }
        }
        else {
            NLST(argv, 1, 1, result_buff);  if (!token) return;  //Call NLST with options of -a and -l
        }
    }
    // Check if command == PWD
    else if(strncmp(argv[0], "PWD", 3) == 0){
        // Check if the argument is inputted
        if(argc != 1){
            if(argv[1][0] == '-'){

            //Check if the option is inputted
            strcpy(result_buff, "501 Syntax error in parameters or arguments.\n");
            write_log(result_buff);
            printf("%s", result_buff);
            }
            else {
                strcpy(result_buff, "501 Syntax error in parameters or arguments.\n");
                write_log(result_buff);
                printf("%s", result_buff);
            }
        }

        else {
            //Call PWD function
            PWD(result_buff);
        }

    }

    // Check if command is RNFR followed by RNTO
    else if (strncmp(argv[0], "RNFR", 4) == 0) {
    if (argv[1][0] == '-') {
        // Check if an invalid option is inputted
        strcpy(result_buff, "Error: invalid option\n");
        printf("%s", result_buff);
        return;
    } 
    // Check if there are not enough arguments
    if (argv[1] == NULL || argv[3] == NULL) {
        // Add "REQUIRE ARG" error message if there are not enough arguments
        strcpy(result_buff, "Error: two arguments are required\n");
        printf("%s", result_buff);
        return;
    }
    // Two arguments are inputted -> Call RENAME function
    RENAME(argv, result_buff);
    }



    //Check if command is CWD
	else if (strncmp(argv[0], "CWD", 3) == 0) {
        //Check if argument of CWD command is inputted
        if(argv[1] != NULL){
        //Check if the option is inputted
        if(argv[1][0] == '-'){
            strcpy(result_buff, "501 invalid option\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
            }
            else {
                CD(argv, result_buff);    //Call CD function
        }
        }
        else {
            strcpy(result_buff, "501 argument is required\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
        }
            
    }
    //Check if command is CDUP
	else if (strncmp(argv[0], "CDUP", 4) == 0) {
        
        argv[1] = "..";

       
            CD(argv, result_buff);
        
        
    }

    //Check if command is MKD
	else if(strncmp(argv[0], "MKD", 3) == 0) {
        //Check if argument of MKD command is inputted
        if(argv[1] != NULL){

        //Check if the option is inputted
        if(argv[1][0] == '-'){
            strcpy(result_buff, "501 invalid option\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
            }

    	else MKD(argv, result_buff);  //Call MKD function
        }
        else {
            strcpy(result_buff, "501 argument is required\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
        }
	}

    //Check if command is RMD
    else if(strncmp(argv[0], "RMD", 3) == 0){
        //Check if argument of RMD command is inputted
        if(argv[1] != NULL){
        //Check if the option is inputted
        if(argv[1][0] == '-'){
            strcpy(result_buff, "501 invalid option\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
            }

    	else RMD(argv, result_buff);  //Call RMD function
        }
        else {
            strcpy(result_buff, "501 argument is required\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
        }
    }

    //Check if command is DELE
    else if(strncmp(argv[0], "DELE", 4) == 0){
        //Check if argument of DELE command is inputted
        if(argv[1] != NULL){
        //Check if the option is inputted
        if(argv[1][0] == '-'){
            strcpy(result_buff, "501 invalid option\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
            }

    	else DELE(argv, result_buff); //Call DELE function
        }
        else {
            strcpy(result_buff, "501 argument is required\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
        }
    }
    //Check if command is TYPE ascii / binary
    else if(strncmp(argv[0], "TYPE", 4) == 0){

        //if command is TYPE binary
        if (strncmp(argv[1], "I", 1) == 0) {
                type = BIN_MODE;
                strcpy(result_buff, "201 Type set to I.\n");
                printf("%s", result_buff);
                write_log(result_buff);
            }
            //else if command is TYPE ascii
            else if (strncmp(argv[1], "A", 1) == 0){
                type = ASCII_MODE;
                strcpy(result_buff, "201 Type set to A.\n");
                printf("%s", result_buff);
                write_log(result_buff);
            }

    }

    else {
        strcpy(result_buff, "500 Unknown command\n"); // Unknown command response
        write_log(result_buff);
        printf("%s", result_buff);
     }
    
    free(buff_copy); // Free the copy of buff
}

//A function to process get command
void handle_get_command(int connfd, int data_sock, char *filename) {
    char buff[BUFFSIZE] ={0};   // Buffer for reading and writing data

    // Open the file for reading
    int file = open(filename, FLAGS, type);
    if (file < 0) {
        snprintf(buff, sizeof(buff), "550 Failed to open file.\n");
        write_log(buff);    // Log the error
        write(connfd, buff, strlen(buff));
        return;
    }
    // Send a message to the client indicating the type of data connection
    if(type == ASCII_MODE) {
        snprintf(buff, sizeof(buff), "150 Opening ASCII mode data connection for %s.\n", filename);
    }
    else {
        snprintf(buff, sizeof(buff), "150 Opening BINARY mode data connection for %s.\n", filename);
    }
    write_log(buff);    // Log the message
    write(connfd, buff, strlen(buff));  // Send the message to the client
    
    ssize_t n;  // Number of bytes read
    size_t total_bytes_sent = 0;

    // Read the file and send it to the client
    while ((n = read(file, buff, sizeof(buff))) > 0) {
        if (write(data_sock, buff, n) != n) {
            snprintf(buff, sizeof(buff), "550 Failed transmission.\n");
            write_log(buff);
            write(connfd, buff, strlen(buff));
            close(file);
            close(data_sock);   // Close the data socket
            return;
        }
        total_bytes_sent += n;  // Increment the total bytes sent
    }

    close(file);
    close(data_sock);

    snprintf(buff, sizeof(buff), "226 Transfer complete.\n");
    write_log(buff);    // Log the completion message
    write(connfd, buff, strlen(buff));  // Notify the client of the completion

    // Display the number of bytes sent to the client
    snprintf(buff, sizeof(buff), "OK. %zu bytes is sent.\n", total_bytes_sent);
    write(connfd, buff, strlen(buff));  // Send the byte count to the client
    write_log(buff);
}

//A function to process put command
void handle_put_command(int connfd, int data_sock, char *filename) {
    char buff[BUFFSIZE] ={0};

    // Open the file for writing (creating if necessary)
    int file = open(filename, FLAGS, type);
    if (file < 0) {
        snprintf(buff, sizeof(buff), "550 Failed to create file.\n");
        write(connfd, buff, strlen(buff));
        write_log(buff);    // Log the error
        return;
    }
    
    // Send a message to the client indicating the type of data connection
    if(type == ASCII_MODE) {
        snprintf(buff, sizeof(buff), "150 Opening ASCII mode data connection for %s.\n", filename);
    }
    else {
        snprintf(buff, sizeof(buff), "150 Opening BINARY mode data connection for %s.\n", filename);
    }
    write_log(buff);    // Log the message
    write(connfd, buff, strlen(buff));  // Send the message to the client
    
    ssize_t n;
    size_t total_bytes_received = 0;    // Total bytes received from the client

    // Read data from the client and write it to the file
    while ((n = read(data_sock, buff, sizeof(buff))) > 0) {
        if (write(file, buff, n) != n) {
            snprintf(buff, sizeof(buff), "550 Failed transmission.\n");
            write_log(buff);
            write(connfd, buff, strlen(buff));  // Notify the client of the error
            close(file);
            close(data_sock);
            return;
        }
        total_bytes_received += n;  // Increment the total bytes received
    }

    //Close the file and data connection
    close(file);
    close(data_sock);

    snprintf(buff, sizeof(buff), "226 Transfer complete.\n");
    write(connfd, buff, strlen(buff));
    write_log(buff);

    // Display the number of bytes received from the client
    snprintf(buff, sizeof(buff), "OK. %zu bytes is received.\n", total_bytes_received);
    write(connfd, buff, strlen(buff));
    write_log(buff);
}


//A function to process for "NLST" command
void NLST(char * argv[128], int aflag, int lflag, char result_buff[BUFFSIZE]) {

    
    char *directory = ""; // Determine the directory to list

    if (argv[1] == NULL && argv[2] == NULL) {
            // If no directory is specified or there's only an option without a directory
            directory = getcwd(NULL , 0); // Use the current directory
        } 
        else if(argv[1] != NULL && argv[1][0] == '-' && argv[2] != NULL){
            // If the directory is specified after an option
            directory = argv[2];
        }
        else if(argv[1] != NULL && argv[1][0] == '-' && argv[2] == NULL){
            directory = getcwd(NULL, 0);
        
        }
        else if(argv[1] != NULL && argv[1][0] != '-'){
            // If the directory is directly specified
            directory = argv[1];
        }
    struct stat file_stat;


    // Check if the specified path is a valid directory or file
    if (stat(directory, &file_stat) == -1) {
        strcat(result_buff, "550: No such file or directory\n");
        write_log(result_buff);
        printf("%s", result_buff);
        return;
    }

    if (S_ISDIR(file_stat.st_mode)) {
        // The input is a directory
        DIR *dir;
        struct dirent **namelist;
        struct dirent *entry;
        struct passwd *pwd;
        struct group *grp;
        struct tm *tm;
        char path[1024];

        // Check if the directory can be accessed
        if (access(directory, R_OK) == -1) {
            strcat(result_buff, "550: cannot access\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
        }

        // Open the directory
        dir = opendir(directory);
        if (dir == NULL) {
            strcat(result_buff, "550: failed to open directory\n");
            write_log(result_buff);
            printf("%s", result_buff);
            return;
        }

        // Count the number of valid entries
        int n = 0;
        while ((entry = readdir(dir)) != NULL) {
            if (!aflag && entry->d_name[0] == '.') {
                continue; // Skip hidden files if -a is not set
            }
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
            if (!aflag && entry->d_name[0] == '.') {
                continue; // Skip hidden files
            }
            namelist[i++] = entry; // Add to the list
        }

        // Sort the entries
        bubblesort(namelist, n);

        // Iterate through the sorted entries to build the result buffer
        int file_count = 0;
        for (int j = 0; j < n; j++) {
            char *name = namelist[j]->d_name;

            sprintf(path, "%s/%s", directory, name);

            if (stat(path, &file_stat) == -1) {
                strcat(result_buff, "550: No such file or directory\n");
                write_log(result_buff);
                printf("%s", result_buff);
                continue;
            }

            if (lflag) {

                // Detailed listing if -l is set
                strcat(result_buff, (S_ISDIR(file_stat.st_mode)) ? "d" : "-");
                strcat(result_buff, (file_stat.st_mode & S_IRUSR) ? "r" : "-");
                strcat(result_buff, (file_stat.st_mode & S_IWUSR) ? "w" : "-");
                strcat(result_buff, (file_stat.st_mode & S_IXUSR) ? "x" : "-");
                strcat(result_buff, (file_stat.st_mode & S_IRGRP) ? "r" : "-");
                strcat(result_buff, (file_stat.st_mode & S_IWGRP) ? "w" : "-");
                strcat(result_buff, (file_stat.st_mode & S_IXGRP) ? "x" : "-");
                strcat(result_buff, (file_stat.st_mode & S_IROTH) ? "r" : "-");
                strcat(result_buff, (file_stat.st_mode & S_IWOTH) ? "w" : "-");
                strcat(result_buff, (file_stat.st_mode & S_IXOTH) ? "x" : "-");

                // Number of hard links
                char link_count[20];
                sprintf(link_count, "\t%lu", file_stat.st_nlink);
                strcat(result_buff, link_count);

                // Owner and group information
                if ((pwd = getpwuid(file_stat.st_uid)) != NULL) {
                    strcat(result_buff, "\t");
                    strcat(result_buff, pwd->pw_name);
                } else {
                    char uid_str[20];
                    sprintf(uid_str, "\t%d", file_stat.st_uid);
                    strcat(result_buff, uid_str);
                }

                if ((grp = getgrgid(file_stat.st_gid)) != NULL) {
                    strcat(result_buff, "\t");
                    strcat(result_buff, grp->gr_name);
                } else {
                    char gid_str[20];
                    sprintf(gid_str, "\t%d", file_stat.st_gid);
                    strcat(result_buff, gid_str);
                }

                // File size and last modified time
                char size_str[20];
                sprintf(size_str, "\t%lld", (long long)file_stat.st_size);
                strcat(result_buff, size_str);

                tm = localtime(&file_stat.st_mtime);
                static const char *month_names[] = {
                    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                };
                char time_str[20];
                sprintf(time_str, "\t%s %02d %02d:%02d\t", month_names[tm-> tm_mon], tm-> tm_mday, tm-> tm_hour, tm-> tm_min);
                strcat(result_buff, time_str);

                // File or directory name
                strcat(result_buff, name);
                if (S_ISDIR(file_stat.st_mode)) {
                    strcat(result_buff, "/");
                }
                strcat(result_buff, "\n");
            } else {
                // Just the name if lflag is not set
                strcat(result_buff, name);
                if (S_ISDIR(file_stat.st_mode)) {
                    strcat(result_buff, "/");
                }
                strcat(result_buff, "\n");
            }

            file_count++;
        }

        strcat(result_buff, "\n");


        free(namelist);
        closedir(dir);
    } else {
        // If the input is a file
        if (lflag) {
            // File permissions and other details
            strcat(result_buff, (S_ISDIR(file_stat.st_mode)) ? "d" : "-");
            strcat(result_buff, (file_stat.st_mode & S_IRUSR) ? "r" : "-");
            strcat(result_buff, (file_stat.st_mode & S_IWUSR) ? "w" : "-");
            strcat(result_buff, (file_stat.st_mode & S_IXUSR) ? "x" : "-");
            strcat(result_buff, (file_stat.st_mode & S_IRGRP) ? "r" : "-");
            strcat(result_buff, (file_stat.st_mode & S_IWGRP) ? "w" : "-");
            strcat(result_buff, (file_stat.st_mode & S_IXGRP) ? "x" : "-");
            strcat(result_buff, (file_stat.st_mode & S_IROTH) ? "r" : "-");
            strcat(result_buff, (file_stat.st_mode & S_IWOTH) ? "w" : "-");
            strcat(result_buff, (file_stat.st_mode & S_IXOTH) ? "x" : "-");

            char link_count[20];
            sprintf(link_count, "\t%lu", file_stat.st_nlink);
            strcat(result_buff, link_count);

            struct passwd* pwd;

            // Owner and group information
                if ((pwd = getpwuid(file_stat.st_uid)) != NULL) {
                    strcat(result_buff, "\t");
                    strcat(result_buff, pwd->pw_name);
                } else {
                    char uid_str[20];
                    sprintf(uid_str, "\t%d", file_stat.st_uid);
                    strcat(result_buff, uid_str);
                }
            
            struct group* grp;

                if ((grp = getgrgid(file_stat.st_gid)) != NULL) {
                    strcat(result_buff, "\t");
                    strcat(result_buff, grp->gr_name);
                } else {
                    char gid_str[20];
                    sprintf(gid_str, "\t%d", file_stat.st_gid);
                    strcat(result_buff, gid_str);
                }

                // File size and last modified time
                char size_str[20];
                sprintf(size_str, "\t%lld", (long long)file_stat.st_size);
                strcat(result_buff, size_str);

                struct tm *tm;
                tm = localtime(&file_stat.st_mtime);
                static const char *month_names[] = {
                    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                };
                char time_str[20];
                sprintf(time_str, "\t%s %02d %02d:%02d\t", month_names[tm-> tm_mon], tm-> tm_mday, tm-> tm_hour, tm-> tm_min);
                strcat(result_buff, time_str);

                strcat(result_buff, directory);
                strcat(result_buff, "\n");
        } 
        else {
            // Just the name if lflag is not set
            strcat(result_buff, directory);
            strcat(result_buff, "\n");
        }
    }
}


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

// Function to print the current working directory
void PWD(char result_buff[BUFFSIZE]) {

    char cwd[256] = {0}; 
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        strcpy(result_buff, "257 "); 
        strcat(result_buff, "\"");
        strcat(result_buff, cwd);
        strcat(result_buff, "\" is current directory\n");
        printf("%s", result_buff);
        write_log(result_buff);
    } else {
        perror("getcwd error"); 
    }
}

//A function to renmae files
void RENAME(char *argv[128], char result_buff[BUFFSIZE]) {

    // Check if directory exists and has write permission
    if (access(argv[1], F_OK) == 0 && access(argv[1], W_OK) == -1) {
        strcpy(result_buff, "550 Can't find such file or directory.\n");
        write_log(result_buff);
        printf("%s", result_buff);
        return;
    }

    // Check if the file to be renamed exists
    if (access(argv[1], F_OK) == -1) {
        strcpy(result_buff, "550 Can't find such file or directory.\n");
        write_log(result_buff);
        printf("%s", result_buff);
        return;
    }

    // Check if the new name already exists
    if (access(argv[3], F_OK) == 0) {
        strcpy(result_buff, "550 Can't be renamed.\n");
        write_log(result_buff);
        printf("%s", result_buff);
        return;
    }

    // Rename the file or directory
    if (rename(argv[1], argv[3]) == -1) {
        strcpy(result_buff, "501 Two arguments are required\n");
        write_log(result_buff);
        printf("%s", result_buff);
    } else {
        // Print the old and new names
        strcpy(result_buff, "350 File exists, ready to rename\n");
        write_log(result_buff);
        strcat(result_buff, "250 RNTO command succeeds.\n");
        printf("%s", result_buff);
        write_log("250 RNTO command succeeds.\n");
    }
    return;
}

//A function to change the current working directory.
void CD(char **path, char result_buff[BUFFSIZE]) {

    strtok(path[1], "\n");
    

    // Check if directory exists and has read permission
    if (access(path[1], F_OK) == 0 && access(path[1], R_OK) == -1) {
        strcpy(result_buff, "550 ");
        strcat(result_buff, path[1]);
        strcat(result_buff, ": Can't find such file or directory.\n");
        printf("%s", result_buff);
        write_log(result_buff);
        return;
    }

    // Change directory
    if (chdir(path[1]) == -1) {
        strcpy(result_buff, "550 ");
        strcat(result_buff, path[1]);
        strcat(result_buff, ": Can't find such file or directory.\n");
        write_log(result_buff);
        printf("%s", result_buff);
    } 

        char cwd[1024]; // Buffer to store the new current working directory path

        // Get the new current working directory path
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            if(strcmp(path[0], "CWD") == 0){
                // Print the command and the new current working directory path
                strcpy(result_buff, "250 CWD command succeeds.\n");
                printf("%s", result_buff);
                write_log(result_buff);
            }

            else if(strncmp(path[0], "CDUP", 4) == 0){
                // Print the command
                strcpy(result_buff, "250 CDUP command succeeds.\n");
                printf("%s", result_buff);
                write_log(result_buff);
            }
        }
    
}

// Function to create directories
void MKD(char **dirnames, char result_buff[BUFFSIZE]) {
    int i = 1;
    char success_msg[100] = {0};
    char error_msg[100] = {0};

    // Loop through each directory name provided
    while (dirnames[i] != NULL) {
        char *dirname = dirnames[i];
        if (strcmp(dirname, "") == 0) {
            // Print error if directory name is empty
            strcpy(result_buff, "501 argument is required\n");
            write_log(result_buff);
            printf("%s", result_buff);
        } else {

            // Attempt to create the directory
            if (mkdir(dirname, 0775) == -1) {
                if (errno == EEXIST) {
                    // Print error if directory already exists
                    sprintf(error_msg, "550 %s: can't create directory.\n", dirname);
                    strcat(result_buff, error_msg);
                    printf("%s", error_msg);
                    write_log(result_buff);
                }   
            } 
            else {
                // Print success message if directory is created successfully
                strcat(result_buff, "250 MKD command performed successfully.\n");
                write_log("250 MKD command performed successfully.\n");
                printf("250 MKD command performed successfully.\n");
            }
        }
        i++;
    }


    return;
}

// Function to remove directories
void RMD(char **dirnames, char result_buff[BUFFSIZE]) {
    int i = 1;

    char error_msg[100] = {0};
    char success_msg[100] = {0};

    // Loop through each directory name provided
    while (dirnames[i] != NULL) {
        char *dirname = dirnames[i];
        memset(result_buff, 0, BUFFSIZE);
        if (strcmp(dirname, "") == 0) {
            // Print error if directory name is empty
            strcpy(result_buff, "501 argument is required\n");
            write_log(result_buff);
            printf("%s", result_buff);
        } 
        
        else {
            // Check if directory exists and has write permission
            if (access(dirnames[i], F_OK) == 0 && access(dirnames[i], W_OK) == -1) {
                // Print error if directory cannot be accessed
                strcat(result_buff, "550 ");
                strcat(result_buff, dirname);
                strcat(result_buff, ": can't remove directory.\n");
                write_log(result_buff);
                printf("%s", result_buff);
            }

            // Attempt to remove the directory
            if (rmdir(dirname) == -1) {
                if (errno == ENOENT) {
                    // Print error if directory does not exist
                    
                    // Print error if directory already exists
                    sprintf(error_msg, "550 %s: can't remove directory.\n", dirnames[i]);
                    strcat(result_buff, error_msg);
                    printf("%s", error_msg);
                    write_log(result_buff);
                } 
            } 
            else {
                // Print success message if directory is removed successfully
                
                strcat(result_buff, "250 RMD command performed successfully.\n");
                write_log("250 RMD command performed successfully.\n");
                printf("250 RMD command performed successfully.\n");
            }
        }
        
        i++;
    }
    return;
}

// Function to delete files
void DELE(char **filenames, char result_buff[BUFFSIZE]) {
    int i = 1;
    char error_msg[100] = {0};
    char success_msg[50] = {0};

    // Loop through each filename provided
    while (filenames[i] != NULL) {
        char *filename = filenames[i];
        if (strcmp(filename, "") == 0) {
            // Print error if filename is empty
            strcpy(result_buff, "501 argument is required\n");
            write_log(result_buff);
            printf("%s", result_buff);
        } else {


            // Check if directory exists and has write permission
            if (access(filenames[i], F_OK) == 0 && access(filenames[i], W_OK) == -1) {
                strcat(result_buff, "550 ");
                strcat(result_buff, filenames[i]);
                strcat(result_buff, ": can't find such file or directory.\n");
                write_log(result_buff);
                printf("%s", result_buff);
            }

            // Attempt to delete the file
            if (unlink(filename) == -1) {
                if (errno == ENOENT) {
                    // Print error if file does not exist
                    
                    sprintf(error_msg, "550 %s: can't find such file or directory.\n", filenames[i]);
                    strcat(result_buff, error_msg);
                    printf("%s", error_msg);
                    write_log(result_buff);
                }
            } 
            else {
                // Print success message if file is deleted successfully
                strcat(result_buff, "250 DELE command performed successfully.\n");
                write_log("250 DELE command performed successfully.\n");
                printf("250 DELE command performed successfully.\n");
            }
        }
        i++;
    }
    return;
}

