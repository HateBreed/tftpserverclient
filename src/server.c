/* Author: Jussi Laakkonen / 

Sources used as help:

Beej's Guide to Network Programming
	http://beej.us/guide/bgnet/

Sockets tutorial
	http://www.cs.rpi.edu/courses/sysprog/sockets/sock.html

The World of select()
	http://www.lowtek.com/sockets/select.html

I also looked at the source code of the TFTP implementation in CURL library.
	http://curl.haxx.se/
	The code can be also found with codesearch.google.com with "curl tftp lang:c"

For using time as a seed for random number generator:
	http://www.cs.cf.ac.uk/Dave/C/node21.html

And of course linux man pages and the TFTP RFC 1350

*/

#include "server.h"

/* Address for file transfer */
struct sockaddr_in transfer_address;

/* Own address for listening connections */
struct sockaddr_in own_address;

unsigned int addr_length;

int main(int argc, char *argv[])
{
	int port;
	
	/* Check the given parameters and report error if any then quit */ 
	if(argc == PARAMS)
	{
		if((port = atoi(argv[1])) == 0) error_minor("Error: Invalid port.\nUsage:\t./server <port>\n");
	}
	
	/* If no parameter (port) is defined, default TFTP server port is used (69) */
	else if(argc == PARAMS - 1) port = TFTP_SERVER_PORT;
	
	else param_err();
	
	printf("Server is using port: %d\n",port);

	/* Run the server */
	if(runServer(port) != 0) error_minor("Error: starting error, unknown reason");

	return 0;
}

/*	Starts running the client in given mode, parameters:
		struct hostent *hostname	- host information
		int port			- port used by client
		char *filename			- file to be uploaded / downloaded
	
	Return 0 if successful.
*/
int runServer(int port)
{
	int returnval = 0, t_sock = 0, filecheck = 0, rv = 0, sockSrv = 0, stopping = 0;
	packet *pck = NULL;
	init_packet *i_pck = NULL;
	error_packet *e_pck = NULL;
	clientnode *first_cl = NULL, *temp_cl = NULL;
	filedata *files = NULL;
	char *filename = NULL;
	char buffer[TFTP_BLOCK];
	fd_set srv_fd_set;
	struct timeval to;
	
	/* Create socket */
	if((sockSrv = socket(AF_INET,SOCK_DGRAM,0)) < 0) error("Error creating socket");
	
	/* Set own address data */
	own_address.sin_family = AF_INET;
	own_address.sin_addr.s_addr = htons(INADDR_ANY);
	own_address.sin_port = htons(port);
	
	addr_length = sizeof(struct sockaddr_in);
	
	/* Bind socket to own address */
	if(bind(sockSrv,(struct sockaddr *)&own_address,addr_length) < 0)
	{
		error("Port probably in use, select another port.\nUsage:\t./server <port>\n\nError in socket binding");
	}
	
	while(1)
	{
		FD_ZERO(&srv_fd_set);

		to.tv_sec = 0;
		to.tv_usec = 1;
		
		FD_SET(0,&srv_fd_set);

		/* If server is set to stop and wait for current connections to finish, stop listening the server socket */		if(stopping == 0) FD_SET(sockSrv,&srv_fd_set);
		
		rv = select(sockSrv+1,&srv_fd_set,NULL,NULL,&to);
		
		if (rv < 0) error("Error in select()");
		
		else if(rv > 0)
		{
			/* Adding a client */
			if(FD_ISSET(sockSrv,&srv_fd_set))
			{
				/* Get packet */
				pck = read_packet(sockSrv);
				
				printf("New connection from: %s:%d. ",(char *)inet_ntoa(get_conn_addr().sin_addr), ntohs(get_conn_addr().sin_port));

				/* Create new socket */
				if((t_sock = socket(AF_INET,SOCK_DGRAM,0)) < 0) perror("Error creating socket");
				
				/* Set port and try to bind */
				transfer_address.sin_family = AF_INET;
				transfer_address.sin_addr.s_addr = htons(INADDR_ANY);
				transfer_address.sin_port = htons(create_port(1));
				
				while(bind(t_sock,(struct sockaddr *)&transfer_address,addr_length) < 0)
				{
					transfer_address.sin_port = htons(create_port(0));
				}
				
				printf("Bound to port: %d. ",ntohs(transfer_address.sin_port));

				/* Check packet size */
				if((pck->length < 10) || (pck->length > MAX_MSG))
				{
					pck = encode_error_packet(ERROR_ILLEGAL_OPERATION);
					printf("\nInvalid init packet size\n");
					send_packet(t_sock,pck);
					close(t_sock);
				}
				
				else
				{
					i_pck = decode_init_packet(pck);
		
					/* If client wants to read a file */
					if(ntohs(i_pck->opcode) == OPCODE_RRQ)
					{
						/* File name from packet */
						filename = parse_filename(i_pck->content);

						printf("Requesting file \"%s\".\n",filename);

						first_cl = add_client(first_cl,get_conn_addr(),t_sock,filename,OPCODE_RRQ,files);

						if(read_file_to_block(first_cl) == -1)
						{
							pck = encode_error_packet(ERROR_NOT_FOUND);
							send_packet(t_sock,pck);
							first_cl->state = STOP;
						}
					}
					
					/* If client wants to write a file */
					else if (ntohs(i_pck->opcode) == OPCODE_WRQ)
					{
						/* File name from packet */
						filename = parse_filename(i_pck->content);
						printf("Sending file \"%s\".\n",filename);

						/* Check that it can be written */
						if((filecheck = check_file(filename)) != 0)
						{	
							pck = encode_error_packet(filecheck);
							send_packet(t_sock,pck);
							close(t_sock);
						}
						else
						{
							first_cl = add_client(first_cl,get_conn_addr(),t_sock,filename,OPCODE_WRQ,NULL);
						}
					}
					
					/* Error */
					else if (ntohs(i_pck->opcode) == OPCODE_ERROR)
					{
						e_pck = decode_error_packet(pck);
						printf("Error from client: %s\n",e_pck->content);
						close(t_sock);
					}
					
					/* Else illegal operation */
					else
					{
						pck = encode_error_packet(ERROR_ILLEGAL_OPERATION);
						send_packet(t_sock,pck);
						printf("Invalid opcode\n");
						close(t_sock);
					}
				}
			}
			
			/* If something is typed into stdin */
			if(FD_ISSET(0,&srv_fd_set))
			{
				bzero(&buffer,TFTP_BLOCK);

				fgets(buffer,TFTP_BLOCK-1,stdin);

				/* Closing ? */
				if(strcasecmp(buffer,"CLOSE\n") == 0) stopping = 1;
			}
		}
		
		temp_cl = first_cl;
		
		/* Go through clients */
		while(temp_cl != NULL)
		{
			/* If not set as stopped */
			if(temp_cl->state != STOP)
			{
				/* Set address info */
				set_conn_addr(temp_cl->address);
				
				switch(temp_cl->mode)
				{
					case OPCODE_RRQ:
					
						upload_mode(temp_cl);
						break;
				
					case OPCODE_WRQ:
				
						/* If there was some error while downloading from client - try to remove the file */
						if(download_mode(temp_cl) != 1)
						{
							printf("Error happened. Trying to remove unfinished file ... ");
							if(close(temp_cl->filed) == 0)
							{
								temp_cl->filed = -1;
								if(!remove(temp_cl->filename)) printf("file \"%s\" was removed.\n",temp_cl->filename);
								else printf("file \"%s\" cannot be removed.\n",temp_cl->filename);
							}
						}
						break;
					
					default: break;
				}
			}
			
			temp_cl = temp_cl->next;
		}
		/* Remove stopped clients */
		first_cl = remove_stopped(first_cl);

		if((stopping == 1) && (first_cl == NULL))
		{
			printf("Server stopping\n");
			break;
		}
	}

	close(sockSrv);
	return returnval;
}



