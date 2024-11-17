///////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                 //
// Date : 2024/04/18                                                 //
// OS : Ubuntu 20.04.6 LTS 64bits                                    //
//                                                                   //
// Author : Kang Hyun Min                                            //
// Student ID : 2020202092                                           //
// ----------------------------------------------------------------- //
// Title : System Programming Assignment #1-3 ( ftp server )        //
// Description : This file receives and check the user command,      //
// converts it into an appropriate FTP-command, and transmits       //
// the ftp-command, arguments, and options to standard output       //
// through the write function.                                      //
///////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////
// main                                                              //
// ================================================================= //
// Input: int argc, char* argv[]                                     //
// argc -> the number of arguments                                   //
// argv[] -> char data of arguments                                  //
// (Input parameter Description)                                     //
// -                                                                 //
// (Out parameter Description)                                       //
// Purpose: check the user command and converts it into              //
// an appropriate FTP-command, and write them to standard output     //
///////////////////////////////////////////////////////////////////////



int main(int argc, char* argv[]){

    char buf[128] = {0};  // Buffer to store command string

    // If the command is "pwd"
    if(strcmp(argv[1], "pwd") == 0){
        strcat(buf, "PWD");     // Convert user command as FTP command
        // Concatenate all arguments after the command
        for(int i = 2; i < argc; i++){
            strcat(buf, " ");
            strcat(buf, argv[i]);
        }
        // Write the command string to standard output
        write(1, buf, strlen(buf));
        
    }
    // If the command is "rename"
    else if (strcmp(argv[1], "rename") == 0){
        // Convert user command as FTP command
        if(argv[2] != NULL && argv[3] != NULL){
            // Construct the RNFR command
            strcpy(buf, "RNFR ");
            strcat(buf, argv[2]);
            strcat(buf, "\n");
            // Write the RNTO command to standard output
            write(1, buf, strlen(buf));

            // Construct the RNTO command
            strcpy(buf, "RNTO ");
            strcat(buf, argv[3]);
            strcat(buf, "\n");
            // Write the RNTO command to standard output
            write(1, buf, strlen(buf));
        }

        else {
            // If source and destination names are not provided, write a usage message
            strcat(buf, "RNFR RNTO");
            write(1, buf, strlen(buf));
        }
        
    }
    // If the command is "cd"
    else if(strcmp(argv[1], "cd") == 0){
        // Check if the command is "cd .." to convert it to CDUP command
        if ((argc > 2) ? strcmp(argv[2], "..") == 0 : 0|| (argc > 3) ? strcmp(argv[3], "..") == 0 : 0) {
            // Convert user command as FTP command
            strcat(buf, "CDUP");
            for(int i = 2; i < argc; i++){
             strcat(buf, " ");
             strcat(buf, argv[i]);
            }
        }
        // For other cases, convert it to CWD command
        else {
            // Convert user command as FTP command
            strcat(buf, "CWD");
            for(int i = 2; i < argc; i++){
             strcat(buf, " ");
             strcat(buf, argv[i]);
             }
        }

        // Write the converted command to standard output
        write(1, buf, strlen(buf));
    }
    
    // If the command is "mkdir"
    else if(strcmp(argv[1], "mkdir") == 0){
        strcat(buf, "MKD");     // Convert user command as FTP command
        int i = 2;

        // Concatenate all arguments after the command
        for(int i = 2; i < argc; i++){
            strcat(buf, " ");
            strcat(buf, argv[i]);
        }
    
    // Write the command string to standard output
     write(1, buf, strlen(buf));
    }

    // If the command is "rmdir"
    else if(strcmp(argv[1], "rmdir") == 0){
        strcat(buf, "RMD");     // Convert user command as FTP command
        int i = 2;

    // Concatenate all arguments after the command
     while (argv[i] != NULL) {
        strcat(buf, " ");
        strcat(buf, argv[i]);
        
        i++;
    }
    // Write the command string to standard output
     write(1, buf, strlen(buf));
        
    }
    // If the command is "delete"
    else if(strcmp(argv[1], "delete") == 0){
        strcat(buf, "DELE");        // Convert user command as FTP command
        int i = 2;
    // Concatenate all arguments after the command
     while (argv[i] != NULL) {
        strcat(buf, " "); 
        strcat(buf, argv[i]);
        
        i++;
    }
    // Write the command string to standard output
     write(1, buf, strlen(buf));
        
        
    }

    // If the command is "quit"
    else if(strcmp(argv[1], "quit") == 0){
        strcat(buf, "QUIT");    // Convert user command as FTP command
        int i = 2;
    // Concatenate all arguments after the command
     while (argv[i] != NULL) {
        strcat(buf, " ");
        strcat(buf, argv[i]);
        
        i++;
    }
    // Write the command string to standard output
     write(1, buf, strlen(buf));
        
        
    }

    // If the command is "ls"
    else if(strcmp(argv[1], "ls") == 0){
        strcat(buf, "NLST");    // Convert user command as FTP command
        int i = 2;

        // If additional arguments are provided, concatenate them to the buffer
        if(argv[2] == NULL){
            write(1, buf, strlen(buf)); // Write the command string to standard output
            
        }
        else {
        // Concatenate all arguments after the command
         while (argv[i] != NULL) {
            strcat(buf, " ");
            strcat(buf, argv[i]);
        
           i++;
          }
        // Write the command string to standard output
     write(1, buf, strlen(buf));
        }
        
        
    }
    // If the command is "dir"
    else if(strcmp(argv[1], "dir") == 0){
        strcat(buf, "LIST");    // Convert user command as FTP command
        int i = 2;

      // Concatenate all arguments after the command
     while (argv[i] != NULL) {
        strcat(buf, " "); 
        strcat(buf, argv[i]);
        
        i++;
    }
    // Write the command string to standard output
     write(1, buf, strlen(buf));
        
        
    }
    
}
