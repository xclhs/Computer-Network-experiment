#include <arpa/inet.h> // for struct sockaddr_in
#include <sys/socket.h>//socket操作
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h> // INET_ADDRSTRLEN
#include <unistd.h>   
#include <stdlib.h>    //atoi ascii to int
#include <stdio.h>		//输入输出处理
#include <errno.h>		//报错
#include <time.h>
#include <string.h>


#define PORT 6789
#define LISTENQ 10
#define BUFFERSIZE 1024


ssize_t writen(int fd, void *vptr, size_t n);
static int create_server_proc();
//创建服务，监听服务请求create_server
static void handle_client_pro(int sockfd, char*buffer, int len);

int main(int argc, char** argv) {
	printf("********************* SELECT SERVICES ***************************\n");
	int i, maxi, maxfd, listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE];
	ssize_t n;
	fd_set rset, allset;//读集合，所有描述符集合
	char buf[BUFFERSIZE];
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;

	if ((listenfd = create_server_proc()) < 0) {
		perror("ERROR:create listenfd error!");
		exit(-1);
	};
	//初始化select
	maxfd = listenfd;
	maxi = -1;
	FD_ZERO(&allset);//描述符清零
	FD_SET(listenfd, &allset);//把listen返回的描述符置1
	struct timeval mt;
	for (i = 0; i < FD_SETSIZE; i++) {
		client[i] = -1;
	}

	printf("NOTE:create success,wait for ask...\n");

	for (;;) {
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);//n个ready
		if (nready < 0) {
			perror("ERROR:select error!");
			exit(-1);
		}
		if (FD_ISSET(listenfd, &rset))//客户端连接,检查I/O操作是否ready
		{  //ready则接受连接
			printf("%d socket ready!\n", nready);
			clilen = sizeof(cliaddr);
			if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) < 0) {
				perror("ERROR:accept failed");
				close(connfd);
				exit(-1);
			}
			//将FD放入客户端
			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd;//保存连接描述符
					break;
				}
			if (i == FD_SETSIZE) {
				perror("ERROR:too many clients");
				exit(-1);
			}
			FD_SET(connfd, &allset);//增加一个新的df

			if (connfd > maxfd) {
				maxfd = connfd;  //供select使用
			}

			if (i > maxi) {
				maxi = i;  //client[]最大的index
			}


			if (--nready <= 0) {
				continue;     //没有可读的df
			}
		}
		for (i = 0; i <= maxi; i++) {
			printf("select:deal with the clients request\n");
			if ((sockfd = client[i]) < 0) {
				continue;
			}
			if (FD_ISSET(sockfd, &rset)) {
				if ((n = read(sockfd, buf, BUFFERSIZE)) <= 0) {
					if (n < 0) {
						perror("ERROR:read failed\n");
						exit(-1);
					}
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;//存储有效的connection fd
				}else handle_client_pro(sockfd,buf,sizeof(buf));
				if (--nready <= 0) {//处理完
					break;
				}
			}
		}
	}
}

static void handle_client_pro(int sockfd, char*buffer, int len) {
	buffer[len - 1] = '\0';
	printf("Received client message:%s\n", buffer);
	if (writen(sockfd, buffer, len) < 0) {
		perror("ERROR:write failed!");
		exit(-1);
	}
}

static int create_server_proc() {
	int fd;
	struct sockaddr_in servaddr;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("ERROR:create socket failed");
		exit(-1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	//马上释放端口号

	if ((bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) )< 0) {
		perror("ERROR:bind failed");
		exit(-1);
	}
	if (listen(fd, LISTENQ) < 0) {
		perror("ERROR:listen failed");
		exit(-1);
	}
	return fd;
}

ssize_t writen(int fd, void *vptr, size_t n) {//写操作
	size_t nleft = n;
	size_t nwriten = 0;
	void *ptr = vptr;
	while (nleft>0) {
		if ((nwriten = write(fd, ptr, nleft)) <= 0) {
			if (nwriten < 0 && errno == EINTR) {
				nwriten = 0;
				continue;
			}
			else {
				perror("SERVER ERROR: write failed!");
				exit(-1);
			}
		}
		nleft -= nwriten;
		ptr += nwriten;
	}
	return n;
}