#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <errno.h>

#define EXIT_SUCC 0
#define EXIT_FAIL 1

char quit[] = "exit";

void error(char *msg);
int openSocket(char *hostIP, int port);
int say(int socket, char *msg);
int readIn(int socket, char *buf, int len);
void* readThread(void *d_sock);
void* sendThread(void *d_sock);

int main(){
	int d_sock;
	pthread_t r_thread;
	pthread_t w_thread;
	void* result;

	d_sock = openSocket("127.0.0.1", 3000);

	//create & excute thread
	if(pthread_create(&r_thread, NULL, readThread, (void*)d_sock) == -1){
		error("Fail to creat read thread");
		exit(EXIT_FAIL);
	}
	if(pthread_create(&w_thread, NULL, sendThread, (void*)d_sock) == -1){
		pthread_join(r_thread, &result);
		error("Fail to create send thread");
		exit(EXIT_FAIL);
	}

	//wait thread to end
	pthread_join(r_thread, &result);
	pthread_join(w_thread, &result);

	return EXIT_SUCC;
}

void error(char *msg){
	fprintf(stderr, "%s : %s\n", msg, strerror(errno));
	exit(EXIT_FAIL);
}

int openSocket(char *hostIp, int port){
	int d_sock = socket(PF_INET, SOCK_STREAM, 0);
	int c;
	struct sockaddr_in name;

	memset(&name, 0, sizeof(name));

	name.sin_family = AF_INET;
	name.sin_addr.s_addr = inet_addr(hostIp);
	name.sin_port = htons(port);

	if(d_sock == -1){
		error("Fail to open socket");
		exit(EXIT_FAIL);
	}
	
	c = connect(d_sock, (struct sockaddr*)&name, sizeof(name));

	if(c == -1){
		error("Fail to connect socket");
		exit(EXIT_FAIL);
	}

	return d_sock;
}


int say(int socket, char *msg){
	write(socket, msg, strlen(msg));
	return EXIT_SUCC;
}

int readIn(int socket, char *buf, int len){
	int str_len = 0;
	while((str_len = read(socket, buf, len)) != 0){
		buf[str_len]='\0';
		printf("receive : %s", buf);
	}
	return EXIT_SUCC;
}

void* readThread(void *d_sock){
	int socket = (int)d_sock;
	char buf[256];
	while(socket != -1){
		if(readIn(socket, buf, 256))
			break;
	}
}
void* sendThread(void *d_sock){
	int socket = (int) d_sock;
	char buf[256];
	while(socket != -1){
		if(fgets(buf, sizeof(buf), stdin) != "s")
			say(socket, buf);
		if(strncmp(buf, quit, 4) == 0){
			break;
		}
	}
}




