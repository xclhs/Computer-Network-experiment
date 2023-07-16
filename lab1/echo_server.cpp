/*

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>//for struct sockaddr_in

#define BUFFER_SIZE 1024
#define LENGTH_OF_QUEUE 5


void str_echo(int listenfd);
ssize_t writen(int fd, void *vptr, size_t n);

int main(int argc, char** argv) {
	int listenfd, connfd ,port;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	if (argc != 2) {
		perror("usage:tcpcli  <Port>");
		exit(-1);
	}

	port = atoi(argv[1]);

	if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)<0)) {
		perror("socket error");
		exit(-1);
	}

	

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = PF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = INADDR_ANY;
	int bindfd = bind(listenfd, (const struct sockaddr*)&servaddr, sizeof(servaddr));
	if (bindfd<0) {
		perror("server bind failed");
		exit(-1);
	}
	if ((listen(listenfd, LENGTH_OF_QUEUE))<0) {
		perror("server listen failed");
	}

	for (;;) {
		clilen = sizeof(cliaddr);
		if ((connfd = accept(listenfd, ( struct sock_addr*)(&cliaddr), &clilen
		))<0) {
			if (errno == EINTR)continue;
			else {
				perror("server accept failed");
				exit(-1);
			}
		}
		char buffer[BUFFER_SIZE];
		if ((childpid = fork()) == 0) {
			close(listenfd);
			str_echo(connfd);
			exit(0);
		}
		close(connfd);
	}
	waitpid(childpid, NULL, 0);
}


void str_echo(int connfd) {
	size_t n;
	char buffer[BUFFER_SIZE];

again:
	while ((n = read(connfd, buffer, BUFFER_SIZE)) > 0) {
		if (writen(connfd, buffer, n) < 0) {
			perror("write listenfd failed!");
			exit(-1);
		}
	}
	if (n < 0 && errno == EINTR) {
		goto again;
	}
	else {
		perror("read listenfd failed!");
		exit(-1);
	}
	exit(0);
}

ssize_t writen(int fd, void *vptr, size_t n) {
	size_t nleft=n;
	size_t nwriten=0;
	void *ptr=vptr;
	while (nleft > 0) {
		if ((nwriten = write(fd, ptr, nleft)) <= 0) {
			if (nwriten < 0 || errno == EINTR) {
				nwriten = 0;
			}
			else {
				perror("server writen failed!");
				return -1;
			}
		}
		nleft -= nwriten;
		ptr += nwriten;
	}
	return n;
}

*/