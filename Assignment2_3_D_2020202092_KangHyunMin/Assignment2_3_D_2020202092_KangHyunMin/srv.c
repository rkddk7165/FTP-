///////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                 //
// Date : 2024/05/16                                                 //
// OS : Ubuntu 20.04.6 LTS 64bits                                    //
//                                                                   //
// Author : Kang Hyun Min                                            //
// Student ID : 2020202092                                           //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #2-3 ( ftp server )         //
// Description : The purpose of this file is to read FTP-COMMAND     //
// data from client and perform operations according to this command,//                     
// and send the result to client. Also, this file stores and prints  //
// information about currently connected clients and                 //
// manages signals for specific situations.                          //          
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

#define BUFFSIZE 4096

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


void sh_chld(int); // SIGCHLD handler function
void sh_alrm(int); // SIGALRM handler function
void sh_int(int);  // SIGINT handler function

//////////////////////////////////////////////////////////////////////////
// Function: client_info                                                //
// ===================================================================  //
// Input: struct sockaddr_in* cliaddr -> Client address structure       //
// Output: None                                                         //
// Purpose: Prints the client information, including IP and port number.//
//////////////////////////////////////////////////////////////////////////
void add_process_info(pid_t pid, int port);

//////////////////////////////////////////////////////////////////////////
// Function: client_info                                                //
// ===================================================================  //
// Input: struct sockaddr_in* cliaddr -> Client address structure       //
// Output: None                                                         //
// Purpose: a function to remove process information                    //
//          when process is terminated                                  //        
//////////////////////////////////////////////////////////////////////////
void remove_process_info(pid_t pid);

///////////////////////////////////////////////////////////////////////////
// Function: print_process_info                                          //
// ===================================================================   //
// Input: None                                                           //
// Output: None                                                          //
// Purpose: A function to print process information                      //
///////////////////////////////////////////////////////////////////////////
void print_process_info(); 

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

//////////////////////////////////////////////////////////////////////////
// Function: client_info                                                //
// ===================================================================  //
// Input: struct sockaddr_in* cliaddr -> Client address structure       //
// Output: None                                                         //
// Purpose: Prints the client information, including IP and port number.//
//////////////////////////////////////////////////////////////////////////
void client_info(const struct sockaddr_in *cliaddr) {
    printf("=========Client info=========\n");
    printf("Client IP: %s\n", inet_ntoa(cliaddr->sin_addr));
    printf("Client port: %d\n", ntohs(cliaddr->sin_port));
    printf("=============================\n");
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
    
    
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    //Applying SIGCHLD, SIGALRM, SIGINT signal handlers
    signal(SIGCHLD, sh_chld);
    signal(SIGALRM, sh_alrm);
    signal(SIGINT, sh_int);

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

    alarm(10); // Timer to output process information at 10 second intervals

    // Accept incoming connections
    while (1) {
        //Accept client connection request
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        //Create new child process
        pid_t pid = fork();

        ////// Child process behavior //////
        if (pid == 0) { 
            close(server_fd);
            add_process_info(getpid(), ntohs(client_addr.sin_port)); // Store process information
            printf("Child Process PID : %d\n", getpid());
            print_process_info();
            alarm(0); // if new client is connected, reset alarm
            char buff[BUFFSIZE];

            //A loop that process FTP commands and transmits data to and from the client.
            while (1) {
                //Clear buffer to read command from client
                memset(buff, 0, BUFFSIZE);
                
                //Clear result_buff
                char result_buff[BUFFSIZE] = {0};
                
                //Read FTP command from the client
                int n = read(client_fd, buff, BUFFSIZE);

                //if connection to the client is lost
                if (n <= 0) {
                    
                    printf("Client(%d)'s Release\n", getpid()); 
                    break; //Remove connection from the client
                }

                if (strncmp(buff, "QUIT", 4) == 0) { // if FTP command is "QUIT"
                    printf("Client(%d)'s Release\n", getpid()); //Print client release message
                    remove_process_info(getpid()); // Remove process information
                    close(client_fd);
                    exit(0); // Process exit
                }

                printf("%s  [%d]\n", buff, getpid());
                //call cmd_process function to perform behaviors for the command
                cmd_process(buff, result_buff);


                // Send result_buff to client
                write(client_fd, result_buff, strlen(result_buff)); 

                //Clear result_buff for next command behavior
                memset(result_buff, 0, BUFFSIZE);
            }
            // Remove process information before terminating the connection
            remove_process_info(getpid()); 
            close(client_fd);
            exit(0); // end process
        
        }
        ///// Parent Process /////
        else if (pid > 0) { 
            client_info(&client_addr); // Print client information
            add_process_info(pid, ntohs(client_addr.sin_port)); // Add process information
            alarm(10); // Timer to output process information at 10 second intervals
            close(client_fd);
        } else {
            perror("Fork failed");
            close(client_fd); // Close client socket when fork fails
        }
    }

    close(server_fd); // Close server socket
    return 0;
}

//A function to add process information when a new process is created.
void add_process_info(pid_t pid, int port) {
    if (process_count < MAX_PROCESS) {
        process_list[process_count].pid = pid;
        process_list[process_count].client_port = port;
        process_list[process_count].start_time = time(NULL);
        process_count++;
    }
}

//A function to remove process information when process is terminated.
void remove_process_info(pid_t pid) {
    int i;
    for (i = 0; i < process_count; i++) {
        if (process_list[i].pid == pid) {
            break;
        }
    }

    if (i < process_count) {
        for (int j = i; j < process_count - 1; j++) {
            process_list[j] = process_list[j + 1];
        }
        process_count--;
    }
}

//A function to print process information every 10 seconds.
void print_process_info() {
    printf("Current Number of Client: %d\n", process_count); 
    printf("PID      PORT     TIME\n");
    for (int i = 0; i < process_count; i++) {
        printf("%-8d %-8d %-8d\n", 
               process_list[i].pid,
               process_list[i].client_port,
               (int)difftime(time(NULL), process_list[i].start_time));
    }
}

//SIGCHLD signal handler function
void sh_chld(int signum) {
    pid_t terminated_pid;
    while ((terminated_pid = waitpid(-1, NULL, WNOHANG)) > 0) { 
        remove_process_info(terminated_pid); // Remove terminated process information
    }
}

//SIGALRM signal handler function
void sh_alrm(int signum) {
    print_process_info(); // print process information every 10 seconds
    alarm(10); // reset 10 seconds timer
}

//SIGINT signal handler function
void sh_int(int signum) {
    printf("\nThe server has shut down.\n");

    // all of processes is terminated
    for (int i = 0; i < process_count; i++) {
        kill(process_list[i].pid, SIGINT); // terminate processes.
    }

    close(server_fd);

    exit(0); // Server exit
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


    
    // Check for QUIT command
    if (strncmp(argv[0], "QUIT", 4) == 0) {
        // If extra arguments are provided
        if(argc > 1){
            strcat(result_buff, "Error: invalid option\n");
            printf("%s", result_buff);
            free(buff_copy);
            return;
        }

        strcpy(result_buff, "Program quit!!\n");  // Indicate program quit
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
        if(strcmp(argv[1], "-a") == 0){
            aflag++;    // Set -a flag
            NLST(argv, aflag, lflag, result_buff);    // Call NLST function with -a flag
        }
        else if(strcmp(argv[1],"-l") == 0){
            lflag++;    // Set -l flag
            NLST(argv, aflag, lflag, result_buff);    // Call NLST function with -l flag              
        }
        else if(strcmp(argv[1], "-al") == 0 || strcmp(argv[1], "-la") == 0){
            aflag++;
            lflag++;
            NLST(argv, aflag, lflag, result_buff);    // Call NLST function with -a and -lflag
        }

        // If an invalid option is provided
        else {
            strcat(result_buff, "Error: invalid option\n");
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
    else if(strcmp(argv[0], "LIST") == 0){


        // Check for options
        if (argv[1] != NULL) {
            if(argv[1][0] == '-'){
            // Option provided
            strcpy(result_buff, "Error: invalid option\n");
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
    else if(strcmp(argv[0], "PWD") == 0){
        // Check if the argument is inputted
        if(argc != 1){
            if(argv[1][0] == '-'){

            //Check if the option is inputted
            strcpy(result_buff, "Error: invalid option\n");
            printf("%s", result_buff);
            }
            else {
                strcpy(result_buff, "Error: argument is not required\n");
                printf("%s", result_buff);
            }
        }

        else {
            //Call PWD function
            PWD(result_buff);
        }

    }

    // Check if command is RNFR followed by RNTO
    else if (strcmp(argv[0], "RNFR") == 0) {
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
	else if (strcmp(argv[0], "CWD") == 0) {
        //Check if argument of CWD command is inputted
        if(argv[1] != NULL){
        //Check if the option is inputted
        if(argv[1][0] == '-'){
            strcpy(result_buff, "Error: invalid option\n");
            printf("%s", result_buff);
            return;
            }
            else {
                CD(argv, result_buff);    //Call CD function
        }
        }
        else {
            strcpy(result_buff, "argument is required\n");
            printf("%s", result_buff);
            return;
        }
            
    }
    //Check if command is CDUP
	else if (strcmp(argv[0], "CDUP") == 0) {
        
        argv[1] = "..";

       
            CD(argv, result_buff);
        
        
    }

    //Check if command is MKD
	else if(strcmp(argv[0], "MKD") == 0) {
        //Check if argument of MKD command is inputted
        if(argv[1] != NULL){

        //Check if the option is inputted
        if(argv[1][0] == '-'){
            strcpy(result_buff, "Error: invalid option\n");
            printf("%s", result_buff);
            return;
            }

    	else MKD(argv, result_buff);  //Call MKD function
        }
        else {
            strcpy(result_buff, "Error: argument is required\n");
            printf("%s", result_buff);
            return;
        }
	}

    //Check if command is RMD
    else if(strcmp(argv[0], "RMD") == 0){
        //Check if argument of RMD command is inputted
        if(argv[1] != NULL){
        //Check if the option is inputted
        if(argv[1][0] == '-'){
            strcpy(result_buff, "Error: invalid option\n");
            printf("%s", result_buff);
            return;
            }

    	else RMD(argv, result_buff);  //Call RMD function
        }
        else {
            strcpy(result_buff, "Error: argument is required\n");
            printf("%s", result_buff);
            return;
        }
    }

    //Check if command is DELE
    else if(strcmp(argv[0], "DELE") == 0){
        //Check if argument of DELE command is inputted
        if(argv[1] != NULL){
        //Check if the option is inputted
        if(argv[1][0] == '-'){
            strcpy(result_buff, "Error: invalid option\n");
            printf("%s", result_buff);
            return;
            }

    	else DELE(argv, result_buff); //Call DELE function
        }
        else {
            strcpy(result_buff, "Error: argument is required\n");
            printf("%s", result_buff);
            return;
        }
    }

    else {
        strcpy(result_buff, "Unknown command\n"); // Unknown command response
        printf("%s", result_buff);
     }
    
    free(buff_copy); // Free the copy of buff
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
        strcat(result_buff, "Error: No such file or directory\n");
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
            strcat(result_buff, "Error: cannot access\n");
            printf("%s", result_buff);
            return;
        }

        // Open the directory
        dir = opendir(directory);
        if (dir == NULL) {
            strcat(result_buff, "Error: failed to open directory\n");
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
                strcat(result_buff, "Error: No such file or directory\n");
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
                strcat(result_buff, "\t");
            }

            file_count++;

            if (!lflag && file_count % 5 == 0) {
                strcat(result_buff, "\n");
            }
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
        strcpy(result_buff, "\"");
        strcat(result_buff, cwd);
        strcat(result_buff, "\" is current directory\n");
    } else {
        perror("getcwd error"); 
    }
}

//A function to renmae files
void RENAME(char *argv[128], char result_buff[BUFFSIZE]) {

    // Check if directory exists and has write permission
    if (access(argv[1], F_OK) == 0 && access(argv[1], W_OK) == -1) {
        strcpy(result_buff, "Error: cannot access\n");
        printf("%s", result_buff);
        return;
    }

    // Check if the file to be renamed exists
    if (access(argv[1], F_OK) == -1) {
        strcpy(result_buff, "Error: File '");
        strcat(result_buff, argv[1]);
        strcat(result_buff, "' does not exist.\n");
        printf("%s", result_buff);
        return;
    }

    // Check if the new name already exists
    if (access(argv[3], F_OK) == 0) {
        strcpy(result_buff, "Error: name to change already exists\n");
        printf("%s", result_buff);
        return;
    }

    // Rename the file or directory
    if (rename(argv[1], argv[3]) == -1) {
        strcpy(result_buff, "two arguments are required\n");
        printf("%s", result_buff);
    } else {
        // Print the old and new names
        strcpy(result_buff, "RNFR ");
        strcat(result_buff, argv[1]);
        strcat(result_buff, "\nRNTO ");
        strcat(result_buff, argv[3]);
        strcat(result_buff, "\n");
    }
    return;
}

//A function to change the current working directory.
void CD(char **path, char result_buff[BUFFSIZE]) {

    // Check if directory exists and has read permission
    if (access(path[1], F_OK) == 0 && access(path[1], R_OK) == -1) {
        strcpy(result_buff, "Error: cannot access\n");
        printf("%s", result_buff);
        return;
    }

    // Change directory
    if (chdir(path[1]) == -1) {
        strcpy(result_buff, "Error: directory not found\n");
        printf("%s", result_buff);
    } else {

        char cwd[1024]; // Buffer to store the new current working directory path

        // Get the new current working directory path
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            if(strcmp(path[0], "CWD") == 0){
                // Print the command and the new current working directory path
                strcpy(result_buff, "CWD ");
                strcat(result_buff, path[1]);
                strcat(result_buff, "\n");
            }

            else if(strcmp(path[0], "CDUP") == 0){
                // Print the command
                strcpy(result_buff, "CDUP\n");
            }
            // Print the new current working directory path
            strcat(result_buff, "\"");
            strcat(result_buff, cwd);
            strcat(result_buff, "\" is current directory\n");
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
            strcpy(result_buff, "Error: argument is required\n");
            printf("%s", result_buff);
        } else {

            // Attempt to create the directory
            if (mkdir(dirname, 0775) == -1) {
                if (errno == EEXIST) {
                    // Print error if directory already exists
                    sprintf(error_msg, "Error: cannot create directory '%s': File exists\n", dirname);
                    strcat(result_buff, error_msg);
                }
            } 
            else {
                // Print success message if directory is created successfully
                sprintf(success_msg, "MKD %s\n", dirname);
                strcat(result_buff, success_msg);
                
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
        if (strcmp(dirname, "") == 0) {
            // Print error if directory name is empty
            strcpy(result_buff, "Error: argument is required\n");
            printf("%s", result_buff);
        } 
        
        else {
            // Check if directory exists and has write permission
            if (access(dirnames[i], F_OK) == 0 && access(dirnames[i], W_OK) == -1) {
                // Print error if directory cannot be accessed
                strcat(result_buff, "Error: cannot access\n");
                printf("%s", result_buff);
            }

            // Attempt to remove the directory
            if (rmdir(dirname) == -1) {
                if (errno == ENOENT) {
                    // Print error if directory does not exist
                    
                    sprintf(error_msg, "Error: failed to remove '%s'\n", dirname);
                    strcat(result_buff, error_msg);
                    printf("%s", error_msg);
                    
                    
                } 
            } 
            else {
                // Print success message if directory is removed successfully
                
                sprintf(success_msg, "RMD %s\n", dirname);
                strcat(result_buff, success_msg);
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
            strcpy(result_buff, "Error: argument is required\n");
            printf("%s", result_buff);
        } else {

            // Check if directory exists and has write permission
            if (access(filenames[i], F_OK) == 0 && access(filenames[i], W_OK) == -1) {
                strcat(result_buff, "Error: cannot access\n");
                printf("%s", result_buff);
            }

            // Attempt to delete the file
            if (unlink(filename) == -1) {
                if (errno == ENOENT) {
                    // Print error if file does not exist
                    
                    sprintf(error_msg, "Error: failed to delete '%s'\n", filename);
                    strcat(result_buff, error_msg);
                }
            } 
            else {
                // Print success message if file is deleted successfully
                
                sprintf(success_msg, "DELE %s\n", filename);
                strcat(result_buff, success_msg);
            }
        }
        printf("%s", error_msg);
        i++;
    }
    return;
}