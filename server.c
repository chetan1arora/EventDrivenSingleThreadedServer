#include "util.h"

#define LISTENQ 15
#define MAXLINE 80
#define FILELEN 4096
#define MAXSIZE 4096
#define ATONCE 512

void die(char* msg){
	perror(msg);
	exit(0);
}

fd_set rset, allsetr, wset, allsetw;
int myfavclient;

struct clientstate
{
	int fd;
	int state;			//0 connection accepted, 1-request being read, 2-processing, 3-replying back
	char buf[MAXLINE];
	int in_index;
	int out_index;
	int is_cgi;
	int f_size;
};

typedef struct clientstate* cState;
void processClient (cState cs);
void terminate(int sockfd, cState client,int i, fd_set* allsetr);
void fname(char* buf, char* name);
int iscgi(char* name);
void loadFile(int p,char* msg, int size);
int ismapped(char* buf, void** mem, int size);
off_t _fsize(char* name);
int done_write(struct clientstate cs);

int displayMincore(void *addr,int k){
	int p = getpagesize();
	//printf("%d\n", p);
	int l = (k-1 + p)/p;
	unsigned char vec[l];
	if(mincore(addr, k, vec) == -1){
		perror("mincore");
	}
	printf("%d\n",l);
	for(int i = 0; i< l ; i++){
		printf("%d %d\n", i, vec[i]);
		if((1&vec[i])==0)
			return -1;
	}
	return 0;
}



void processClient (cState cs)
{
	char filename[MAXLINE];
	fname(cs->buf, filename);
	cs->is_cgi = iscgi(filename);
	if(cs->is_cgi){
	}else{
		printf("not cgi\n");
		cs->f_size = _fsize(cs->buf);
		//int k = 8000;
		int k = cs->f_size;
		int fd = open(cs->buf,O_RDWR);
		void* addr1 = mmap(NULL, k, PROT_READ, MAP_SHARED, fd, 0);	
		// if file not in memory
		int g=displayMincore(addr1,k);		
		printf("g is %d\n",g);
		if(g){	
			printf("Mapping into memory.\n");
			loadFile(fork(),cs->buf, cs->f_size);
			//sleep(2);
		//	printf("wake up..\n"); 
		}
		if(displayMincore(addr1,k))
			printf("ismapped: 1\n");
		//sleep(15); // unnecessary
		}
	FD_SET (cs->fd, &allsetw);
	cs->state = 3;
}

off_t _fsize(char* name){
	struct stat buf;
	int fd = open(name, O_RDONLY);
	int n = fstat(fd, &buf);
	return buf.st_size;
}
/*
int ismapped(char* buf, void** mem, int size){
	FILE* f = fopen(buf, "r");
	char *vec = (char *)0;
	int pageSize = getpagesize();
	
	vec = calloc(1, (size+pageSize-1)/pageSize);
	int k = mincore(0, size, vec);
	fclose(f);
	if(k == -1) die("mincore f");	
	printf("ismapped: %d\n", k);
	return 0;	
}


void findaddr(int shid, void** addr){
	shmat(shid, *addr, 0);
}

int _sharedMem(int size){
	return shmget(IPC_PRIVATE, size , IPC_CREAT);
}
*/

void loadFile(int p, char* msg, int size){
	if(p == -1)die("fork() f");
	else if(p != 0)return;
	int t = getpagesize();
	int fd = open(msg,O_RDWR);
	int l = (size-1 + t)/t;
	char buf[l];
	printf("start mmap\n");
	void* addr1 = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	for(int i= 0; i < l; i++){
		*(char*)(buf+i) =*(char*)(addr1+t*i);			
	}
	close(fd);
	
	//findaddr(shid, &addr);
		
	//if((map = mmap(addr, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)die("mmap");
	//printf("mmap success\n");
	exit(0);
}

int main (int argc, char **argv) 
{
	if(argc == 1){
		printf("Usage: %s <port>", argv[0]);
		exit(0);
	}
	
	//int shmem = _sharedMem(MAXSIZE);
	//create_child(); // blocking reading
	//return fd
	//FD_SET(&allset,fd);

	int i, maxi, maxfd, listenfd, connfd, sockfd;
	int toProcess = 0;
	int nready;

	struct clientstate client[FD_SETSIZE];
	ssize_t n;	

	char buf[MAXLINE];
	socklen_t clilen;

	struct sockaddr_in cliaddr, servaddr;

	listenfd = _tcp();
	if(listenfd == -1)die("_tcp() f");
	if(_bind(listenfd, atoi(argv[1])) == -1)die("_bind() f");

	listen(listenfd, LISTENQ);
	maxfd = listenfd;		
	maxi = -1;

	for (i = 0; i < FD_SETSIZE; i++){
		client[i].fd = -1;
		client[i].state = -1;
		client[i].in_index = 0;
		client[i].out_index = 0;
		memset(client[i].buf, 0, sizeof(client[i].buf));
		client[i].is_cgi = -1;
	}

	FD_ZERO (&allsetr);
	FD_ZERO (&allsetw);
	FD_SET (listenfd, &allsetr);

	for (;;)
	{
		printf("waiting...\n");
		rset = allsetr;
		wset = allsetw;
		nready = select (maxfd + 1, &rset, &wset, NULL, NULL);

		if (FD_ISSET (listenfd, &rset)){			/* new client connection */
			clilen = sizeof (cliaddr);
			printf("*********************************************************\n");
			printf("accepting...\n");
			connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
			int fl=fcntl(connfd, F_GETFL, 0);
			fl=fl|O_NONBLOCK;
			fcntl(connfd, F_SETFL, &fl);
			// printf ("new client: %s, port %d\n",
			// inet_ntop (AF_INET, &cliaddr.sin_addr, 4, NULL),
			// ntohs (cliaddr.sin_port));
			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i].fd < 0){
					client[i].fd = connfd;	/* save descriptor */
					client[i].state = 0;
					break;
				}	

			if (i == FD_SETSIZE){
				printf ("too many clients");
				exit (0);
			}

			FD_SET (connfd, &allsetr);	/* add new descriptor to set */

			if (connfd > maxfd)
				maxfd = connfd;	/* for select */
			if (i > maxi)
				maxi = i;		/* max index in client[] array */

			if (--nready <= 0)
				continue;		/* no more readable descriptors */
		}

		printf("ready clients\n");
		for (i = 0; i <= maxi; i++)
		{
			/* check all clients for data */
			if ((sockfd = client[i].fd) < 0)
				continue;
			// printf ("%d %d %d %d %s\n", sockfd, client[i].state,
			// 	client[i].in_index, client[i].out_index, client[i].buf);
			printf("------------------------------------------------------\n");
			printf("sockfd: %d\n", sockfd);
			printf("state: %d\n", client[i].state);
			printf("readable: %d\n", FD_ISSET (sockfd, &rset) != 0);
			printf("writable: %d\n", FD_ISSET (sockfd, &wset) != 0);

			if (client[i].state == 3 && FD_ISSET (sockfd, &wset)){
				printf("Writing\n");
				int fd = open(client[i].buf,O_RDWR);
				void* mem = mmap(NULL, client[i].f_size, PROT_READ, MAP_SHARED, fd, 0);
				if(!displayMincore(mem,client[i].f_size)){
					if(mem != NULL){
						printf("got mapped memory\n");
					}
					// write from mem
					n = write (sockfd, mem + client[i].out_index,ATONCE);
					if(n == -1)die("write() f");
					printf("wrote %zd bytes\n", n);
					client[i].out_index = client[i].out_index + n;
					if(done_write(client[i])){
						// done writing
						client[i].state = 0;
						client[i].out_index = 0;
						FD_CLR (sockfd, &allsetw);
						FD_SET (sockfd, &allsetr);
					}
				}
			}

			if ((client[i].state == 0 || client[i].state == 1) && FD_ISSET (sockfd, &rset)){
				printf("reading...\n");
				if ((n =read (sockfd, client[i].buf + client[i].in_index, MAXLINE)) == 0)
				{
	    			/*connection closed by client */ 
	    			printf("read 0 bytes\n");
	    			printf("closing...\n");
	    			printf("*********************************************************\n");
	    			terminate(sockfd, client, i, &allsetr);
					
				}
				else
				{
					client[i].state = 1;
					client[i].in_index = client[i].in_index + n;
					myfavclient = sockfd;
					
					// if (client[i].in_index >= EXPECTED_LEN)
					// {
						client[i].state = 2;
						FD_CLR (sockfd, &allsetr);
						toProcess++;
					// }
		    		//write (sockfd, buf, n);
				}
				strcpy(client[i].buf,strtok(client[i].buf,"\r"));
				printf("read %zd bytes...%s  (%lu)\n", n, client[i].buf, strlen(client[i].buf));

				if (--nready <= 0){
					printf("------------------------------------------------------\n"); 
					break;		/* no more readable descriptors */
				}
			}
			printf("------------------------------------------------------\n"); 
		}
		printf("myfavclient(%d) is set to read? %d \n", myfavclient, FD_ISSET(sockfd, &allsetr));


		if (toProcess > 0){
			for (i = 0; i <= maxi; i++)
	  		{			
	  			printf("processing...\n");
	  			/* check all clients for data */
				if ((sockfd = client[i].fd) < 0)
					continue;

				if (client[i].state == 2)
				{
					processClient (&client[i]);
					toProcess--;
				}
			}
		}
		printf("stop now.....!");
		sleep(2);
	}
}

void terminate(int sockfd, cState client,int i, fd_set* allsetr){
	close (sockfd);
	FD_CLR (sockfd, allsetr);
	client[i].fd = -1;
	client[i].state = -1;
	client[i].in_index=0;
	client[i].out_index=0;
	memset(client[i].buf, 0, sizeof(client[i].buf));
}

int findlast(char* name, char d){
	int l = strlen(name);
	int i;
	for(i = l - 1; i >= 0; i--){
		if(name[i] == d){
			return i;
		}
	}
	return -1;
}

int findfirst(char* name, char d){
	int l = strlen(name);
	int i;
	for(i = 0; i < l; i++){
		if(name[i] == d){
			return i;
		}
	}
	return l;
}

void fname(char* buf, char* name){
	int start = findlast(buf, '/');
	int end = findfirst(buf, '?');
	strncpy(name, buf + start + 1, end - start);
	name[end - start + 1] = '\0';
}


int iscgi(char* name){
	int l = strlen(name);
	return strcmp(".cgi", name + l - 4) == 0;
}


int done_write(struct clientstate cs){
	return cs.f_size <= cs.out_index;
}
