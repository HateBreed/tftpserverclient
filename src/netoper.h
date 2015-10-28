/* Author: Jussi Laakkonen /  */

#ifndef __NETOPER_H
#define __NETOPER_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "genoper.h"

void set_conn_addr(struct sockaddr_in);

struct sockaddr_in get_conn_addr();

packet *read_packet(int);

int send_packet(int, packet *);

#endif
