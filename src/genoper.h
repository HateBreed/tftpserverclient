/* Author: Jussi Laakkonen /  */

#ifndef __GENOPER_H
#define __GENOPER_H

#include "structures.h"

filedata *add_file_block(filedata *,char *, int);
filedata *put_file_block(filedata *,char *, int);
filedata *add_data_packet(filedata *,data_packet *, int);

unsigned long write_block_to_file(clientnode *);
int read_file_to_block(clientnode *);

clientnode *add_client(clientnode *, struct sockaddr_in , int , char *, int , filedata *);
clientnode *remove_stopped(clientnode *);

int check_file(char *);
int check_file_write(char *);

void free_filedata(filedata *);

#endif
