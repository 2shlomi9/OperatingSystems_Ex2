
CC = gcc
CFLAGS = -Wall

all: client

client: Client.o
	$(CC) $(CFLAGS) -o client Client.o -lcrypto

clean:
	rm -f client *.o
	
