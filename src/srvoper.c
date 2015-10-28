/* Author: Jussi Laakkonen /  */

#include "srvoper.h"

/* For select */

/* 	Parse the filename from the init packet, parameter:
		char init_content[TFTP_BLOCK] - The content of init packet
	
	Returns the parsed filename
*/
char *parse_filename(char init_content[TFTP_BLOCK])
{
	int i = 0, j = 0;
	char *filename = NULL;
	
	/* Get the length of the filename */
	for(i = 0; i < strlen(init_content) && init_content[i] != '\0'; i++);
	
	/* Reserve memory and clear it */
	filename = (char *)malloc(i);
	bzero(filename,i);
	
	/* Copy filename */
	for(j = 0; j <= i; j++) filename[j] = init_content[j];
	
	return filename;
}

/* 	Creates a random port between [1000,65000] for connections, parameter:
		int create	- For creating a seed for random function,
						1 - create new seed (time)
						0 - take next random with previous seed

	Returns the port number
*/
int create_port(int create)
{
	unsigned int port = 0;
	time_t t;

	/* Get the time in milliseconds and use it as a seed for random number generator */
	(void) time(&t);
	if(create == 1) srand((long)t);
	
	/* Create port */
	port = (unsigned int)rand();
	
	/* Create port until it is within the limits */
	while((port > 65000) || (port < 1000))
	{
		port = (unsigned int)rand();
	}
	//printf("random port: %d (%d rnd)\n",port);
	
	return port;
}


/*	Receive block from client, parameter:
		clientnode *cl	- the data structure of client
*/
int download_mode(clientnode *cl)
{
	int rv = 0, packet_len = 0, returnval = 1;
	packet *pck = NULL;
	data_packet *d_pck = NULL;
	error_packet *e_pck = NULL;
	
	struct timeval to;

	fd_set s_set;
	
	/* If waiting for data or in started state, (re)send ack */
	if((cl->state == START) || (cl->state == WAIT))
	{
		pck = encode_ack_packet(cl->lastpacket);
		send_packet(cl->socket,pck);
		cl->state = RUN;
	}
	
	FD_ZERO(&s_set);
	
	FD_SET(cl->socket,&s_set);
	
	to.tv_sec = 0;
	to.tv_usec = cl->timeout;
	
	rv = select(cl->socket+1,&s_set,NULL,NULL,&to);
	
	if(rv < 0)
	{
		perror("Error in download select()");
		cl->state = STOP;
	}
	
	else if (rv > 0)
	{
		/* Got something */
		if(FD_ISSET(cl->socket,&s_set))
		{
			/* Read packet and check size */
			pck = read_packet(cl->socket);
			packet_len = pck->length;
		
			/* Reset timeout */
			cl->timeout = TIMEOUT_USEC;
			
			if((packet_len < 4)||(packet_len > MAX_MSG))
			{
				pck = encode_error_packet(ERROR_ILLEGAL_OPERATION);
				send_packet(cl->socket,pck);
				cl->state = STOP;
			}
			
			/* Otherwise can expect a data packet */
			else
			{
				d_pck = decode_data_packet(pck);

				/* If a data packet */
				if(ntohs(d_pck->opcode) == OPCODE_DATA)
				{
					/* If packet number is same as expected */
					if(ntohs(d_pck->blockno) == cl->lastpacket + 1)
					{
						/* If length greater than 4 (at least one byte of data) - add */
						if(packet_len > 4)
						{
							cl->data = add_data_packet(cl->data,d_pck,packet_len);
							if(write_block_to_file(cl) == -1)
							{
								pck = encode_error_packet(ERROR_ACCESS_VIOLATION);
								send_packet(cl->socket,pck);
								cl->state = STOP;
								returnval = -1;
							}
						}
						
						cl->lastpacket += 1;
						
						/* Send ack */
						pck = encode_ack_packet(cl->lastpacket);
						
						send_packet(cl->socket,pck);
						//printf("ACK: %d for packet: %d (%d bytes)\n",cl->lastpacket,ntohs(d_pck->blockno), packet_len);
						
						/* If smaller than 512 (+4) it is the last packet -> stop */
						if(packet_len != MAX_MSG) cl->state = STOP;
						else cl->state = RUN;
					}
				}
				
				/* If it is an error packet, decode it, print message and stop */
				else if(ntohs(d_pck->opcode) == OPCODE_ERROR)
				{
					e_pck = decode_error_packet(pck);
					
					printf("Error: %s\n",e_pck->content);
					
					cl->state = STOP;
					returnval = -1;
				}

				/* Otherwise a errorous packet */
				else
				{
					printf("Erroreus packet");
					
					pck = encode_error_packet(ERROR_ILLEGAL_OPERATION);
					send_packet(cl->socket,pck);
					
					cl->state = STOP;
					returnval = -1;
				}
			}
		}
	}
	
	/* Timeout */
	else
	{
		cl->state = WAIT;
		/* Increase the timeout value */
		cl->timeout += TIMEOUT_ADD;
		
		/* If maximum of 10 timeouts is reached -> stop */
		if(cl->timeout == TIMEOUT_MAX)
		{
			cl->state = STOP;
			returnval = -1;
		}
	}
	return returnval;
}

/*	Send block to client, parameter:
		clientnode *cl	- client data structure
		
	Returns 
*/
void upload_mode(clientnode *cl)
{
	int n = 0;
	packet *pck = NULL;
	
	/* Check that we have data */
	if(cl->data != NULL) 
	{	
		/* If got ack for this data packet */
		if((cl->state == RUN) || (cl->state == START))
		{
			pck = encode_data_packet(cl->data);
			
			/* If sent less than package size -> stop */
			if((n = send_packet(cl->socket,pck)) != (cl->data->length + 4))
			{
				cl->state = STOP;
				perror("Sent less than package size to client!\n");
			}

			if(cl->data->length == 0)
			{
				cl->state = STOP;
				return;
			}			

			//printf("SENT: %d (%d bytes)\n",cl->data->block_id,n);
			n = check_ack(cl);
			
			/* Got ack, read next */
			if(n == 1)
			{	
				/* If previous wasn't less than block size */
				if(cl->data->length == TFTP_BLOCK)
				{
					n = read_file_to_block(cl);
					
					/* Error */
					if(n == -1)
					{
						pck = encode_error_packet(ERROR_ACCESS_VIOLATION);
						send_packet(cl->socket,pck);
					
						cl->state = STOP;
					}
					
					/* Otherwise data */
					else cl->state = RUN;
				}
				else
				{
					cl->state = STOP;
				}
				
			}
			
			/* Timeout */
			else if(n == 0) cl->state = RUN;
			
			/* Error */
			else if(n == -1) cl->state = STOP;
		}
	}
	
	/* No data, stop */
	else cl->state = STOP;
}

/*	Checks if there is an ACK message from client, parameter:
		clientnode *cl	- the data structure of client
	
	Returns int:
		1	- If got correct ack
		0	- If timeout happened or ack was incorrect
		-1	- If errors
*/
int check_ack(clientnode *cl)
{
	int rv = 0;
	
	packet *pck = NULL;
	ack_packet *a_pck = NULL;
	error_packet *e_pck = NULL;
	
	fd_set s_set;

	struct timeval to;
	
	FD_ZERO(&s_set);

	FD_SET(cl->socket,&s_set);

	to.tv_sec = TIMEOUT_SEC;
	to.tv_usec = cl->timeout;

	//printf("Timeout: %d\n",cl->timeout);
	rv = select(cl->socket+1,&s_set,NULL,NULL,&to);

	if(rv < 0)
	{
		perror("Error in upload select()");
		cl->state = STOP;
		return -1;
	}

	/* If something in socket, read */
	else if(rv > 0)
	{
		if(FD_ISSET(cl->socket,&s_set))
		{
			pck = read_packet(cl->socket);

			cl->timeout = TIMEOUT_USEC;
			/* Check length */
			if(pck->length == 4)
			{
				/* Decode ack packet and check that the blocknumbers match */
				a_pck = decode_ack_packet(pck);
				if(ntohs(a_pck->opcode) == OPCODE_ACK)
				{
					if(ntohs(a_pck->blockno) == cl->data->block_id)
					{
						cl->lastpacket += 1;
						//printf("ACK: %d for %d\n",ntohs(a_pck->blockno), cl->data->block_id);
						return 1;
					}
					else
					{
						return 0;
					}
				}

				/* If it is an error packet with empty message */
				/* This is highly unlikely scenario */
				else if(ntohs(a_pck->opcode) == OPCODE_ERROR)
				{
					e_pck = decode_error_packet(pck);
					printf("Error code %d\n",ntohs(e_pck->errcode));
					return -1;
				}

				/* Otherwise got something illegal, send error message */
				else
				{
					pck = encode_error_packet(ERROR_ILLEGAL_OPERATION);
					send_packet(cl->socket,pck);
					printf("Erroreus packet\n");
					return -1;
				}
			}

			/* If the message is some other TFTP message */
			else if((pck->length > 4) && (pck->length <= MAX_MSG))
			{
				e_pck = decode_error_packet(pck);
					
				/* Check if it is an error message and print the message */
				if(ntohs(e_pck->opcode) == OPCODE_ERROR)
				{
					e_pck = decode_error_packet(pck);
					printf("Error: %s\n",e_pck->content);
					return -1;
				}

				/* Otherwise got something illegal, send error message */
				else
				{
					pck = encode_error_packet(ERROR_ILLEGAL_OPERATION);
					send_packet(cl->socket,pck);
					printf("Erroreus packet\n");
					return -1;
				}
			}

			/* Otherwise got something illegal, send error message */
			else
			{
				pck = encode_error_packet(ERROR_ILLEGAL_OPERATION);
				send_packet(cl->socket,pck);
				printf("Erroreus packet\n");
				return -1;
			}
		}
		else return 0;
	}
	
	/* Timeout */
	else
	{
		/* Increase the timeout value */
		cl->timeout += TIMEOUT_ADD;
		
		/* If maximum of 10 timeouts is reached -> stop */
		if(cl->timeout == TIMEOUT_MAX) cl->state = STOP;

		return 0;
	}
}
