/* Author: Jussi Laakkonen /  */

#include "netoper.h"

/* Address of the other party */
struct sockaddr_in conn_address;

/*	Sets the connecting address struct, parameter:
		struct sockaddr_in c_addr - The connection address information
*/
void set_conn_addr(struct sockaddr_in c_addr)
{
	conn_address = c_addr;
	return;
}

/*	Gets the address information

	Returns the sockadd_in structure of last connected client (read_packet
	saves information to conn_address).
*/
struct sockaddr_in get_conn_addr()
{
	return conn_address;
}


/*	Reads the data from socket, parameter:
		int sock	- socket to be used for sending

	Returns the received data as struct packet
*/
packet *read_packet(int sock)
{
	int n = 0;
	unsigned int address_length = sizeof(struct sockaddr_in);

	packet *pck = (packet *)malloc(sizeof(packet));
	pck->content = (char *)malloc(MAX_MSG);
	
	bzero(pck->content,MAX_MSG);

	/* Read packet (as void pointer) from socket */	
	if((n = recvfrom(sock,(void*)pck->content,MAX_MSG,0,(struct sockaddr *)&conn_address,&address_length)) < 0) perror("Receiving");

	//printf("received from: %s:%d\n",(char *)inet_ntoa(conn_address.sin_addr), ntohs(conn_address.sin_port));

	/* Set length */
	pck->length = n;

	return pck;
}

/*	Sends given packet to socket, parameters:
		int sock	- socket to be used for sending
		packet *pck	- packet to be sent

	Returns amount of bytes written
*/
int send_packet(int sock, packet *pck)
{
	int n = 0;
	unsigned int address_length = sizeof(struct sockaddr_in);
	
	//printf("sending to: %s:%d\n",(char *)inet_ntoa(conn_address.sin_addr), ntohs(conn_address.sin_port));
	/* Send the content as void pointer */
	if((n = sendto(sock,(void *)pck->content,pck->length,0,(struct sockaddr *)&conn_address,address_length)) < 0) perror("Sending");
	
	return n;
}
