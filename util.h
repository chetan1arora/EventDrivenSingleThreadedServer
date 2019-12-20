#ifndef __UTILS__HDR__
#define __UTILS__HDR__

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <ctype.h>
#include <sys/shm.h>
#include <sys/stat.h>


#define PORT 3000
#define MSG_LEN 200
#define IP "172.17.11.102"

typedef struct Note{
	int from;
	int to;
	char msg[MSG_LEN];
} note;

struct msgbuf{
	long mtype;
	note t;
};

typedef struct Safe{
	pthread_mutex_t lock;
	int v;
} safe;

struct threadvar{
	int fd;
	int db;
	safe* left;
	safe* done;
};

int _tcp();
int _sem(int n);
int _queue();
struct sockaddr_in _addr(int ip, int port);
int _bind(int fd, int port);
int _connect(int fd, char* ip, int port);
int _accept(int fd);
int _send(int sock, char* msg);
int _recv(int fd, char* msg);
int _write(int q, note t);
int _read(int q, note* t, int to);
safe* _safe(int n);
void _lock(safe* a);
void _unlock(safe* a);

#endif
