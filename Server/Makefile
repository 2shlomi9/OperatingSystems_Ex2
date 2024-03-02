
CC = gcc
CFLAGS = -Wall

all: server 

server: Server.o
	$(CC) $(CFLAGS) -o server Server.o -lcrypto

clean:
	rm -f server *.o
	
