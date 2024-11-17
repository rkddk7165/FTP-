///////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                 //
// Date : 2024/04/18                                                 //
// OS : Ubuntu 20.04.6 LTS 64bits                                    //
//                                                                   //
// Author : Kang Hyun Min                                            //
// Student ID : 2020202092                                           //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #1-3 ( ftp server )         //
// Description : The purpose of this file is to read FTP-COMMAND     //
// data from standard input through the read-function,               //
// perform operations according to this command,                     //
// and output the results.                                           //
///////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
 

char buf[128] = {0}; // Buffer to store input commands

// Function to print the current working directory
void PWD(){
    char *cwd; // Pointer to store the current working directory path

    // Get the current working directory
    cwd = getcwd(NULL, 0);

    // Allocate memory to store the output string
    char *output = (char *)malloc(strlen(cwd) + 20);
    if (output == NULL) {
        free(cwd);
        return;
    }

    // Copy the current working directory path to the output string
    strcpy(output, "\"");
    strcat(output, cwd);
    strcat(output, "\" is current directory\n");

    // Print the output string
    write(1, output, strlen(output));

    // Free allocated memory
    free(output);
    free(cwd);
}


// Function to rename a file or directory
void RENAME(char **arg) {
    // Check if directory exists and has write permission
    if (access(arg[1], F_OK) == 0 && access(arg[1], W_OK) == -1) {
        write(1, "Error: cannot access\n", strlen("Error: cannot access\n"));
        return;
    }

    // Check if the file to be renamed exists
    if (access(arg[1], F_OK) == -1) {
        write(1, "Error: File '", strlen("Error: File '"));
        write(1, arg[1], strlen(arg[1]));
        write(1, "' does not exist.\n", strlen("' does not exist.\n"));
        return;
    }

    // Check if the new name already exists
    if (access(arg[3], F_OK) == 0) {
        write(1, "Error: name to change already exists\n", strlen("Error: name to change already exists\n"));
        return;
    }

    // Rename the file or directory
    if (rename(arg[1], arg[3]) == -1) {
        write(1, "cannot rename\n", strlen("cannot rename\n"));
    } else {
        // Print the old and new names
        write(1, "RNFR ", strlen("RNFR "));
        write(1, arg[1], strlen(arg[1]));
        write(1, "\nRNTO ", strlen("\nRNTO "));
        write(1, arg[3], strlen(arg[3]));
        write(1, "\n", 1);
    }
}

// Function to change the current working directory
void CD(char **path) {

    // Check if directory exists and has read permission
    if (access(path[1], F_OK) == 0 && access(path[1], R_OK) == -1) {
        write(1, "Error: cannot access\n", strlen("Error: cannot access\n"));
        return;
    }

    // Change directory
    if (chdir(path[1]) == -1) {
        write(1, "Error: directory not found\n", strlen("Error: directory not found\n"));
    } else {

        char cwd[1024]; // Buffer to store the new current working directory path

        // Get the new current working directory path
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            if(strcmp(path[0], "CWD") == 0){
                // Print the command and the new current working directory path
                write(1, "CWD ", strlen("CWD "));
                write(1, path[1], strlen(path[1]));
                write(1, "\n", strlen("\n"));
            }

            else if(strcmp(path[0], "CDUP") == 0){
                // Print the command
                write(1, "CDUP\n", strlen("CDUP\n"));
            }
            // Print the new current working directory path
            write(1, "\"", 1);
            write(1, cwd, strlen(cwd));
            write(1, "\" is current directory\n", strlen("\" is current directory\n"));
        }
    }
}

// Function to create directories
void MKD(char **dirnames) {
    int i = 1;

    // Loop through each directory name provided
    while (dirnames[i] != NULL) {
        char *dirname = dirnames[i];
        if (strcmp(dirname, "") == 0) {
            // Print error if directory name is empty
            write(1, "Error: argument is required\n", strlen("Error: argument is required\n"));
        } else {

            // Attempt to create the directory
            if (mkdir(dirname, 0775) == -1) {
                if (errno == EEXIST) {
                    // Print error if directory already exists
                    char error_msg[100];
                    sprintf(error_msg, "Error: cannot create directory '%s': File exists\n", dirname);
                    write(1, error_msg, strlen(error_msg));
                }
            } 
            else {
                // Print success message if directory is created successfully
                char success_msg[50];
                sprintf(success_msg, "MKD %s\n", dirname);
                write(1, success_msg, strlen(success_msg));
            }
        }
        i++;
    }
}

// Function to remove directories
void RMD(char **dirnames) {
    int i = 1;

    // Loop through each directory name provided
    while (dirnames[i] != NULL) {
        char *dirname = dirnames[i];
        if (strcmp(dirname, "") == 0) {
            // Print error if directory name is empty
            write(1, "Error: argument is required\n", strlen("Error: argument is required\n"));
        } else {

            // Check if directory exists and has write permission
            if (access(dirnames[i], F_OK) == 0 && access(dirnames[i], W_OK) == -1) {
                // Print error if directory cannot be accessed
                write(1, "Error: cannot access\n", strlen("Error: cannot access\n"));
            }

            // Attempt to remove the directory
            if (rmdir(dirname) == -1) {
                if (errno == ENOENT) {
                    // Print error if directory does not exist
                    char error_msg[50];
                    sprintf(error_msg, "Error: failed to remove '%s'\n", dirname);
                    write(1, error_msg, strlen(error_msg));
                } 
            } 
            else {
                // Print success message if directory is removed successfully
                char success_msg[50];
                sprintf(success_msg, "RMD %s\n", dirname);
                write(1, success_msg, strlen(success_msg));
            }
        }
        i++;
    }
}

// Function to delete files
void DELE(char **filenames) {
    int i = 1;

    // Loop through each filename provided
    while (filenames[i] != NULL) {
        char *filename = filenames[i];
        if (strcmp(filename, "") == 0) {
            // Print error if filename is empty
            write(1, "Error: argument is required\n", strlen("Error: argument is required\n"));
        } else {

            // Check if directory exists and has write permission
            if (access(filenames[i], F_OK) == 0 && access(filenames[i], W_OK) == -1) {
                write(1, "Error: cannot access\n", strlen("Error: cannot access\n"));
            }

            // Attempt to delete the file
            if (unlink(filename) == -1) {
                if (errno == ENOENT) {
                    // Print error if file does not exist
                    char error_msg[50];
                    sprintf(error_msg, "Error: failed to delete '%s'\n", filename);
                    write(1, error_msg, strlen(error_msg));
                }
            } 
            else {
                // Print success message if file is deleted successfully
                char success_msg[50];
                sprintf(success_msg, "DELE %s\n", filename);
                write(1, success_msg, strlen(success_msg));
            }
        }
        i++;
    }
}

// Function to exit the program
void QUIT() {
    write(1, "QUIT success\n", strlen("QUIT success\n"));
    exit(0);
}

// Function to perform bubblesort on array of dirent pointers
// This function is used in NLST function to sort names of directories or files
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

// Function to list directory contents
void NLST(char **arg, int aflag, int lflag) {

    char *directory = "";   // Directory path

    // Determine the directory path based on input arguments when inputted command is "NLST"
    if(strcmp(arg[0], "NLST") == 0){
        if (arg[1] == NULL && arg[2] == NULL) {
            // No directory specified, use current directory
            directory = getcwd(NULL , 0);
        } 
        else if(arg[1] != NULL && arg[1][0] == '-' && arg[2] != NULL){
            // Directory specified after option flag
            directory = arg[2];
        }
        else if(arg[1] != NULL && arg[1][0] == '-' && arg[2] == NULL){
            directory = getcwd(NULL, 0);
        
        }
        else if(arg[1] != NULL && arg[1][0] != '-'){
            // Directory specified directly
            directory = arg[1];
        }
    }

    // Determine the directory path based on input arguments when inputted command is "LIST"
    else if(strcmp(arg[0], "LIST") == 0){
        if (arg[1] == NULL) {
            // No directory specified, use current directory
            directory = getcwd(NULL, 0);
        } 
        else {
            // Directory specified directly
            directory = arg[1];
        }
    }

    struct stat file_stat;  // File status structure

    // Check if the input is a file or directory
    if (stat(directory, &file_stat) == -1) {
        write(1, "Error: No susch file or directory\n", strlen("Error: No susch file or directory\n"));
        return;
    }

    if (S_ISDIR(file_stat.st_mode)) {
        // Input is a directory
        DIR *dir;
        struct dirent **namelist; //using to store name of files or directory
        struct dirent *entry;     //using for output name of files or directory when lflag = 1
        struct passwd *pwd;       //using for output user datas when lflag = 1
        struct group *grp;        //using for output groupID when lflag = 1
        struct tm *tm;            //using for output modified time when lflag = 1
        char path[1024];

        // Check if directory is accessible
        if(access(directory, R_OK) == -1){
            write(1, "Error : cannot access\n", strlen("Error : cannot access\n"));
            exit(1);
        }   

        // Open the directory
        dir = opendir(directory);
        if (dir == NULL) {
            return;
        }

        // Read all entries in the directory
        int n = 0;
        while ((entry = readdir(dir)) != NULL) {
            // Ignore hidden files if -a flag is not set
            if (!aflag && entry->d_name[0] == '.') {
                continue;
            }
            n++;
        }

        // Allocate memory for namelist array
        namelist = (struct dirent **)malloc(n * sizeof(struct dirent *));
        if (namelist == NULL) {

            closedir(dir);
            return;
        }

        // Reset directory stream position
        rewinddir(dir);

        // Populate namelist array with entries
        int i = 0;
        while ((entry = readdir(dir)) != NULL) {
            // Ignore hidden files if -a flag is not set
            if (!aflag && entry->d_name[0] == '.') {
                continue;
            }
            namelist[i++] = entry;
        }

        // Sort the entries using bubblesort
        bubblesort(namelist, n);

        // Print the sorted entries
        int file_count = 0;
        for (int i = 0; i < n; i++) {
            char *name = namelist[i]->d_name;

            // Get the full path of the entry
            sprintf(path, "%s/%s", directory, name);

            // Get file status
            if (stat(path, &file_stat) == -1) {
                write(1, "Error: No such file or directory\n", strlen("Error: No such file or directory\n"));
                continue;
            }

            // Print file details if -l flag is set
            if (lflag) {
                // File type and permissions
                write(1, (S_ISDIR(file_stat.st_mode)) ? "d" : "-", 1);
                write(1, (file_stat.st_mode & S_IRUSR) ? "r" : "-", 1);
                write(1, (file_stat.st_mode & S_IWUSR) ? "w" : "-", 1);
                write(1, (file_stat.st_mode & S_IXUSR) ? "x" : "-", 1);
                write(1, (file_stat.st_mode & S_IRGRP) ? "r" : "-", 1);
                write(1, (file_stat.st_mode & S_IWGRP) ? "w" : "-", 1);
                write(1, (file_stat.st_mode & S_IXGRP) ? "x" : "-", 1);
                write(1, (file_stat.st_mode & S_IROTH) ? "r" : "-", 1);
                write(1, (file_stat.st_mode & S_IWOTH) ? "w" : "-", 1);
                write(1, (file_stat.st_mode & S_IXOTH) ? "x" : "-", 1);

                // Number of hard links
                char link_count[20];
                sprintf(link_count, "\t%lu", file_stat.st_nlink);
                write(1, link_count, strlen(link_count));

                // Owner name
                if ((pwd = getpwuid(file_stat.st_uid)) != NULL) {
                    write(1, "\t", 1);
                    write(1, pwd->pw_name, strlen(pwd->pw_name));
                } else {
                    char uid_str[20];
                    sprintf(uid_str, "\t%d", file_stat.st_uid);
                    write(1, uid_str, strlen(uid_str));
                }

                // Group name
                if ((grp = getgrgid(file_stat.st_gid)) != NULL) {
                    write(1, "\t", 1);
                    write(1, grp->gr_name, strlen(grp->gr_name));
                } else {
                    char gid_str[20];
                    sprintf(gid_str, "\t%d", file_stat.st_gid);
                    write(1, gid_str, strlen(gid_str));
                }

                // File size
                char size_str[20];
                sprintf(size_str, "\t%lld", (long long)file_stat.st_size);
                write(1, size_str, strlen(size_str));

                // Get file modification time
                static const char *month_names[] = {
                    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                };
                tm = localtime(&file_stat.st_mtime);      
                char time_str[20];
                sprintf(time_str, "\t%s %02d %02d:%02d\t", month_names[tm->tm_mon], tm->tm_mday, tm->tm_hour, tm->tm_min);
                write(1, time_str, strlen(time_str));

                // Print file name with '/' if it's a directory
                if (S_ISDIR(file_stat.st_mode)) {
                    write(1, name, strlen(name));
                    write(1, "/\n", 2);
                } else {
                    write(1, name, strlen(name));
                    write(1, "\n", 1);
                }
            } else {
                // Print file name only when lflag == 0
                if (S_ISDIR(file_stat.st_mode)) {
                    write(1, name, strlen(name));
                    write(1, "/\t", 2);
                } else {
                    write(1, name, strlen(name));
                    write(1, "\t", 1);
                }
            }

            file_count++;

            if(!lflag && file_count % 5 == 0)
                write(1, "\n", 1);
        }
            write(1, "\n", 1);

        

        // Free dynamically allocated memory
        free(namelist);
        closedir(dir);
    } else {
        // Input is a file
        // Print file details
        if (lflag) {
            // File type and permissions
            write(1, (S_ISDIR(file_stat.st_mode)) ? "d" : "-", 1);
            write(1, (file_stat.st_mode & S_IRUSR) ? "r" : "-", 1);
            write(1, (file_stat.st_mode & S_IWUSR) ? "w" : "-", 1);
            write(1, (file_stat.st_mode & S_IXUSR) ? "x" : "-", 1);
            write(1, (file_stat.st_mode & S_IRGRP) ? "r" : "-", 1);
            write(1, (file_stat.st_mode & S_IWGRP) ? "w" : "-", 1);
            write(1, (file_stat.st_mode & S_IXGRP) ? "x" : "-", 1);
            write(1, (file_stat.st_mode & S_IROTH) ? "r" : "-", 1);
            write(1, (file_stat.st_mode & S_IWOTH) ? "w" : "-", 1);
            write(1, (file_stat.st_mode & S_IXOTH) ? "x" : "-", 1);

            // Number of hard links
            char link_count[20];
            sprintf(link_count, "\t%lu", file_stat.st_nlink);
            write(1, link_count, strlen(link_count));

            // Owner name
            struct passwd *pwd;
            if ((pwd = getpwuid(file_stat.st_uid)) != NULL) {
                write(1, "\t", 1);
                write(1, pwd->pw_name, strlen(pwd->pw_name));
            } else {
                char uid_str[20];
                sprintf(uid_str, "\t%d", file_stat.st_uid);
                write(1, uid_str, strlen(uid_str));
            }

            // Group name
            struct group *grp;
            if ((grp = getgrgid(file_stat.st_gid)) != NULL) {
                write(1, "\t", 1);
                write(1, grp->gr_name, strlen(grp->gr_name));
            } else {
                char gid_str[20];
                sprintf(gid_str, "\t%d", file_stat.st_gid);
                write(1, gid_str, strlen(gid_str));
            }

            // File size
            char size_str[20];
            sprintf(size_str, "\t%lld", (long long)file_stat.st_size);
            write(1, size_str, strlen(size_str));

            // Get file modification time
            struct tm *tm;
            static const char *month_names[] = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
            };
            tm = localtime(&file_stat.st_mtime);
            char time_str[20];
            sprintf(time_str, "\t%s %02d %02d:%02d\t", month_names[tm->tm_mon], tm->tm_mday, tm->tm_hour, tm->tm_min);
            write(1, time_str, strlen(time_str));

            // Print file name
            write(1, directory, strlen(directory));
            write(1, "\n", 1);
        } else {
            // Print file name only when lflag == 0
            write(1, directory, strlen(directory));
            write(1, "\n", 1);
        }
    }
}





///////////////////////////////////////////////////////////////////////
// main                                                              //
// ================================================================= //
//                                                                   //
// -                                                                 //
// (Input parameter Description)                                     //
// -                                                                 //
// (Out parameter Description)                                       //
// Purpose: The purpose is to check FTP commands, arguments,         //
// and options, and perform appropriate exception handling and       //
// function calls according to the commands.                         //
///////////////////////////////////////////////////////////////////////
int main(){
	
	
	char buf[128];  // Array to store command arguments
    char **arg;     // a variable to tokenize datas of cli.out

    // Read input from standard input
    read(0, buf, sizeof(buf));

    int token_count = 0;
    char *token;

    // Count the number of tokens in the input
    for (int i = 0; i < sizeof(buf); i++) {
        if (buf[i] == ' ' || buf[i] == '\n') {
            token_count++;
        }
    }

    // Handle the case where there's no space or newline after the last tokenfile_count++;


    if (buf[sizeof(buf)-1] != ' ' && buf[sizeof(buf)-1] != '\n') {
        token_count++;
    }

    // Allocate memory for the arg array based on the token count
    arg = (char **)malloc((1+token_count) * sizeof(char *));
    if (arg == NULL) {
        perror("Memory allocation failed");
    }

    // Tokenize the input string based on spaces and newlines
    token = strtok(buf, " \n");
    int i = 0;
    while (token != NULL) {
        arg[i++] = token;
        token = strtok(NULL, " \n");
    }
    arg[i] = 0;

    // Check command == PWD
	if(strcmp(arg[0], "PWD") == 0){
        // Check if the argument is inputted
        if(token_count != 1){
            if(arg[1][0] == '-'){

            //Check if the option is inputted
            printf("Error: invalid option\n");
            }
            else printf("Error: argument is not required\n");
        }

        else {
            //Call PWD function
            PWD();
        }

		
		
	}

    //Check if arguments of RENAME command are inputted
    else if(strcmp(arg[0], "RNFR") == 0 && strcmp(arg[1], "RNTO") == 0){
        //arguments are not inputted -> error
        printf("two arguments are required\n");
        return 1;
    }

    //Check if arguments of RENAME command are inputted
	else if(strcmp(arg[0], "RNFR") == 0 && strcmp(arg[2], "RNTO") == 0){
        if(arg[1][0] == '-'){
            //CHeck if the option is inputted
            write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            exit(0);
        }
        else
        //two arguments are inputted -> Call RENAME funcion
		RENAME(arg);
	}

    
    //Check if command is CWD
	else if (strcmp(arg[0], "CWD") == 0) {
        //Check if argument of CWD command is inputted
        if(arg[1] != NULL){
        //Check if the option is inputted
        if(arg[1][0] == '-'){
            printf("Error: invalid option\n");
            exit(0);
            }
            else {
                CD(arg);    //Call CD function
        }
        }
        else {
            printf("argument is required\n");
            return 1;
        }
            
    }
    //Check if command is CDUP
	else if (strcmp(arg[0], "CDUP") == 0) {
        //Check if argument of CDUP command is inputted
        if(arg[1] != NULL){
            //Check if the option is inputted
             if(arg[1][0] == '-'){
             printf("Error: invalid option\n");
             exit(0);
            }
        else {
            //Call CD function
            CD(arg);
        }
        }
        else {
            printf("argument is required\n");
            return 1;
        
    }
    }

    //Check if command is MKD
	else if(strcmp(arg[0], "MKD") == 0) {
        //Check if argument of MKD command is inputted
        if(arg[1] != NULL){

        //Check if the option is inputted
        if(arg[1][0] == '-'){
            printf("Error: invalid option\n");
            exit(0);
            }

    	else MKD(arg);  //Call MKD function
        }
        else {printf("Error: argument is required\n");
            exit(0);
        }
	}
    //Check if command is RMD
    else if(strcmp(arg[0], "RMD") == 0){
        //Check if argument of RMD command is inputted
        if(arg[1] != NULL){
        //Check if the option is inputted
        if(arg[1][0] == '-'){
            printf("Error: invalid option\n");
            exit(0);
            }

    	else RMD(arg);  //Call RMD function
        }
        else {printf("Error: argument is required\n");
            exit(0);
        }
    }

    //Check if command is DELE
    else if(strcmp(arg[0], "DELE") == 0){
        //Check if argument of DELE command is inputted
        if(arg[1] != NULL){
        //Check if the option is inputted
        if(arg[1][0] == '-'){
            printf("Error: invalid option\n");
            exit(0);
            }

    	else DELE(arg); //Call DELE function
        }
        else {printf("Error: argument is required\n");
            exit(0);
        }
    }

    //Check if command is QUIT
    else if(strcmp(arg[0], "QUIT") == 0){
        //Check if argument of QUIT command is inputted
        if(arg[1] != NULL){
            //Check if the option is inputted
            if(arg[1][0] == '-'){
            printf("Error: invalid option\n");
            exit(0);
            }
            else {
            printf("argument is not required\n");
            }
        }
        

    	else QUIT();    //Call QUIT function
        
        
    }

    //Check if command is NLST
    else if(strcmp(arg[0], "NLST") == 0){

        int aflag, lflag = 0;  // Flags for options -a and -l

        // Check if no options are provided
        if(arg[1] == NULL && arg[2] == NULL){
            // Print the command and call NLST function without any options
            write(1, "NLST\n", strlen("NLST\n"));
            NLST(arg, aflag, lflag);
        }
        // If the first argument starts with '-' (indicating an option)
        else if(arg[1][0] == '-'){
        // Check for various option combinations
        if(strcmp(arg[1], "-a") == 0){
            aflag++;    // Set -a flag
            printf("NLST -a\n");
            NLST(arg, aflag, lflag);    // Call NLST function with -a flag
        }
        else if(strcmp(arg[1],"-l") == 0){
            lflag++;    // Set -l flag
            printf("NLST -l\n");
            NLST(arg, aflag, lflag);    // Call NLST function with -l flag
        }
        else if(strcmp(arg[1], "-al") == 0 || strcmp(arg[1], "-la") == 0){
            aflag++;
            lflag++;
            printf("NLST -al\n");
            NLST(arg, aflag, lflag);    // Call NLST function with -a and -lflag
        }

        // If an invalid option is provided
        else write(1, "Error: invalid option\n",strlen("Error: invalid option\n") );
        }
        // If options are not provided and the first argument is not an option
        else if (arg[1] != NULL && arg[1][0] != '-'){
            write(1, "NLST\n", strlen("NLST\n") );
            NLST(arg, aflag, lflag);
            
        }
        // Exit the program
        exit(0);
    }
        
    
    // If the command is "LIST"
    else if(strcmp(arg[0], "LIST") == 0){

    int aflag = 0, lflag = 0;  // Flags for options -a and -l

    // Check for options
    if (arg[1] != NULL) {
        if(arg[1][0] == '-'){
        // Option provided
        printf("Error: invalid option\n");
        return 0;
    }
        else {
            NLST(arg, 1, 1);    //Call NLST with options of -a and -l
        }
    }
    else {
        NLST(arg, 1, 1);    //Call NLST with options of -a and -l
    }
    }
   
    // Free dynamically allocated memory
    free(arg);

	return 0;
    }





    



