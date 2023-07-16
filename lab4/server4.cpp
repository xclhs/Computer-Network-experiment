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
#include <sys/select.h>
#include "header4.h"
#include <math.h>




/*
	�����ܹ�ģʽ��
		1���ͻ���ͨ���ͷ�����������ɻ�������
		2�����������пͻ��˵�½�����ߵ�ʱ��֪ͨ���пͻ��˸���Ϣ
		3��hash��ķ����洢�ͻ�����Ϣ
*/




void show();
int insert(nodeptr n);
int check(const char* name);
int mystrcmp(const char* s1, const char* s2);
int hash(const char *name);
nodeptr find(const char* name);
static void init();
static int get_sockfd();
void writeit(int sockfd);
void readit(int sockfd);
int insert(nodeptr n);
void delete_(const char* name);
int  handle_client_req(int sockfd);
int check_time();
int  broadcast();
int mysubstring(char *des, const char* name);
ssize_t readline(int fd, void* vptr, size_t max);
ssize_t writen(int fd, void* vptr, size_t max);




nodeptr head;
nodeptr newn;
nodeptr clients[maxconnect];
int num=0; 
info *readbuffer, *writebuffer;
char* rbuffer[2048];
char* wbuffer[2048];
time_t t;






int main(int argc,char**argv) {
	int sockfd,connfd, i, maxi, maxfd, nready, client[FD_SETSIZE],listenfd;
	ssize_t n;
	fd_set rset, allset;
	socklen_t clilen;
	struct sockaddr_in cliaddr;
	/*select���ж�д*/
	if ((listenfd = get_sockfd())<0) {
		perror("��ȡsockfdʧ��\n");
		exit(-1);
	}
	
	
	maxfd = 10;
	maxi = -1;
	FD_ZERO(&allset);//����������
	FD_SET(listenfd, &allset);//��sockfd��1
	init();
	for (i = 0; i < FD_SETSIZE; i++) {
		client[i] = -1;//��ʼ��
	}

	printf("��������ʼ���ɹ�,��������\n");
	for (;;) {
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
		if (nready < 0) {
			perror("select����ֵ����");
			exit(-1);
		}
		if (FD_ISSET(listenfd, &rset))//���I/O����
		{
			printf("���µĿͻ�����������\n");
			clilen = sizeof(cliaddr);
			if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen))<0){
				perror("�������Ӵ���\n");
				close(connfd);
				exit(-1);
			}
			for (i = 0; i < FD_SETSIZE; i++) {
				if (client[i] < 0) {
					client[i] = connfd;//����������
					break;
				}
			}
			if (i == FD_SETSIZE) {
				perror("���ӵĿͻ��˹���\n");
				exit(-1);
			}

			FD_SET(connfd, &allset);

			if (connfd > maxfd) {
				maxfd = connfd;
			}

			if (i > maxi) {
				maxi = i;
			}

			if (--nready <= 0) {
				continue;
			}
		}
		for (i = 0; i <= maxi; i++) {
			if ((sockfd = client[i]) < 0) {
				continue;
			}
			if (FD_ISSET(sockfd, &rset)) {
				printf("����������I/O����\n");
				memset(rbuffer, 0, sizeof(rbuffer));
				memset(readbuffer, 0, sizeof(info));
				printf("���\n");
				if ((n = read(sockfd, rbuffer, sizeof(rbuffer))) <= 0) {
					printf("�����������\n");
					if (n < 0) {
						perror("��ȡ�ͻ�������ʧ��\n");
						exit(-1);
					}
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} else {
					printf("��������\n");
					memset(readbuffer,0, sizeof(info));
					memcpy(readbuffer, rbuffer, sizeof(info));
				}
				if (handle_client_req(sockfd) < 0) {
					perror("����������ͻ�����Ϣ����\n");
					exit(-1);
				}
				if (--nready <= 0)//��������������
				{
					if (check_time() < 0) {
						perror("�ͻ��˼���ʧ��\n");
						exit(-1);
					}
					break;
				}
			}
		}
	}



	return 0;
}


 int  handle_client_req(int sockfd) {
	printf("�յ����Կͻ��˵���Ϣ type:%d\n", readbuffer->type);
	switch (readbuffer->type) {
	case 1:
	{

	newn = (nodeptr)malloc(sizeof(node));
		if (!newn) {
			perror("�ڴ����ʧ��\n");
		}
		newn->next = NULL;
		printf("client %s ����ע����Ϣ\n", readbuffer->name);
		strcpy(newn->name, readbuffer->name);
		newn->sockfd = sockfd;
		memset(wbuffer, 0, sizeof(wbuffer));
		newn->t = time(NULL);
		printf("����ע���¼�\n");
		if ( (insert(newn))<0 ) {
			writebuffer->type = -1;
			strcpy(writebuffer->msg, "ע����Ϣʧ��");
			memcpy(wbuffer, writebuffer, sizeof(info));
			if (writen(sockfd, wbuffer, sizeof(wbuffer)) < 0) {
				perror("������Ϣʧ��\n");
				exit(-1);
			}
		} else {
			printf("ע����Ϣ�ɹ�\n");
			writebuffer->type = 1;
			memset(wbuffer, 0, sizeof(wbuffer));
			memcpy(wbuffer, writebuffer, sizeof(wbuffer));
			if (writen(sockfd, wbuffer, sizeof(wbuffer)) < 0) {
				perror("������Ϣʧ��\n");
				exit(-1);
			}

			printf("�㲥ע����Ϣ�����ͻ���\n");
			memset(wbuffer, 0, sizeof(wbuffer));
			writebuffer->type = 0;
			memset(writebuffer->msg, 0, sizeof(writebuffer->msg));
			sprintf(writebuffer->msg, "�ͻ���%sע��ɹ�", readbuffer->name);
			memcpy(wbuffer, writebuffer, sizeof(info));
			if ((broadcast()) < 0) {
				perror("�������㲥��Ϣʧ��\n");
				exit(-1);
			}
		}
	}
		break;
	case 2:
	{
		char name[40];
		nodeptr n;
		if (mysubstring(name,readbuffer->msg)==0) {
			printf("��ȡ�ͻ����ǳƣ�%s\n", name);
			if ((n = find(name))) {
				printf("�ҵ��ͻ���ע����Ϣ\n");
				sprintf(writebuffer->msg, " %s send to %s",readbuffer->name,readbuffer->msg);
				writebuffer->type = 2;
				memset(wbuffer, 0, sizeof(wbuffer));
				memcpy(wbuffer, writebuffer, sizeof(info));
				if (writen(n->sockfd, wbuffer, sizeof(wbuffer)) < 0) {
					perror("������Ϣʧ��\n");
					exit(-1);
				}
			}else {
				memset(wbuffer, 0, sizeof(wbuffer));
				writebuffer->type = -2;
				strcpy(writebuffer->msg, "�ÿͻ���δ�ҵ�\n");
				memcpy(wbuffer, writebuffer, sizeof(info));
				if (writen(sockfd, wbuffer, sizeof(wbuffer)) < 0) {
					perror("������Ϣʧ��\n");
					exit(-1);
				}
			}
		}


	}
		break;
	case 3:
	{
		delete_(readbuffer->name);
		writebuffer->type = 2;
		memset(writebuffer->msg, 0, sizeof(writebuffer->msg));
		sprintf(writebuffer->msg, "client %s leaving\n", readbuffer->name);
		memcpy(wbuffer, writebuffer, sizeof(info));
		if (broadcast() < 0) {
			perror("�������㲥��Ϣʧ��\n");
			exit(-1);
		}

	}
		break;
	case 4:
	{
		if (check(readbuffer->name) < 0) {
			perror("heart beat ʧ��\n");
			exit(-1);
		}
		nodeptr n;
		if ((n=find(readbuffer->name))) {
			printf("%ld", n->t);
		}

	}
		break;
	default:
		break;
	}



}




static int get_sockfd() {
	int sockfd;
	struct sockaddr_in server;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket����ʧ��\n");
		exit(-1);
	}

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	//�����ͷŶ˿ں�

	int listenq = 10;
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (const struct sockaddr*)&server, sizeof(server))<0) {
		perror("bindʧ��\n");
		exit(-1);
	}

	if ((listen(sockfd, listenq)) < 0) {
		perror("listenʧ��\n");
		exit(-1);
	}

	return sockfd;
}



/*��ʼ����������*/
static void init() {
	head = (nodeptr)malloc(sizeof(node));
	readbuffer = (info*)malloc(sizeof(info));
	writebuffer = (info*)malloc(sizeof(info));
	strcpy(writebuffer->name, "server");
	head->next = NULL;
	int i = 0;
	for (; i < maxconnect; i++) {
		clients[i] = NULL;
	}
}


/*������*/
int insert(nodeptr n) {
	printf("������\n");
	int index = hash(n->name);
	while (clients[index]) {
		if (mystrcmp(clients[index]->name, n->name) == 0)//��������ͬ�Ŀͻ�����
		{
			printf("�ͻ���ͬ��\n");
			return -1;
		}
		index += 1;
		if (index == 100) {
			index = 0;
		}
	}
	clients[index] = n;
	
	n->next = head->next;
	head->next = n;

	show();
	num += 1;
	printf("�������\n");
	return 0;
}

void show() {
	nodeptr n = head->next;
	while (n) {
		printf("��ע��ͻ���%s\n", n->name);
		n = n->next;
	}
	
}

/*ɾ�����*/
void delete_(const char* name) {
	nodeptr now = head->next;
	nodeptr pre = head;
	while (now) {
		if (mystrcmp(now->name, name) == 0) {
			pre->next = now->next;
			free(now);
			now = NULL;
			num -= 1;
			printf("�ýڵ��ѱ�ɾ��\n");
		}
		else {
			pre = now;
			now = now->next;
		}
	}
}



/*�ҵ���Ϣ�����ص�ַ��Ϣ*/
nodeptr find(const char* name) {
	nodeptr  now = head->next;
	while (now) {
		if (mystrcmp(now->name, name)==0) {
			return now;
		}
		now = now->next;
	}
	return NULL;
}


int mystrcmp(const char* s1,const char* s2){
	int l1 = sizeof(s1);
	int l2 = sizeof(s2);
	if (l2 > l1) {
		return -1;
	}
	int i=0;
	while (s1[i] == s2[i]&&i<l2) {
		i++;
	}
	if (i == l2) {
		return 0;
	}
	return s1[i] - s2[i];
}


int hash(const char *name){
	/*���ݿͻ������ƽ���hash*/
	/*��ʱû�в��ػ��ƣ����������ӣ������Ҫ�Ļ�*/
	int l = sizeof(name);
	int index=0;
	int i = 0;
	while (i < l) {
		index += name[i] - '0';
		i++;
	}
	printf("��������ֵΪ%d\n", index);
	return (abs(index) % 100);
}

ssize_t writen(int fd, void* vptr, size_t max) {
	size_t nwriten=0;
	size_t nleft=max;
	void *ptr = vptr;
	while (nleft) {
		if ((nwriten = write(fd, ptr, nleft)) < 0) {
			if (errno == EINTR) {
				continue;
			}
			else {
				perror("����������\n");
				exit(-1);
			}
		}
		nleft -= nwriten;
		ptr += nwriten;
	}
	return max - nleft;
}


ssize_t readline(int fd, void* vptr, size_t max) {
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


int check(const char* name) {
	nodeptr n;
	if (!(n=find(name))) {
		perror("δ���Ҹÿͻ���\n");
		exit(-1);
	}	else {
		n->t = time(NULL);
	}
}



int mysubstring(char *des,const char* name) {
	bzero(des, sizeof(des));
	int length = sizeof(name);
	int i = 0;
	int f = 0;
	while (i<length) {
		if (name[i] != ':') {
			des[i] = name[i];
		} else {
			f = 1;
			break;
		}
		i++;
	}
	if (f==1) {		
		return 0;
	}	else {
		return -1;
	}
}

int  broadcast() {
	nodeptr nd = head->next;
	int i = 1;
	while (nd) {
		printf("%s\n", nd->name);
		if (writen(nd->sockfd, wbuffer, sizeof(wbuffer)) < 0) {
			perror("�㲥��Ϣ����\n");
			exit(-1);
		}
		printf("�Ѿ��㲥��%d���ͻ���\n", i++);
		nd = nd->next;
	}
	printf("�㲥����\n");
	return 0;
}


int check_time() {
	nodeptr n = head->next;
	t = time(NULL);
	
	while (n) {
		if ((t - (n->t)) > 10) {
			memset(writebuffer->msg, 0, sizeof(writebuffer->msg));
			memset(wbuffer, 0, sizeof(wbuffer));
			sprintf(writebuffer->msg,"�ͻ���%s����\n", n->name);
			delete_(n->name);
			writebuffer->type = 0;
			memcpy(wbuffer, writebuffer, sizeof(info));
			if (broadcast() < 0) {
				perror("�㲥��Ϣ����\n");
			}
		}
		n = n->next;
	}
	return 0;
}