# Author: Jussi Laakkonen /  

CFLAGS = -Wall

SERVER = server
CLIENT = client
SERVERSRC = server.c srvoper.c
CLIENTSRC = client.c clioper.c
SOURCES = netoper.c genoper.c structures.c errors.c

all: build

build: $(SOURCES) 
	gcc $(CFLAGS) $(CLIENTSRC) $(SOURCES) -o $(CLIENT)
	gcc $(CFLAGS) $(SERVERSRC) $(SOURCES) -o $(SERVER)

server: $(SOURCES)
	gcc $(CFLAGS) $(SERVERSRC) $(SOURCES) -o $(SERVER)
	
client: $(SOURCES)
	gcc $(CFLAGS) $(CLIENTSRC) $(SOURCES) -o $(CLIENT)

clean: rmclient rmserver

rmclient:
	rm -v $(CLIENT)
	
rmserver:
	rm -v $(SERVER)
