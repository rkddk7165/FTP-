////////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                  //
// Date : 2024/04/30                                                  //
// OS : Ubuntu 20.04.6 LTS 64bits                                     //
//                                                                    //
// Author : Kang Hyun Min                                             //
// Student ID : 2020202092                                            //
// -----------------------------------------------------------------  //
// Title : System Programming Assignment #2-1 ( ftp server )          //
// Description : This file contains the code for the FTP server       //
//               program that handles client connections, processes   //
//               FTP commands, and returns the appropriate responses  //
//               to client.                                           //
////////////////////////////////////////////////////////////////////////
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
#include <pwd.h>
#include <grp.h>
#include <time.h>

//Buffer size for data communication
#define BUFFSIZE    4096

//////////////////////////////////////////////////////////////////////////
// Function: NLST                                                       //
// ===================================================================  //
// Input: char* argv[128] -> Array of command-line arguments            //
//        int aflag -> '-a' option flag (include hidden files)          //
//        int lflag -> '-l' option flag (detailed list)                 //
//        char* result_buff -> Buffer to store the directory listing    //
// Output: None                                                         //
// Purpose: Generates a directory listing based on the provided options.//
//////////////////////////////////////////////////////////////////////////
void NLST(char* argv[128], int aflag, int lflag, char* result_buff);


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
    write(1, ip_info, strlen(ip_info));

    char port_info[50];
    snprintf(port_info, sizeof(port_info), "Client port: %d\n", ntohs(cliaddr->sin_port));
    write(1, port_info, strlen(port_info));

    const char* info_footer = "=============================\n";
    write(1, info_footer, strlen(info_footer));
}



///////////////////////////////////////////////////////////////////////////
// Function: cmd_process                                                 //
// ===================================================================   //
// Input: const char* buff -> Command received from client               //
//        char* result_buff -> Buffer to store the command result        //
// Output: None                                                          //
// Purpose: Processes the command received from the client and populates //
//          result_buff with the appropriate response.                   //
///////////////////////////////////////////////////////////////////////////
void cmd_process(const char *buff, char *result_buff) {

    memset(result_buff, 0, BUFFSIZE); // Clear the result buffer

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
        else {strcat(result_buff, "Error: invalid option\n");
                     free(buff_copy);
                     return;
        }
        }
        // If options are not provided and the first argument is not an option
        else if (argv[1] != NULL && argv[1][0] != '-'){
            NLST(argv, aflag, lflag, result_buff);
            
        }
        
    }
    else {
        strcpy(result_buff, "Unknown command\n"); // Unknown command response
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

//////////////////////////////////////////////////////////////////////////
// Function: NLST                                                       //
// ===================================================================  //
// Input: char* argv[128] -> Array of command-line arguments            //
//        int aflag -> '-a' option flag (include hidden files)          //
//        int lflag -> '-l' option flag (detailed list)                 //
//        char* result_buff -> Buffer to store the directory listing    //
// Output: None                                                         //
// Purpose: Generates a directory listing based on the provided options.//
//////////////////////////////////////////////////////////////////////////
void NLST(char * argv[128], int aflag, int lflag, char *result_buff) {

    memset(result_buff, 0, BUFFSIZE); // Initialize the result buffer
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
            return;
        }

        // Open the directory
        dir = opendir(directory);
        if (dir == NULL) {
            strcat(result_buff, "Error: failed to open directory\n");
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


//////////////////////////////////////////////////////////////////////////////
// Function: main                                                           //
// ===================================================================      //
// Input: int argc -> Number of command-line arguments                      //
//        char* argv[] -> Array of command-line arguments                   //
// Output: int -> Return code of the program                                //
// Purpose: Server-side implementation to listen for incoming connections   //
//          and process commands from clients.                              //
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]){
    // Check for port number argument
    if (argc != 2) {
        write(1, "Port Number required.\n", strlen("Port Number required.\n"));
        return 1;
    }

    int port = atoi(argv[1]); // Convert to integer

    // Socket and network-related variables
    int socket_fd, client_fd;
    int len, len_out;
    struct sockaddr_in server_addr, client_addr;
    char buf[BUFFSIZE] ={}; // Buffer for communication
    char result_buff[BUFFSIZE] = {}; // Buffer to hold command results

    // Create the socket
    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        write(1, "Server: can't open stream socket.\n", strlen("Server: can't open stream socket.\n"));
        return 0;
    }

    // Set up the server address structure
    bzero ((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // Bind the socket to the address
    if(bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        write(1, "Server: Can't bind local address.\n", strlen("Server: Can't bind local address.\n"));
        return 0;
    }

    // Listen for incoming connections
    if (listen(socket_fd, 5) < 0) {
        write(1, "Server: Listen failed.\n", strlen("Server: Listen failed.\n"));
        close(socket_fd);
        return -1;
    }

    // Accept incoming connections
    while(1){
        len = sizeof(client_addr);
        client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &len);

        if(client_fd < 0){
            write(1, "Server: accept failed.\n", strlen("Server: accept failed.\n"));
            return 0;
        }

       // Output client information
        client_info(&client_addr);

        // Read and process client commands
        while((len_out = read(client_fd, buf, BUFFSIZE)) > 0){
            buf[len_out] = '\0';  // Null-terminate the received data

        write(1, buf, strlen(buf));  // Output received command

        // Process the received command and get the result
        cmd_process(buf, result_buff);

        // Send the result to the client
        write(client_fd, result_buff, strlen(result_buff));

        }
        // Close the client connection
        close(client_fd);
        return 0;
    }
    // Close the server socket at the end of execution
    close(socket_fd);
    return 0;

}