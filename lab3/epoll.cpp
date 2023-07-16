#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> // for struct sockaddr_in
#include <string.h>   // 字节操作函数 
#include <stdlib.h>    //atoi ascii to int
#include <stdio.h>		//输入输出处理
#include <limits.h>



#define MAX_SIZE 1024
#define LISTENQ 5
#define MAXPOLL 128
#define PORT 6789
#define INFIM -1

ssize_t writen(int fd, void *vptr, size_t n);

int main1(int argc, char** argv) {
	printf("****************EPOLL SERVICES*****************\n");
	int i, nfds, listenfd, connfd, sockfd, epollfd,max;
	ssize_t n;
	socklen_t clilen;
	char buf[MAX_SIZE];
	struct epoll_event ev, events[20];
	struct sockaddr_in cliaddr, servaddr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("ERROR:create socket failed!");
		exit(-1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);


	if ((epollfd = epoll_create(MAXPOLL)) < 0)//返回epoll实例关联的文件描述符
	{
		perror("ERROR:epoll_createl() failed!\n");
		exit(-1);
	}
	ev.data.fd = listenfd;
	ev.events = EPOLLIN | EPOLLET;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) < 0) {//注册文件描述符
		perror("ERROR:epoll_ctl() failed!\n");
		exit(-1);
	}

	if ((bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0) {
		perror("ERROR:bind failed!");
		exit(-1);
	}

	if (listen(listenfd, LISTENQ) < 0) {
		perror("ERROR:listen failed!");
		exit(-1);
	}

	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//立即释放端口号

	printf("*****************NOTE:Initialization successful***************\n");

	max = 0;
	for (;;) {
		printf("NOTE:wait for ready...\n");
		if ((nfds = epoll_wait(epollfd, events, max+1, -1)) < 0) {
			perror("ERROR:epoll_wait failed!\n");
			exit(-1);
		}
		printf("NOTE:nfds is %d\n", nfds);

		for (i = 0; i < nfds; ++i) {
			if (events[i].data.fd == listenfd) {
				clilen = sizeof(cliaddr);
				if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) < 0) {
					perror("ERROR:accept failed!");
					exit(-1);
				}
				if (max >= MAXPOLL) {
					perror("ERROR:too many connection!\n");
					exit(-1);
				}

				printf("NOTE:accept a new connection\n");
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = connfd;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0)
				{
					perror("ERROR:epoll set insertion error");
					exit(-1);
				}
				max += 1;
			} else {
				int n;
				sockfd = events[i].data.fd;
				if (events[i].events&EPOLLIN) {
					bzero(buf, MAX_SIZE);
					if ((n = read(sockfd, buf, MAX_SIZE)) < 0) {
						perror("ERROR:read failed!");
						close(sockfd);
						eixt(-1);
					} else {
						printf("NOTE:receive from client:%s.\n",buf);
						if (writen(sockfd, buf, n) < 0) {
							perror("ERROR:write failed!");
							exit(-1);
						}
						printf("NOTE:write successful\n");
					}
				}
		


			}
		}
	}
}



ssize_t writen(int fd, void *vptr, size_t n) {
	void *ptr = vptr;
	size_t nwriten = 0;
	size_t nleft = n;
	while (nleft > 0) {
		if ((nwriten = write(fd, ptr, nleft)) <= 0) {
			if (nwriten < 0 && errno == EINTR) {
				nwriten = 0;
				continue;
			}else{
				perror("ERROR:write failed!");
				exit(-1);
			}
		}
		nleft -= nwriten;
		ptr += nwriten;
	}
	return n;
}