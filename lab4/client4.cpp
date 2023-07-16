#include <arpa/inet.h> // for struct sockaddr_in
#include <sys/socket.h>//socket操作
#include <sys/types.h>
#include <netinet/in.h> // INET_ADDRSTRLEN
#include <unistd.h>   
#include <stdlib.h>    //atoi ascii to int
#include <stdio.h>		//输入输出处理
#include <errno.h>		//报错
#include <time.h>
#include <string.h>
#include "header4.h"


typedef struct {
	int flag;
	pthread_mutex_t mutex;
}flag;

typedef struct {
	int sends;
	pthread_mutex_t mutex;
}sends;

typedef struct {
	info*client2serverw;
	pthread_mutex_t mutex;
}c2sw;

typedef struct {
	info*client2serverr;
	pthread_mutex_t mutex;
}c2sr;



flag f;
sends s;
c2sr cr;
c2sw cw;
char buffer[1024*2];
char wbuffer[1024 * 2];




static int bindTCP();
static void *readit(void *fd);
static void *writeit(void *fd);
void handler(const char* name);
int heartBeat(int sockfd);
void runread(int sockfd);
int readline(int fd, void *vptr, int max);



int main(int argc, char** argv) {
	pthread_t tid1, tid2;
	int sockfd;

	//①
	if (argc != 2) {
		perror("Usage:tcpcli <clientname>\n");
		exit(-1);
	}

	//②
	cw.client2serverw = (info*)malloc(sizeof(info));
	cr.client2serverr = (info*)malloc(sizeof(info));
	strcpy(cw.client2serverw->name, argv[1]);

	s.sends = f.flag = 0;
	//③
	if ((sockfd = bindTCP()) < 0) {
		perror("socket获取失败\n");
		exit(-1);
	}

	printf("获取sockfd\n");

	//④
	if (pthread_create(&tid1, NULL, &writeit, (void*)&sockfd) != 0) {
		perror("ERROR:pthread created failed!");
		exit(-1);
	}

	//⑤
	if (pthread_create(&tid2, NULL, &readit, (void*)&sockfd) != 0) {
		perror("ERROR:pthread created failed!");
		exit(-1);
	}

	printf("线程运行成功\n");
	handler(argv[1]);
	/*
	if (pthread_create(&tid3, NULL, &handlerit, (void*)argv[1],(void*)&sockfd) != 0) {
	perror("ERROR:pthread created failed!");
	exit(-1);
	}
	*/



}

static int bindTCP() {
	struct sockaddr_in servaddr;
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed!\n");
		exit(-1);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	if (inet_pton(PF_INET, IP, &servaddr.sin_addr) < 0) {
		perror("IP parsing error\n");
		exit(-1);
	}

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	//马上释放端口号

	if (connect(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		perror("connet error\n");
		exit(-1);
	}

	return sockfd;
}

static void *readit(void *fd) {
	int sockfd = *((int*)fd);
//	pthread_detach(pthread_self());
	printf("read Thread %ld created!\n", pthread_self());
	runread(sockfd);
	close(sockfd);
	printf("read Thread %ld terminated!\n", pthread_self());
	pthread_exit(NULL);
}

void runread(int sockfd) {
	pthread_mutex_lock(&cr.mutex);
	char buffer[2048];
	for (;;) {
		memset(buffer, 0, sizeof(buffer));
		memset(cr.client2serverr, 0, sizeof(info));
		int readn;
		if ((readn=read(sockfd, buffer, sizeof(buffer)))<=0) {
			if (readn == 0) {
				continue;
			}
			perror("信息读取失败\n");
			exit(-1);
		}	
		memcpy(cr.client2serverr, buffer, sizeof(info));
		switch (cr.client2serverr->type) {
		case -1:
		{
			perror("服务器注册失败\n");
		}
		break;
		case 0:
		{
			fputs(cr.client2serverr->msg, stdout);
		}
		break;
		case 1:
		{
			pthread_mutex_lock(&f.mutex);
			printf("服务器注册成功！\n");
			f.flag = 1;
			pthread_mutex_unlock(&f.mutex);
		}
		break;
		case 2: {
			printf("recv:\n");
			fputs(cr.client2serverr->msg, stdout);
			printf("\n");
		}
		}
	}
	pthread_mutex_unlock(&cr.mutex);
}

/*static void *handlerit(void* name,void* fd) {
int sockfd = *((int*)fd);
char *str =(char*) name;
printf("handler Thread %ld created!", pthread_self());
pthread_detach(pthread_self());
handler(str);
close(sockfd);
printf("handler Thread %ld terminated!", pthread_self());
pthread_exit(NULL);
}
*/



static void *writeit(void *fd) {
	int sockfd = *((int*)fd);
//	pthread_detach(pthread_self());
	printf("write Thread %ld created!\n", pthread_self());
	heartBeat(sockfd);
	close(sockfd);
	printf("write Thread %ld terminated!", pthread_self());
	pthread_exit(NULL);
}

int heartBeat(int sockfd) {
	pthread_mutex_lock(&cw.mutex);
	cw.client2serverw->type = 4;
	pthread_mutex_unlock(&cw.mutex);
	for (;;) {
		memset(wbuffer, 0, sizeof(wbuffer));
		pthread_mutex_lock(&s.mutex);
		pthread_mutex_lock(&cw.mutex);
		pthread_mutex_lock(&f.mutex);
		if (s.sends) {
			memcpy(wbuffer, cw.client2serverw, sizeof(info));
			if (writen(sockfd, wbuffer, sizeof(wbuffer)) < 0) {
				perror("ERROR:tcp writen failed\n");
				exit(-1);
			}
			memset(cw.client2serverw->msg, 0, maxline);
			s.sends = 0;
			cw.client2serverw->type = 4;
			printf("send to server sucessful\n");
		}
		if (f.flag) {
			memcpy(wbuffer, cw.client2serverw, sizeof(info));
			if (write(sockfd, wbuffer, sizeof(wbuffer)) < 0) {
				perror("ERROR:heart beat writen failed\n");
				exit(-1);
			}
		}
		pthread_mutex_unlock(&f.mutex);
		pthread_mutex_unlock(&cw.mutex);
		pthread_mutex_unlock(&s.mutex);	
		sleep(1);
	}
}


/*主要用来处理用户输入的命令，以及交互信息等*/
void handler(const char* name) {
	/********处理console  command *******/
	char command, cname[40], msg[1024];
	printf("-----------------command处理界面---------------------\n");
	for (;;) {
		printf("\n请输入你的命令：【1】注册	【2】通信【3】离开：");
		scanf(" %c", &command);
		switch (command)
		{
		case '1'://注册
		{
			pthread_mutex_lock(&f.mutex);
			if (f.flag) {
				printf("本客户端已在服务器端注册\n");
			}
			else {
				pthread_mutex_lock(&s.mutex);
				pthread_mutex_lock(&cw.mutex);
				bzero(cw.client2serverw, sizeof(info));
				strcpy(cw.client2serverw->name, name);
				cw.client2serverw->type = 1;
				s.sends = 1;
				pthread_mutex_unlock(&s.mutex);
				printf("完成基本信息输入\n");
			}
			pthread_mutex_unlock(&f.mutex);
			pthread_mutex_unlock(&cw.mutex);
		}
		break;
		case '2':
		{
			pthread_mutex_lock(&f.mutex);
			pthread_mutex_lock(&s.mutex);
			pthread_mutex_lock(&cw.mutex);
			if (f.flag) {
				printf("******************请输入发送客户端和信息[name:msg]*******************\n");
				cw.client2serverw->type = 2;
				printf("格式：name:msg\n");
				scanf("%c", &command);
				if (fgets(cw.client2serverw->msg, maxline, stdin) != NULL) {
					printf("传出数据:%s",cw.client2serverw->msg);
				}
				s.sends = 1;

			}	else {
				printf("警告：该客户端未注册\n");
			}
			pthread_mutex_unlock(&cw.mutex);
			pthread_mutex_unlock(&s.mutex);
			pthread_mutex_unlock(&f.mutex);
			printf("数据读取完毕，传送中\n");
		}
		break;
		case '3':
		{

			printf(" 请确认是否退出？（y/n) \n :");
			scanf(" %c", &command);
			switch (command)
			{
			case 'y':
			case 'Y':
			{
				pthread_mutex_lock(&cw.mutex);
				printf("***该客户端正在准备关闭***\n");
				cw.client2serverw->type = 3;
				pthread_mutex_unlock(&cw.mutex);
				pthread_mutex_lock(&s.mutex);
				int sends = 1;	
				pthread_mutex_unlock(&s.mutex);
				while (1) {
					sleep(2);
					pthread_mutex_lock(&s.mutex);
					if (!s.sends) {
						break;
					}
					pthread_mutex_unlock(&s.mutex);
					sleep(2);
				}
				exit(0);
			}

			case 'n':
				break;
			case 'N':
				break;
			default:
				break;
			}
		}
		break;
		default:
			printf("无法识别的命令\n");
			break;
		}
		int sends = 1;
	}
}

int readline(int fd, void *vptr, int max) {
	size_t nread;
	size_t nleft = max;
	char *ptr = vptr;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR) {
				continue;
			}
			else {
				perror("读入信息错误\n");
				exit(-1);
			}
		}
		nleft -= nread;
		ptr += nread;
	}
	return (max - nleft);

}

int writen(int fd, void *vptr, int max) {
	size_t nwriten;
	size_t nleft = max;
	char* ptr = vptr;

	while (nleft > 0) {
		if ((nwriten = write(fd, ptr, nleft)) < 0) {
			perror("写入错误\n");
			exit(-1);
		}
		nleft -= nwriten;
		ptr += nwriten;

	}
	return max - nleft;
}