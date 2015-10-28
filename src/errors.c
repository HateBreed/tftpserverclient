/* 	Author:	Jussi Laakkonen /  */

#include "errors.h"

/* Prints the given error message to stderr and exits the program */
void error(char *msg)
{
	perror(msg);
	exit(1);
}

/* Some other error with no (probable) error code */
void error_minor(char *msg)
{
	printf("%s\n",msg);
	exit(1);
}

/* Prints invalid parameter message and exits */
void param_err()
{
	printf("Invalid parameters!\nUsage:\n");
	printf("\tUpload: ./client <hostname> <port> upload <filename>\n");
	printf("\tDownload: ./client <hostname> <port> download <filename>\n");
	fflush(stdout);
	exit(1);
}
