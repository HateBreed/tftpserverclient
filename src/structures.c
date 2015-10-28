/* Author: Jussi Laakkonen /  */

#include "structures.h"

/* Error messages */
static char *error_messages[] =
{
	"Not defined, see error message (if any).",
	"File not found.",
	"Access violation.",
	"Disk full or allocation exceeded.",
	"Illegal TFTP operation.",
	"Unknown transfer ID.",
	"File already exists.",
	"No such user.",
};

/*	Encodes initialization packet, parameters:
		int code_	- opcode, WRQ or RRQ
		char *filename	- name of the file
		char *mode	- data mode (only octet)

	Returns the encoded packet as packet struct.
*/
packet* encode_init_packet(int code_, char *filename_,char *mode_)
{
	int length = 0;
	/* Create both packets */
	init_packet *i_pck = (init_packet *)malloc(sizeof(init_packet));
	packet *pck = (packet *)malloc(sizeof(packet));

	/* Get length */
	length = sizeof(uint16_t) + strlen(filename_) + 1 + strlen(mode_) + 1;

	/* Set opcode */
	i_pck->opcode = htons(code_);

	/* Put parameters to content */
	snprintf(i_pck->content,sizeof(i_pck->content),"%s%c%s%c",filename_,'\0',mode_,'\0');
	
	/* Set length of packet and cast init_packet to char pointer
		- probably not the best way but it works */
	pck->length = length;
	pck->content = (char *)i_pck;

	return pck;
}

/*	Decodes the init packet from general packet, parameter:
		packet *pck - the packet containing init packet
		
	Returns the decoded init packet
*/
init_packet* decode_init_packet(packet *pck)
{
	/* Create init packet */
	init_packet *i_pck = (init_packet *)malloc(sizeof(init_packet));
	
	/* Cast content of packet to init_packet type pointer */
	i_pck = (init_packet *)pck->content;
	
	return i_pck;
}

/*	Encode data packet from filedata, parameter:
		filedata *data	- Data block to be encoded

	Returns packet containing encoded data packet
*/
packet* encode_data_packet(filedata *data)
{
	int length = 0;
	
	/* Create both packets */
	data_packet *data_pck = (data_packet *)malloc(sizeof(data_packet));
	packet *pck = (packet *)malloc(sizeof(packet));

	/* Get length */
	length = sizeof(uint16_t) + sizeof(uint16_t) + data->length;

	/* Set data */
	data_pck->opcode = htons(OPCODE_DATA);
	data_pck->blockno = htons(data->block_id);
	memcpy(data_pck->data,data->data,data->length);

	/* Set length and cast data_packet to char pointer */
	pck->length = length;
	pck->content = (char *)data_pck;

	return pck;
}

/*	Decodes the data packet from general packet, parameter:
		packet *pck	- the packet containing data packet

	Returns the decoded data_packet
*/
data_packet* decode_data_packet(packet *pck)
{
	/* Create data packet */
	data_packet *data_pck;

	/* Cast content of packet to data_packet */
	data_pck = (data_packet *)pck->content;

	return data_pck;
}

/*	Encode ack packet for a data_block, parameter:
		int block	- the block number

	Retuns the encoded ack_packet as packet
*/
packet* encode_ack_packet(int block)
{
	int length = 0;

	/* Create both packets */
	ack_packet *a_pck = (ack_packet *)malloc(sizeof(ack_packet));
	packet *pck = (packet *)malloc(sizeof(packet));
	
	/* Get length and set data */
	length = sizeof(uint16_t) + sizeof(uint16_t);
	a_pck->opcode = htons(OPCODE_ACK);
	a_pck->blockno = htons(block);
	
	/* Set length and cast ack_pck to char pointer */
	pck->length = length;
	pck->content = (char *)a_pck;

	return pck;
}

/*	Decodes the ack_packet from general packet, parameter:
		packet *pck	- the packet containing ack packet

	Returns the decoded ack_packet
*/
ack_packet *decode_ack_packet(packet *pck)
{
	/* Create ack packet */
	ack_packet *ack_pck;

	/* Cast content of packet to ack_packet */
	ack_pck = (ack_packet *)pck->content;

	return ack_pck;
}

/* 	Encode error packet with given errorcode, parameter:
		int errorcode	- The code which indicates the type of error
		
	Returns the encoded error packet as a packet
*/
packet* encode_error_packet(int errorcode)
{
	int length = 0;
	
	/* Create packets */
	error_packet *e_pck = (error_packet *)malloc(sizeof(error_packet));
	packet *pck = (packet *)malloc(sizeof(packet));
	
	/* Set values */
	e_pck->opcode = htons(OPCODE_ERROR);
	e_pck->errcode = htons(errorcode);
	
	/* Length to content = message + NULL char */
	length = strlen(error_messages[errorcode]) + 1;
	
	/* Put message and NULL char into error packet */
	snprintf(e_pck->content,length,"%s%c",error_messages[errorcode],'\0');
	
	length = length + sizeof(uint16_t) + sizeof(uint16_t);
	
	/* Cast to char pointer and set values to packet */
	pck->length = length;
	pck->content = (char *)e_pck;
	
	return pck;
}

/*	Decodes an error packet from packet, parameter:
		packet *pck	- the packet containing error packet
		
	Returns decoded error_packet
*/
error_packet *decode_error_packet(packet *pck)
{
	/* Error packet */
	error_packet *e_pck;
	
	/* Cast the content of packet to error_packet */
	e_pck = (error_packet *)pck->content;

	return e_pck;
}
