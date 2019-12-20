all: client server

client: client.c util.h util.c
	gcc client.c util.c -o client

server: server.c util.c util.h
	gcc server.c util.c -o server