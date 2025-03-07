#CC=aarch64-none-linux-gnu-gcc
CC=gcc
CFLAGS=-Wall -g

all: aesdsocket

aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c -o aesdsocket.o 

default: aesdsocket.o
	$(CC) $(CFLAGS) aesdsocket.o -o aesdsocket

clean:
	rm -f aesdsocket.o aesdsocket
