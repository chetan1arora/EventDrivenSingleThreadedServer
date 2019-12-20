#include "util.h"

int _tcp(){
	return socket(AF_INET,SOCK_STREAM,0);
}

int _sem(int n){
	return semget(IPC_PRIVATE,n, IPC_CREAT | 0666);
}

safe* _safe(int n){
	safe* s = (safe*)malloc(sizeof(safe));
	s->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	s->v = n;
	return s;
}

struct sockaddr_in _addr(int ip, int port){
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(port);
	servaddr.sin_addr.s_addr=ip;
	return servaddr;
}

int _bind(int fd, int port){
	struct sockaddr_in t = _addr(htonl(INADDR_ANY), port);
	return bind(fd,(struct sockaddr *)&t,sizeof(t));
}

int _connect(int fd, char* ip, int port){
	struct sockaddr_in t = _addr(inet_addr(ip), port);
	return connect(fd, (struct sockaddr *) &t, sizeof(t));
}

int _accept(int fd){
	struct sockaddr_in clntAddr; 
	unsigned int clntLen = sizeof(clntAddr);
	return accept(fd, (struct sockaddr *)&clntAddr, &clntLen);
}

int _send(int fd, char* msg){
	return send(fd, msg, (strlen(msg) + 1)*sizeof(char), 0);
}

int _recv(int fd, char* msg){
	return read(fd, msg, MSG_LEN*sizeof(char));
}

int _queue(){
	return msgget(IPC_PRIVATE, IPC_CREAT| 0666);
}

int _write(int q, note t){
	struct msgbuf m = {(long)(t.to), t};
	return msgsnd(q, &m, sizeof(m.t), 0);
}

int _read(int q, note* t, int to){
	struct msgbuf m;
	int n = msgrcv(q, &m, sizeof(m.t), (long)to, IPC_NOWAIT);
	if(n != ENOMSG)
		*t = m.t;
	return n;
}

void _lock(safe* a){
	pthread_mutex_lock(&(a->lock));
}

void _unlock(safe* a){
	pthread_mutex_unlock(&(a->lock));
}
