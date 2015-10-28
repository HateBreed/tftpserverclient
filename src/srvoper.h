/* Author: Jussi Laakkonen /  */

#ifndef __SRVOPER_H
#define __SRVOPER_H

#include <time.h>

#include "netoper.h"

char *parse_filename(char[]);

int create_port(int);

int download_mode(clientnode *);

void upload_mode(clientnode *);

int check_ack(clientnode *);

#endif
