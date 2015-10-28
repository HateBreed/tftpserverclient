Readme for TFTP server and client
Sixth exercise for Network Programming course
Author:	Jussi Laakkonen / 

Task:

To implement a TFTP server and client according to RFC 1350.



Improvements:

	Retransmission improved (was also in previous versions but there were some minor flaws).

	Error messages - errors are detected and send to peer.

	Files will be written and read block by block when needed, previously the whole file was read into memory
	before sending and all received blocks stayed in memory until the transfer was finished.
	
	Files are checked (that it can be read/written) and also if the file exists - doesn't allow to write over.
	
	Server supports multiple simultaneous clients (amount not limited).

	Server can be set to stop, it waits for current file transfers to finish.


Files:

	server.c - Server code, new connection initialization.
	server.h - Header file of server.c - includes server operations (srvoper.h).
	
	srvoper.c - Server functions, uploading/downloading, port creation etc.
	srvoper.h - Header file of srvoper.h - includes net operations (netoper.h).
	
	client.c - Client code, connection parameters and connection initialization.
	client.h - Header file of client.c - includes client operations (clioper.h).
	
	clioper.c - Client functions, loops for upload/download.
	clioper.h - Header file of clioper.c - includes net operations (netoper.h).

	netoper.c - Sending/receiving of packets.
	netoper.h - Header file of tftpoper.c - includes generic operations (genoper.h)
	
	genoper.c - Generic operations, adding blocks to linked list, clearing list, file write/read operations.
	genoper.h - Header file of genoper.c - includes structures and encoding & decoding functions (structures.h)

	structures.c - Encoding and decoding of messages.
	structures.h - Header file of structures.c, contains packet structures - includes definitions.h and errors.h

	errors.c - Error message printing, provides functions for error messages (and quit) and parameter errors. 
	errors.h - Header file of errors.c.
	
	definitions.h - The definitions file, generic includes and compile-time parameter definitions
	
	Makefile - The makefile.
	

Building the code:

	Build both:
		make 
		make build
		- As a result server- and client-executables are compiled
	
	Build server:
		make server
		-  Results server-executable file.
		
	Build client:
		make client
		 - Results client-executable file.
	
	Builds the program with gcc using -Wall flag.

Removing executables:

	Remove both:
		make clean
		- Removes client and server executables

	Remove client:
		make rmclient
		- Removes client executable

	Remove server:
		make rmserver
		- Removes server executable
Usage:

	Server:

		Define port:
		
			./server <port>
				<port> 	- Port used by server
			
		With default TFTP port:
		
			./server
				Without parameter the default TFTP server port (69) is used. This might not work on some machines
				if user does not have necessary rights to use that particular port.
			
	
	Client:
		Upload:
	
			./client <hostname> <port> upload <filename>
				<hostname>	- The hostname of the server
				<port>		- The port of the TFTP server
				<filename>	- File to be uploaded

		Download:
	
			./client <hostname> <port> download <filename>
				<hostname>	- The hostname of the server
				<port>		- The port of the TFTP server
				<filename>	- File to be downloaded

	If program detects that given parameters are incorrect, usage help will be printed.


Other info:

	Packets are decoded and encoded by using cast, it might not be the best way to do it but it seems to work 
	(at least in CURL TFTP implementation it was used, I got the idea from there). I tried using similar methods
	presented at Beej's Guide to Network Programming but I didn't get it working with those ideas.

	The "packet" is sent as a void pointer (char pointer cast to void pointer) and received also in similar 
	fashion. The actual packet is got by casting received packet to structure pointer.

	Select is used to detect packets in socket and retransmission is also supported in both modes (upload and
	download) by server and client.

	The timeout values can be changed by modifying values of TIMEOUT_SEC and TIMEOUT_USEC in definitions.h file.
	Default value for timeout is 0.010 seconds.

	If server detects that client has timeouted too many times server will stop that client, when a packet is received from client,
	server resets this counter, packet doesn't need to be correct one, when packet is received it means that client is alive.
	Server increments the timeout of a client by 0.010 seconds (TIMEOUT_ADD) at a time and when it reaches 0.100
	(TIMEOUT_MAX) seconds client is stopped. Same functionality is implemented also for the client.

	When writing to a file the file is created (if previous found, it is not overwritten) and read-write-execute
	rights are given to the user running the program. Both client and server check if the file exists and it can be written,
	if operation is not permitted, program (client) / connection (server) is closed. 

	Normal errors (file not found etc.) are detected and sent to client by server. When error occurs server stops serving that client.
	If something goes wrong during the file download in either side, it will be detected and program tries to remove the unfinished file.
	
	Multiple clients can be handled simultaneously, one operation is performed in every loop for every client.
	
	Server creates a random port for every client (between 1000 and 65000) and checks if that port can be bound, if not a new
	port number is generated by taking the next pseudo random number with the previous seed (time is used as seed).
	
	When client is set to STOP state in server server closes the file descriptor of that client if it wasn't closed earlier. Server cleans the list
	of clients every round.

	Server can be closed by typing CLOSE (not case sensitive), it waits for clients to finish and doesn't allow new connections after that
	(stops listening the server socket).

Compile time options:

	In client.h
		PARAMS			- amount of parameters needed for client.

	In server.h
		PARAMS 			- amount of parameters needed for server.

	In definitions.h
	
		TFTP OPCODES:
			OPCODE_RRQ		- TFTP read request code
			OPCODE_WRQ		- TFTP write request code
			OPCODE_DATA 		- TFTP data code
			OPCODE_ACK		- TFTP ack code
			OPCODE_ERROR 		- TFTP error code
			
		TFTP ERROR CODES:
			ERROR_NOT_DEFINED	- TFTP error 0
			ERROR_NOT_FOUND		- TFTP error 1
			ERROR_ACCESS_VIOLATION	- TFTP error 2
			ERROR_DISK_FULL		- TFTP error 3
			ERROR_ILLEGAL_OPERATION	- TFTP error 4 
			ERROR_UNKNOWN_ID	- TFTP error 5
			ERROR_FILE_EXISTS	- TFTP error 6
			ERROR_NO_SUCH_USER	- TFTP error 7
		
		TFTP GENERAL:
			TFTP_BLOCK		- Size of TFTP datablock
			MAX_MSG			- Maximum length of message
			TFTP_SERVER_PORT	- Default port of TFTP server
		
		TIMEOUTS:
			TIMEOUT_SEC		- Timeout length in seconds
			TIMEOUT_USEC		- Timeout length in micro seconds
			TIMEOUT_ADD		- Time that is added to client timeout in micro seconds
			TIMEOUT_MAX		- Maximum timeout value for client in micro seconds
		


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
	
	And of course linux man pages.


	------------------------------
	- Jussi Laakkonen 3.1.2008   -
	------------------------------
