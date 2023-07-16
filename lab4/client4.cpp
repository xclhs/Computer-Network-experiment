#include <arpa/inet.h> // for struct sockaddr_in
#include <sys/socket.h>//socket����
#include <sys/types.h>
#include <netinet/in.h> // INET_ADDRSTRLEN
#include <unistd.h>   
#include <stdlib.h>    //atoi ascii to int
#include <stdio.h>		//�����������
#include <errno.h>		//����
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

	//��
	if (argc != 2) {
		perror("Usage:tcpcli <clientname>\n");
		exit(-1);
	}

	//��
	cw.client2serverw = (info*)malloc(sizeof(info));
	cr.client2serverr = (info*)malloc(sizeof(info));
	strcpy(cw.client2serverw->name, argv[1]);

	s.sends = f.flag = 0;
	//��
	if ((sockfd = bindTCP()) < 0) {
		perror("socket��ȡʧ��\n");
		exit(-1);
	}

	printf("��ȡsockfd\n");

	//��
	if (pthread_create(&tid1, NULL, &writeit, (void*)&sockfd) != 0) {
		perror("ERROR:pthread created failed!");
		exit(-1);
	}

	//��
	if (pthread_create(&tid2, NULL, &readit, (void*)&sockfd) != 0) {
		perror("ERROR:pthread created failed!");
		exit(-1);
	}

	printf("�߳����гɹ�\n");
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
	//�����ͷŶ˿ں�

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
			perror("��Ϣ��ȡʧ��\n");
			exit(-1);
		}	
		memcpy(cr.client2serverr, buffer, sizeof(info));
		switch (cr.client2serverr->type) {
		case -1:
		{
			perror("������ע��ʧ��\n");
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
			printf("������ע��ɹ���\n");
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


/*��Ҫ���������û����������Լ�������Ϣ��*/
void handler(const char* name) {
	/********����console  command *******/
	char command, cname[40], msg[1024];
	printf("-----------------command�������---------------------\n");
	for (;;) {
		printf("\n��������������1��ע��	��2��ͨ�š�3���뿪��");
		scanf(" %c", &command);
		switch (command)
		{
		case '1'://ע��
		{
			pthread_mutex_lock(&f.mutex);
			if (f.flag) {
				printf("���ͻ������ڷ�������ע��\n");
			}
			else {
				pthread_mutex_lock(&s.mutex);
				pthread_mutex_lock(&cw.mutex);
				bzero(cw.client2serverw, sizeof(info));
				strcpy(cw.client2serverw->name, name);
				cw.client2serverw->type = 1;
				s.sends = 1;
				pthread_mutex_unlock(&s.mutex);
				printf("��ɻ�����Ϣ����\n");
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
				printf("******************�����뷢�Ϳͻ��˺���Ϣ[name:msg]*******************\n");
				cw.client2serverw->type = 2;
				printf("��ʽ��name:msg\n");
				scanf("%c", &command);
				if (fgets(cw.client2serverw->msg, maxline, stdin) != NULL) {
					printf("��������:%s",cw.client2serverw->msg);
				}
				s.sends = 1;

			}	else {
				printf("���棺�ÿͻ���δע��\n");
			}
			pthread_mutex_unlock(&cw.mutex);
			pthread_mutex_unlock(&s.mutex);
			pthread_mutex_unlock(&f.mutex);
			printf("���ݶ�ȡ��ϣ�������\n");
		}
		break;
		case '3':
		{

			printf(" ��ȷ���Ƿ��˳�����y/n) \n :");
			scanf(" %c", &command);
			switch (command)
			{
			case 'y':
			case 'Y':
			{
				pthread_mutex_lock(&cw.mutex);
				printf("***�ÿͻ�������׼���ر�***\n");
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
			printf("�޷�ʶ�������\n");
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
				perror("������Ϣ����\n");
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
			perror("д�����\n");
			exit(-1);
		}
		nleft -= nwriten;
		ptr += nwriten;

	}
	return max - nleft;
}