/////////////////////////////////////////////////////////////////////////
// File Name : kw2020202092_ls.c 					 //
// Date : 2024/04/10 							 //
// OS : Ubuntu 20.04.6 LTS 64bits					 //
//									 //
// Author : Kang Hyun Min		 				 //
// Student ID : 2020202092					         //
// -----------------------------------------------------------------   //
// Title : System Programming Assignment #1-2 ( ftp server )		  //
// Description : This file is a C code that implements the ls command, //
// one of the ftp commands. If no argument is used, it outputs	  //
// the list of files or directories in the current directory, and	  //
// if an argument is used, it outputs the list of files or directories //
// in the direcory of this argument.					  //
/////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>


///////////////////////////////////////////////////////////////////////////////
// main 									//
// =================================================================  	//
// Input: int argc -> variable for the number of inputted arguments   	//
// 	  char* argv[] -> pointer variable to separate arguments		//
//			  based on space and store them in an array 		//
// (Input parameter Description) 						//
// -										//
// (Out parameter Description)						//
// Purpose: Based on the number of input arguments, that is,			// 
// the int argc value, if it is greater than 2, an error is output.		//
// If it is 2, an error is checked and a list of files in the directory	//
// with argv[1] as the name is output. Also, if argc == 1,			//
// the file list in the current directory is outputs				//
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {

	//Declare variables
	DIR *dp;				//directory stream
    	struct dirent *dirp;			//directory entry pointer
    	char *dirname;				//directory name
    	char *exename = basename(argv[0]); 	//execution file name

	//Check the number of command-line arguments
    	if (argc > 2) {
        	printf("only one directory path can be processed\n");
        	exit(1);
    	}
	
	//IF one argument is inputted, assume it's the directory path
    	else if (argc == 2) { 
        	dirname = argv[1];	//store directory path
        	
        	// Attempt to open the directory
        	if ((dp = opendir(dirname)) == NULL) {
        		printf("%s : cannot access '%s' : ", exename, dirname);
     
        		//Check if the directory exists
        		if(access(dirname, F_OK) == -1){
        	    		printf("No such directory\n");
        	    		exit(1);
        	   	 }	
        	   	//Check if access to read the directory is denied
           	 	else if(access(dirname, R_OK) == -1){
           	 		printf("Access denied\n");
           	 		exit(1);
      		  	}
	 	  }
   	 } 
    	
    	//if no arguments provided, use current directory
    	else {
        	dirname = ".";
        	// Attempt to open the current directroy
        	if ((dp = opendir(dirname)) == NULL) {
        	    exit(1);
        	}
    	}
	
	// Read and print the list of files in directory
    	while ((dirp = readdir(dp)) != NULL) {
        	printf("%s\n", dirp->d_name);
    	}
	
	//close the directory stream
	closedir(dp);
    	return 0;
}

