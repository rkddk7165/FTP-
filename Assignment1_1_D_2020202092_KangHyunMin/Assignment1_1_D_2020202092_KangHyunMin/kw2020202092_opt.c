/////////////////////////////////////////////////////////////////////////////////////////////
// File Name : kw2020202092_opt.c 								              //
// Date : 2024/04/04 									      //
// OS : Ubuntu 20.04.6 LTS 64bits							      //
//											      //
// Author : Kang Hyun Min 								      //
// Student ID : 2020202092 								      //
// ----------------------------------------------------------------- 		      //
// Title : System Programming Assignment #1-1 ( ftp server ) 			      //
// Description : _opt.c file demonstrates the usage of command-line options parsing using  //
// 	         getopt function. The program takes command-line arguments and parses them //
//               for options specified with the letters a, b and c where c expects an      //
//               argument. 								      //
/////////////////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>

/////////////////////////////////////////////////////////////////////////
// main 								  //
// =================================================================   //
// Input: int argc -> count the number of command-line arguments	  //
// 	  char** argv -> array of command-line arguments		  //
// Output: int -1 program exit 					  //
//	    int 0 loop							  //
// Purpose: Entry point of the program, demonstrates command-line	  //
// options parsing and handling non-option arguments			  //
/////////////////////////////////////////////////////////////////////////
int main (int argc, char ** argv)
{
	//declare and initialize variables
	int aflag = 0, bflag = 0;
	char *cvalue = NULL;
	int index, c;
	opterr = 0;
	//////////////////////// Option Parsing //////////////////////
	// Loop through command-line options using getopt function
	while ((c = getopt (argc, argv, "abdc:")) != -1)
	{
		switch (c)
		{
			case 'a':
				//increment aflag when option '-a' is encountered.
				aflag++;
				break;
			case 'b':
				//increment bflag when option '-b' is encountered.
				bflag++;
				break;
			case 'c':
				//set cvalue to the argument following op
				cvalue = optarg;
				break;
			case 'd':
				//set opterr as 0 when option is '-d'
				opterr = 0;
				break;
			case '?':
				//handle unknown option characters
				printf("Unknown option character\n");
				break;
		}
	}
	//output the values of aflag, bflag, and cvalue after option parsing
	printf("aflag = %d\t, bflag = %d\t, cvalue = %s\n", aflag, bflag, cvalue);
	
	//outputs all non-option arguments, which are undefined arguments 
	for (index = optind; index < argc; index++)
		printf("Non-option argument %s\n", argv[index]);
	
	//Exit program
	return 0;
}

