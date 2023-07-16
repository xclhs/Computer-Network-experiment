#include <arpa/inet.h> // for struct sockaddr_in
#include <sys/socket.h>//socket����
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h> // INET_ADDRSTRLEN
#include <unistd.h>   
#include <stdlib.h>    //atoi ascii to int
#include <stdio.h>		//�����������
#include <errno.h>		//����
#include <time.h>
#include <string.h>


#define PORT 6789
#define LISTENQ 10
#define BUFFERSIZE 1024


ssize_t writen(int fd, void *vptr, size_t n);
static int create_server_proc();
//�������񣬼�����������create_server
static void handle_client_pro(int sockfd, char*buffer, int len);

int main(int argc, char** argv) {
	printf("********************* SELECT SERVICES ***************************\n");
	int i, maxi, maxfd, listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE];
	ssize_t n;
	fd_set rset, allset;//�����ϣ���������������
	char buf[BUFFERSIZE];
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;

	if ((listenfd = create_server_proc()) < 0) {
		perror("ERROR:create listenfd error!");
		exit(-1);
	};
	//��ʼ��select
	maxfd = listenfd;
	maxi = -1;
	FD_ZERO(&allset);//����������
	FD_SET(listenfd, &allset);//��listen���ص���������1
	struct timeval mt;
	for (i = 0; i < FD_SETSIZE; i++) {
		client[i] = -1;
	}

	printf("NOTE:create success,wait for ask...\n");

	for (;;) {
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);//n��ready
		if (nready < 0) {
			perror("ERROR:select error!");
			exit(-1);
		}
		if (FD_ISSET(listenfd, &rset))//�ͻ�������,���I/O�����Ƿ�ready
		{  //ready���������
			printf("%d socket ready!\n", nready);
			clilen = sizeof(cliaddr);
			if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) < 0) {
				perror("ERROR:accept failed");
				close(connfd);
				exit(-1);
			}
			//��FD����ͻ���
			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd;//��������������
					break;
				}
			if (i == FD_SETSIZE) {
				perror("ERROR:too many clients");
				exit(-1);
			}
			FD_SET(connfd, &allset);//����һ���µ�df

			if (connfd > maxfd) {
				maxfd = connfd;  //��selectʹ��
			}

			if (i > maxi) {
				maxi = i;  //client[]����index
			}


			if (--nready <= 0) {
				continue;     //û�пɶ���df
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
					client[i] = -1;//�洢��Ч��connection fd
				}else handle_client_pro(sockfd,buf,sizeof(buf));
				if (--nready <= 0) {//������
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
	//�����ͷŶ˿ں�

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

ssize_t writen(int fd, void *vptr, size_t n) {//д����
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