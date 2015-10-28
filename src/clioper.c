/* Author: Jussi Laakkonen /  */

#include "clioper.h"


/* 	Loop for downloading file, parameters:
		int sock	- socket to be used
		int type	- type of the instance (server or client)

	Returns pointer to first filedata block, stops the loops when receiving error
*/
int download_mode_loop(clientnode *cl)
{
	int n = 0, packet_len = 0, rv = 0;
	int returnval = 1;
	struct timeval to;
	packet *pck = NULL;
	data_packet *d_pck = NULL;
	error_packet *e_pck = NULL;
	fd_set s_set;

	FD_ZERO(&s_set);

	while((cl->state == RUN) || (cl->state == WAIT))
	{
		/* Set timeout values */
		to.tv_sec = TIMEOUT_SEC;
		to.tv_usec = cl->timeout;

		FD_SET(cl->socket,&s_set);

		/* Check if there is something in socket */
		rv = select(cl->socket+1,&s_set,NULL,NULL,&to);

		if(rv < 0)
		{
			pck = encode_error_packet(ERROR_NOT_DEFINED);
			send_packet(cl->socket,pck);
			perror("Error in select()");
			cl->state = STOP;
			returnval = -1;
		}

		/* If got something */
		else if(rv > 0)
		{
			if(FD_ISSET(cl->socket,&s_set)) /* Used socket has something */
			{
				/* Read packet and backup length */
				pck = read_packet(cl->socket);
				packet_len = pck->length;
				
				/* Reset timeout */
				cl->timeout = TIMEOUT_USEC;
				
				/* If packet is less than 4 bytes or greater than 516 bytes (MAX_MSG) discard it */
				if((packet_len < 4) || (packet_len > MAX_MSG))
				{
					pck = encode_error_packet(ERROR_ILLEGAL_OPERATION);
					error_minor("Invalid packet size, discarding");
					send_packet(cl->socket,pck);
				}

				/* Otherwise check the packet */
				else
				{
					if((d_pck = decode_data_packet(pck)) == NULL)
					{
						pck = encode_error_packet(ERROR_NOT_DEFINED);
						send_packet(cl->socket,pck);
						perror("Error decoding packet");
						cl->state = STOP;
						returnval = -1;
						break;
					}

					/* If it is a DATA packet */
					if(ntohs(d_pck->opcode) == OPCODE_DATA)
					{
						//printf("GOT: %d (%d bytes)\n",ntohs(d_pck->blockno),packet_len);
						/* Check blocknumber that it is the same as waited block*/
						if(ntohs(d_pck->blockno) == cl->lastpacket)
						{
							if(packet_len == 4)
							{
								cl->state = STOP;
								returnval = 1;
								break; /* If data packet with 0 data -> last packet */
							}
							
							/* Add data packet to fileblock list */
							cl->data = add_data_packet(cl->data,d_pck,packet_len);
							
							if(write_block_to_file(cl) == -1)
							{
								pck = encode_error_packet(ERROR_ACCESS_VIOLATION);
								send_packet(cl->socket,pck);
								cl->state = STOP;
							}
							
							/* Create ack packet for this packet and send it */
							pck = encode_ack_packet(ntohs(d_pck->blockno));
							n = send_packet(cl->socket,pck);

							//printf("Sent ACK: %d\n",ntohs(d_pck->blockno));
							/* Increase the block counter */
							cl->lastpacket += 1;

							/* If got less data than max 512 bytes -> last packet */
							if(packet_len != MAX_MSG)
							{
								cl->state = STOP;
								returnval = 1;
								break;
							}
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
						break;
					}

					/* Otherwise a errorous packet */
					else
					{
						printf("Erroreus packet");
						
						pck = encode_error_packet(ERROR_ILLEGAL_OPERATION);
						send_packet(cl->socket,pck);
						
						cl->state = STOP;
						returnval = -1;
						
						break;
					}
				}
			}
		}
		/* If timeout happened, send re-ack if not waiting for first block */
		else
		{
			/* client cannot send ack with block number 0 */
			if(cl->lastpacket != 1)
			{
				pck = encode_ack_packet(cl->lastpacket-1);
				n = send_packet(cl->socket,pck);
			}
			
			cl->state = WAIT;
			
			cl->timeout += TIMEOUT_ADD;
			
			/* If max reached */
			if(cl->timeout == TIMEOUT_MAX)
			{
				printf("Timeout happened.\n");
				cl->state = STOP;
				returnval = -1;
			}
		}
	}

	free(pck);
	free(d_pck);

	return returnval;
}

/*	Loop for uploading a file for client, parameters:
		int sock 		- socket used for sending
		filedata *files - pointer to first fileblock

	Returns 0 if successful, stops the loop if an error message is received
*/
int upload_mode_loop(clientnode *cl)
{
	int n = 0, rv = 0, returnval = 1, last_round = 0;
	struct timeval to;
	packet *pck = NULL;
	ack_packet *a_pck = NULL;
	error_packet *e_pck = NULL;
	fd_set s_set;

	FD_ZERO(&s_set);
	/* Start uploading */
	while(cl->state == RUN || cl->state == WAIT)
	{
		/* Timeout values */
		to.tv_sec = TIMEOUT_SEC;
		to.tv_usec = cl->timeout;

		FD_SET(cl->socket,&s_set);

		/* Create data packet and send it, quit if sent less than package size */
		pck = encode_data_packet(cl->data);
		if((n = send_packet(cl->socket,pck)) < pck->length) printf("Sent less than package size!");
		
		//printf("SENT: %d (%d bytes)\n",cl->data->block_id, cl->data->length);
		/* Check socket */
		rv = select(cl->socket+1,&s_set,NULL,NULL,&to);

		if(rv < 0)
		{
			pck = encode_error_packet(ERROR_NOT_DEFINED);
			send_packet(cl->socket,pck);
			error("Error in select()");
		}

		/* If got something */
		else if (rv > 0)
		{
			if(FD_ISSET(cl->socket,&s_set))
			{
				/* Read packet */
				pck = read_packet(cl->socket);

				/* Reset timeout */
				cl->timeout = TIMEOUT_USEC;
				/* Check length */
				if(pck->length == 4)
				{
					/* Decode ack packet */
					a_pck = decode_ack_packet(pck);
					if(ntohs(a_pck->opcode) == OPCODE_ACK)
					{
						/* check that the blocknumbers match */
						if(ntohs(a_pck->blockno) == cl->data->block_id)
						{
							//printf("ACK: %d for packet: %d\n",ntohs(a_pck->blockno),cl->data->block_id);
							if(last_round == 1)
							{
								returnval = 1;
								cl->state = STOP;
								break;
							}
							n = read_file_to_block(cl);
							
							/* EOF */
							if(n == 0)
							{
								cl->state = RUN;
								last_round = 1;
								returnval = 1;
							}
							/* Data */
							else if(n != -1)
							{
								/* If last block */
								if(n < TFTP_BLOCK) last_round = 1;
								cl->state = RUN;
								returnval = 1;
							}
							/* Error */
							else
							{
								pck = encode_error_packet(ERROR_ACCESS_VIOLATION);
								send_packet(cl->socket,pck);
								cl->state = STOP;
								returnval = -1;
							}
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
		}
		/* Timeout */
		else
		{
			cl->state = WAIT;
			
			cl->timeout += TIMEOUT_ADD;
			
			/* If max reached */
			if(cl->timeout == TIMEOUT_MAX)
			{
				printf("Timeout happened.\n");
				cl->state = STOP;
				returnval = -1;
			}
		}
	}

	return returnval;
}
