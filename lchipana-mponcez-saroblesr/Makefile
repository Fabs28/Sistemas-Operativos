# Makefile - practica 3 sistemas operativos

CC     = gcc
CFLAGS = -Wall -g

all: p2-server p2-client

p2-server: p2-server.c hash.c hash.h protocolo.h
	$(CC) $(CFLAGS) p2-server.c hash.c -o p2-server

p2-client: p2-client.c protocolo.h
	$(CC) $(CFLAGS) p2-client.c -o p2-client

clean:
	rm -f p2-server p2-client server.log

.PHONY: all clean
