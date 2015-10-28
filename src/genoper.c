/* Author: Jussi Laakkonen /  */

#include "genoper.h"


/*	Adds new data block to linked list, parameters:
		filedata *first	- pointer to first data block element
		char *newdata	- new data to be added
		int length	- length of data

	Returns the pointer to first element in data block list
*/
filedata *add_file_block(filedata *first,char *newdata, int length)
{
	filedata *current = NULL;
	
	/* Create new element and set data */
	filedata *newFD = (filedata *)malloc(sizeof(filedata));
	memcpy(newFD->data,newdata,length);
	newFD->length = length;
	newFD->next = NULL;

	/* Search place */
	if(first != NULL)
	{
		current = first;
		while(current->next != NULL) current = current->next;
		
		/* When place found, set block number */
		newFD->block_id = current->block_id + 1;
		
		/* Add to list as last */
		current->next = newFD;
		return first;
	}
	else
	{
		/* Otherwise list is empty, block added as first */
		newFD->block_id = 1;
		return newFD;
	}
}

filedata *put_file_block(filedata *last,char *newdata, int length)
{
	filedata *newFD = (filedata *)malloc(sizeof(filedata));
	memcpy(newFD->data,newdata,length);
	newFD->length = length;
	newFD->next = NULL;
	
	if(last == NULL) newFD->block_id = 1;
	else
	{
		newFD->block_id = last->block_id + 1;
		free(last);
		last = NULL;
	}
	
	return newFD;
}

/*	Adds new data in data packet to linked list, parameters:
		filedata *first		- pointer to first data block element
		data_packet *d_packet	- Data packet to be added
		int length		- total length of the packet

	Returns the pointer to first element in data block list
*/
filedata *add_data_packet(filedata *first, data_packet *d_packet, int length)
{
	filedata *current = NULL;

	/* Create new element */
	filedata *newFD = (filedata *)malloc(sizeof(filedata));	

	/* Set data */
	newFD->block_id = ntohs(d_packet->blockno);
	newFD->length = length - sizeof(uint16_t) - sizeof(uint16_t);
	memcpy(newFD->data,d_packet->data,length);
	newFD->next = NULL;

	/* Search place and add to correct place */
	if(first != NULL)
	{
		current = first;
		
		while(current->next != NULL) current = current->next;

		current->next = newFD;

		return first;
	}
	/* Otherwise added as first */
	else return newFD;
}

/*	Write a block to file. Writes all available blocks to file, parameter:
		clientnode *cl	- client data structure
		
	Returns the amount of bytes written
*/
unsigned long write_block_to_file(clientnode *cl)
{
	unsigned long bytes_written = 0;
	filedata *temp = NULL;
	
	/* Check if the file needs to be opened */
	if(cl->filed == -1)
	{
		printf("Opening file \"%s\"... ",cl->filename);
		/* If file cannot be opened for writing, return -1 */
		if((cl->filed = open(cl->filename,O_CREAT|O_WRONLY,S_IRWXU)) == -1)
		{
			printf("cannot open.\n");
			return -1;
		}
		else printf("file opened!\n");
	}
	
	/* Go through all data blocks */
	while(cl->data != NULL)
	{
		bytes_written += write(cl->filed,cl->data->data,cl->data->length);
		
		/* If error happened, stop and leave data */
		if(bytes_written == -1) return bytes_written;
		
		/* Otherwise handle next block */
		else
		{
			temp = cl->data;
		
			cl->data = cl->data->next;
		
			free(temp);
			temp = NULL;
		}
	}
	
	cl->filesize += bytes_written;
	
	/* If last (less than tftp block) close the file descriptor */
	if(bytes_written < TFTP_BLOCK)
	{
		if(cl->filed != -1)
		{
			if(close(cl->filed) == 0)
			{
				cl->filed = -1;
				printf("File \"%s\" was closed. File size: %ld bytes.\n",cl->filename, cl->filesize);
			}
		}
	}
	
	return bytes_written;
}

/*	Reads a block from a file, parameter:
		clientnode *cl	- client data structure
		
	Returns amount of bytes read
*/
int read_file_to_block(clientnode *cl)
{
	int bytes_read = 0;
	char buf[TFTP_BLOCK];
	
	/* If not open */
	if(cl->filed == -1)
	{
		printf("Opening file \"%s\"... ",cl->filename);
		
		/* Try to open, if failure return -1 */
		if((cl->filed = open(cl->filename,O_RDONLY)) == -1)
		{
			printf("cannot open for reading.\n");
			return -1;
		}
		else printf("file opened\n");
	}

	bzero(&buf,TFTP_BLOCK);
	
	/* Read */
	bytes_read = read(cl->filed,&buf,TFTP_BLOCK);
	
	/* If it was successful add data block */
	if(bytes_read != -1)
	{
		//printf("Read %d bytes from \"%s\"\n",bytes_read,cl->filename);
		
		cl->filesize += bytes_read;
		
		cl->data = put_file_block(cl->data,buf,bytes_read);
	}
	
	/* If EOF */
	if(bytes_read == 0)
	{
		if(close(cl->filed) == 0)
		{
			cl->filed = -1;
			printf("File \"%s\" was closed. File size: %ld bytes\n",cl->filename, cl->filesize);
		}
	}
	
	return bytes_read;
}

/*	Creates a node for connecting client and adds it to list (as first), parameters:
		clientnode *first		- pointer to first node in the list
		struct sockaddr_in addr	- address info of connecting client
		int socket			- socket for this client (TID - port)
		char *filename		- the file client wants to write/request
		int mode				- client mode: RRQ / WRQ
		filedata *firstdata		- pointer to first node that contains first data (WRQ - this is null)
		
	Returns the pointer to first clientnode (this)
*/
clientnode *add_client(clientnode *first, struct sockaddr_in addr, int socket, char *filename, int mode, filedata *data)
{
	clientnode *newnode = (clientnode *)malloc(sizeof(clientnode));
	
	/* Set address*/
	newnode->address = addr;
	
	/* Set socket */
	newnode->socket = socket;
	
	/* Set filename */
	newnode->filename = filename;
	
	newnode->filesize = 0;
	
	/* Set mode */
	newnode->mode = mode;
	
	/* Set data */
	newnode->data = data;
	
	/* Set filedescriptor */
	newnode->filed = -1;

	/* Set last packet counter RRQ - 1, WRQ - 0 */
	switch(mode)
	{
		case OPCODE_RRQ:
			newnode->lastpacket = 1;
			break;

		case OPCODE_WRQ:
			newnode->lastpacket = 0;
			break;
	}
	
	/* Set state */
	newnode->state = START;
	
	/* Set timeout */
	newnode->timeout = TIMEOUT_USEC;
	
	/* Add to list */
	if(first != NULL) newnode->next = first;
	else newnode->next = NULL;
	
	return newnode;
}

/*	Removes all stopped clients from client list, parameter:
		clientnode *first	- pointer to first node in the list
		
	Returns pointer to first node in the cleared list.
*/
clientnode *remove_stopped(clientnode *first)
{
	clientnode *current, *temp = NULL, *prev = NULL;
	
	if(first != NULL)
	{
		current = first;
		
		while(current != NULL)
		{	
			/* If needs to be removed */
			if(current->state == STOP)
			{
				/* If file was open, close */
				if(current->filed != -1)
				{
					if(close(current->filed) == 0)
					{
						current->filed = -1;
						printf("File \"%s\" was closed. File size: %ld bytes\n",current->filename,current->filesize);
					}
				}
				
				/* First */
				if(prev == NULL)
				{
					temp = current;
					
					current = current->next;
					
					first = current;
					
					/* Free data and others */
					free_filedata(temp->data);
					close(temp->socket);
					free(temp);
					temp = NULL;
				}
				
				/* Last */
				else if(current->next == NULL)
				{
					prev->next = NULL;
					
					/* Free data and others */
					free_filedata(current->data);
					close(current->socket);
					free(current->filename);
					free(current);
					current = NULL;
				}
				
				/* Middle */
				else
				{
					temp = current;
					
					prev->next = current->next;
					
					current = current->next;
					
					/* Free data and others */
					free_filedata(temp->data);
					close(temp->socket);
					free(temp->filename);
					free(temp);
					temp = NULL;
				}
			}
			else
			{
				prev = current;
				current = current->next;
			}
		}
		
	}

	return first;
}

/* 	Checks if the file exists, parameter:
		char *filename 	- the name of the file to be checked 

	Returns ERROR_FILE_EXISTS code or 0 if it doesn't exist
*/
int check_file(char *filename)
{
	int filed;
	
	/* Try to open the file */
	if((filed = open(filename,O_WRONLY)) != -1)
	{
		/* If the file exists and it can be opened , close file descriptor
		and report error - doesn't allow to overwrite files */
		close(filed);
		printf("File \"%s\" exists.\n",filename);
		return ERROR_FILE_EXISTS;
	}

	/* If the opening didn't succeed, check if it can be created */
	else
	{
		if(check_file_write(filename) == 0) return 0;
		else return ERROR_ACCESS_VIOLATION;
	}
}

/*	Checks if the file can be overwritten, parameter:
		char *filename - the name of the file to be checked
		
	Returns ERROR_ACCESS_VIOLATION code or 0 if it can be written
*/
int check_file_write(char *filename)
{
	int filed;
	
	/* Try to open it (create if not exists) for writing 
	 and grant rights for the file to the user running this instance */ 
	if((filed = open(filename,O_CREAT|O_WRONLY,S_IRWXU)) == -1)
	{
		printf("Cannot open file \"%s\" for writing.\n",filename);
		return ERROR_ACCESS_VIOLATION;
	}
	
	/* File can be written, close filed and return 0 */
	else
	{
		close(filed);
		return 0;
	}
}

/* Clear the list */
void free_filedata(filedata *first)
{
	int count = 0;
	filedata *prev;
	
	while(first != NULL)
	{
		prev = first;
		first = first->next;
		free(prev);
		count++;
	}

	//printf("Free'd %d blocks\n",count);
	return;
}
