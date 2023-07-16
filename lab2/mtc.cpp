#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <signal.h>


#define BUFFER_SIZE 1024
#define PORT 7890

void sig_chld(int sign);
int readline(int fd, void *vptr, int max);
void str_client(FILE *fp, int connfd);
ssize_t writen(int fd, void *vptr, size_t n);

int main(int argc, char** argv) {
	struct sockaddr_in server;
	int sockfd,time=atoi(argv[2]);
	socklen_t len;
	pid_t pid;

	signal(SIGCHLD, &sig_chld);


	if (argc != 3) {
		perror("usage:tcpli <IPaddress> <Threadnum>");
		exit(-1);
	}

	if (inet_pton(PF_INET, argv[1], &server.sin_addr) < 0) {
		perror("Inet_pton failed!");
		exit(-1);
	}

	int i = 0;
	for (; i < time; i++) {
		pid = fork();
		if (pid < 0) {
			perror("ERROR:PID create error");
			exit(-1);
		}
		else if (pid == 0) {//子进程
			if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				perror("ERROR:socket error");
				exit(-1);
			}

			bzero(&server, sizeof(server));
			server.sin_port = htons(PORT);
			server.sin_family = PF_INET;


			if (connect(sockfd, (const struct sockaddr*)&server, sizeof(server)) < 0) {
				perror("ERROR:connect error!");
				exit(-1);
			}
			printf("creating process %d success\n", getpid());
			str_client(stdin,sockfd);//处理事务
			printf("terminate process......\n");
			close(sockfd);
			return 0;
		}
		else {

		}
	}

}

void sig_chld(int sign) {
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("child %d terminated.\n", pid);
		return;
	}

}

void str_client(FILE *fp, int connfd) {
	char send[BUFFER_SIZE], recv[BUFFER_SIZE];
	printf("child %d reading from stdin...", getpid());
	while(fgets(send, BUFFER_SIZE, fp) != NULL) {

		if (writen(connfd, send, sizeof(send)) < 0) {
			perror("ERROR:write failed");
			exit(-1);
		}
		if (readline(connfd, recv, BUFFER_SIZE) < 0) {
			perror("ERROR:read failed!");
			exit(-1);
		}
		printf("Process %d Receive from server:", getpid());
		fputs(recv, stdout);
		printf("\n");
	}
}

ssize_t writen(int fd, void *vptr, size_t n) {
	size_t nleft = n;
	size_t nwriten = 0;
	void *ptr = vptr;
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

int readline(int fd, void *vptr, int max) {
	size_t nread = 0;
	size_t nleft = max;
	char *ptr = vptr;
	while (nleft > 0)
	{
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (nread < 0 && errno == EINTR) {
				nread = 0;
			}
			else {
				perror("read failed!");
				exit(-1);
			}
		}
		else if (nread == 0)break;
		nleft -= nread;
		ptr += nread;

	}
	return (max - nleft);
}