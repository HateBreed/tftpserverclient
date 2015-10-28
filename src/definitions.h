/* Author: Jussi Laakkonen /  */

#ifndef __DEFINITIONS_H
#define __DEFINITIONS_H

#define OPCODE_RRQ 1
#define OPCODE_WRQ 2
#define OPCODE_DATA 3
#define OPCODE_ACK 4
#define OPCODE_ERROR 5

#define ERROR_NOT_DEFINED 0
#define ERROR_NOT_FOUND 1
#define ERROR_ACCESS_VIOLATION 2
#define ERROR_DISK_FULL 3
#define ERROR_ILLEGAL_OPERATION 4
#define ERROR_UNKNOWN_ID 5
#define ERROR_FILE_EXISTS 6
#define ERROR_NO_SUCH_USER 7

#define TFTP_BLOCK 512
#define MAX_MSG 516

#define TIMEOUT_SEC 0

#define TIMEOUT_USEC 10000
#define TIMEOUT_ADD 10000
#define TIMEOUT_MAX 100000

#define TFTP_SERVER_PORT 69
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>


#endif
