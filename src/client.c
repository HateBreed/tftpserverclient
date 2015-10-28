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

And of course linux man pages.

*/

#include "client.h"


/* Mode of the client (upload/download) */
enum cl_mode clientmode;

int main(int argc, char *argv[])
{
	int port = 0;
	int n = 0;
	struct hostent *host;	
	
	/* Check the given parameters and report error if any then quit */ 
	if(argc != PARAMS) param_err();
	else
	{
		if((host = gethostbyname(argv[1])) == 0) error("Error: Unknown hostname");
		if((port = atoi(argv[2])) == 0) error("Error: Invalid port");
		
		/* Set mode */
		if(strcmp(argv[3],"upload") == 0) clientmode = UPLOAD;
		else if(strcmp(argv[3],"download") == 0) clientmode = DOWNLOAD;
		else error_minor("Error: Unknown mode");
	}
	
	/* Run the client */
	if((n = runClient(host,port,argv[4])) != 1)
	{
		
		printf("Error happened.");
		
		if(clientmode == DOWNLOAD)
		{
			if(!remove(argv[4])) printf(" File \"%s\" removed.\n",argv[4]);
			else printf(" File \"%s\" cannot be removed.\n",argv[4]);
		}
		else printf("\n");
	}

	return 0;
}

/*	Starts running the client in given mode, parameters:
		struct hostent *hostname	- host information
		int port			- port used by client
		char *filename			- file to be uploaded / downloaded
	
	Return 0 if successful.
*/
int runClient(struct hostent *hostname, int port, char *filename)
{
	int sockCl = 0;
	int n = 0, returnval = 1;
	packet *pck = NULL;
	filedata *files = NULL;
	ack_packet *a_pck = NULL;
	error_packet *e_pck = NULL;
	
	clientnode *this_node = NULL;
	
	/* Address to which the client connects */
	struct sockaddr_in conn_address;
	
	/* Own address, used for receiving */
	struct sockaddr_in own_address;

	unsigned int addr_length;
	
	/* Create socket */
	if((sockCl = socket(AF_INET,SOCK_DGRAM,0)) < 0) error("Error creating socket");
	
	/* Set own address data */
	own_address.sin_family = AF_INET;
	own_address.sin_addr.s_addr = htons(INADDR_ANY);
	own_address.sin_port = htons(0); /* Let bind choose the port */
	
	/* Set host address data */
	conn_address.sin_addr = *((struct in_addr *)hostname->h_addr);
	conn_address.sin_port = htons(port);
	conn_address.sin_family = AF_INET;
	
	addr_length = sizeof(struct sockaddr_in);
	
	/* Bind socket to own address */
	if(bind(sockCl,(struct sockaddr *)&own_address,addr_length) < 0) error("Error in socket binding");
	
	/* Set connection address (for upload/download loops) */
	set_conn_addr(conn_address);
	
	/* Set client info */
	this_node = add_client(this_node, conn_address, sockCl, filename,clientmode,files);

	switch(clientmode)
	{
		/* Upload mode, read file to blocks, start the loop and finally free filedata */
		case UPLOAD: 
			
			if((read_file_to_block(this_node)) != -1)
			{
				/* Create and send initialization packet in correct mode */
				pck = encode_init_packet(this_node->mode,this_node->filename,"octet");
				n = send_packet(this_node->socket,pck);
			
				/* Read the packet from socket - expecting ack with 0 block number */
				pck = read_packet(this_node->socket);

				/* If length is 4 it is probably ack packet, decode and check block number */
				if(pck->length == 4)
				{
					a_pck = decode_ack_packet(pck);
				
					/* Got correct ack -> start loop */
					if((ntohs(a_pck->opcode) == OPCODE_ACK) && (ntohs(a_pck->blockno) == this_node->lastpacket))
					{
						this_node->state = RUN;
						returnval = upload_mode_loop(this_node);
						if(returnval == 1) printf("Uploaded %ld bytes.\n",this_node->filesize);
					}
				
					/* Otherwise server sent something else than ack packet */
					else
					{
						returnval = -1;
					}
				}
				
				/* Otherwise probably an error packet */
				else
				{
					e_pck = decode_error_packet(pck);
					
					if(ntohs(e_pck->opcode) == OPCODE_ERROR)
					{
						printf("Server sent error: %s\n",e_pck->content);
					}
					else printf("Server sent odd packet (code: %d)\n",ntohs(e_pck->opcode));
				}
				
				this_node = remove_stopped(this_node);
				
				free(pck);
			}
			
			break;

		/* Download mode, start the loop and write blocks to file */
		case DOWNLOAD: 
			
			if(check_file(filename) != 0) break;
			
			else
			{
				/* Create and send initialization packet in correct mode */
				pck = encode_init_packet(this_node->mode,this_node->filename,"octet");
				n = send_packet(this_node->socket,pck);
	
				this_node->state = RUN;
				
				/* Start downloading */
				returnval = download_mode_loop(this_node);
				printf("Downloaded %ld bytes.\n",this_node->filesize);
				this_node = remove_stopped(this_node);
				
				free(pck);
				break;
			}

		/* Unknown mode - should not happen */
		default:
			returnval = -1;
	};
	
	return returnval;
}
