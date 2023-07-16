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
#include <sys/select.h>
#include "header4.h"
#include <math.h>




/*
	基础架构模式：
		1、客户端通过和服务器交互完成基本功能
		2、服务器在有客户端登陆即下线的时候，通知所有客户端该消息
		3、hash表的方法存储客户端信息
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
	/*select进行读写*/
	if ((listenfd = get_sockfd())<0) {
		perror("获取sockfd失败\n");
		exit(-1);
	}
	
	
	maxfd = 10;
	maxi = -1;
	FD_ZERO(&allset);//描述符清零
	FD_SET(listenfd, &allset);//把sockfd置1
	init();
	for (i = 0; i < FD_SETSIZE; i++) {
		client[i] = -1;//初始化
	}

	printf("服务器初始化成功,即将运行\n");
	for (;;) {
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
		if (nready < 0) {
			perror("select返回值错误");
			exit(-1);
		}
		if (FD_ISSET(listenfd, &rset))//检查I/O操作
		{
			printf("有新的客户端请求连接\n");
			clilen = sizeof(cliaddr);
			if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen))<0){
				perror("接受连接错误\n");
				close(connfd);
				exit(-1);
			}
			for (i = 0; i < FD_SETSIZE; i++) {
				if (client[i] < 0) {
					client[i] = connfd;//保存描述符
					break;
				}
			}
			if (i == FD_SETSIZE) {
				perror("连接的客户端过多\n");
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
				printf("处理传进来的I/O操作\n");
				memset(rbuffer, 0, sizeof(rbuffer));
				memset(readbuffer, 0, sizeof(info));
				printf("清空\n");
				if ((n = read(sockfd, rbuffer, sizeof(rbuffer))) <= 0) {
					printf("读入出现意外\n");
					if (n < 0) {
						perror("读取客户端请求失败\n");
						exit(-1);
					}
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} else {
					printf("处理数据\n");
					memset(readbuffer,0, sizeof(info));
					memcpy(readbuffer, rbuffer, sizeof(info));
				}
				if (handle_client_req(sockfd) < 0) {
					perror("服务器处理客户端信息错误\n");
					exit(-1);
				}
				if (--nready <= 0)//处理完所有请求
				{
					if (check_time() < 0) {
						perror("客户端监听失败\n");
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
	printf("收到来自客户端的信息 type:%d\n", readbuffer->type);
	switch (readbuffer->type) {
	case 1:
	{

	newn = (nodeptr)malloc(sizeof(node));
		if (!newn) {
			perror("内存分配失败\n");
		}
		newn->next = NULL;
		printf("client %s 申请注册信息\n", readbuffer->name);
		strcpy(newn->name, readbuffer->name);
		newn->sockfd = sockfd;
		memset(wbuffer, 0, sizeof(wbuffer));
		newn->t = time(NULL);
		printf("处理注册事件\n");
		if ( (insert(newn))<0 ) {
			writebuffer->type = -1;
			strcpy(writebuffer->msg, "注册信息失败");
			memcpy(wbuffer, writebuffer, sizeof(info));
			if (writen(sockfd, wbuffer, sizeof(wbuffer)) < 0) {
				perror("发送信息失败\n");
				exit(-1);
			}
		} else {
			printf("注册信息成功\n");
			writebuffer->type = 1;
			memset(wbuffer, 0, sizeof(wbuffer));
			memcpy(wbuffer, writebuffer, sizeof(wbuffer));
			if (writen(sockfd, wbuffer, sizeof(wbuffer)) < 0) {
				perror("发送信息失败\n");
				exit(-1);
			}

			printf("广播注册信息给各客户端\n");
			memset(wbuffer, 0, sizeof(wbuffer));
			writebuffer->type = 0;
			memset(writebuffer->msg, 0, sizeof(writebuffer->msg));
			sprintf(writebuffer->msg, "客户端%s注册成功", readbuffer->name);
			memcpy(wbuffer, writebuffer, sizeof(info));
			if ((broadcast()) < 0) {
				perror("服务器广播消息失败\n");
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
			printf("获取客户端昵称：%s\n", name);
			if ((n = find(name))) {
				printf("找到客户端注册信息\n");
				sprintf(writebuffer->msg, " %s send to %s",readbuffer->name,readbuffer->msg);
				writebuffer->type = 2;
				memset(wbuffer, 0, sizeof(wbuffer));
				memcpy(wbuffer, writebuffer, sizeof(info));
				if (writen(n->sockfd, wbuffer, sizeof(wbuffer)) < 0) {
					perror("发送信息失败\n");
					exit(-1);
				}
			}else {
				memset(wbuffer, 0, sizeof(wbuffer));
				writebuffer->type = -2;
				strcpy(writebuffer->msg, "该客户端未找到\n");
				memcpy(wbuffer, writebuffer, sizeof(info));
				if (writen(sockfd, wbuffer, sizeof(wbuffer)) < 0) {
					perror("发送信息失败\n");
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
			perror("服务器广播信息失败\n");
			exit(-1);
		}

	}
		break;
	case 4:
	{
		if (check(readbuffer->name) < 0) {
			perror("heart beat 失误\n");
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
		perror("socket创建失败\n");
		exit(-1);
	}

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	//马上释放端口号

	int listenq = 10;
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (const struct sockaddr*)&server, sizeof(server))<0) {
		perror("bind失败\n");
		exit(-1);
	}

	if ((listen(sockfd, listenq)) < 0) {
		perror("listen失败\n");
		exit(-1);
	}

	return sockfd;
}



/*初始化基本数据*/
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


/*插入结点*/
int insert(nodeptr n) {
	printf("插入结点\n");
	int index = hash(n->name);
	while (clients[index]) {
		if (mystrcmp(clients[index]->name, n->name) == 0)//不能用相同的客户端名
		{
			printf("客户端同名\n");
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
	printf("插入结束\n");
	return 0;
}

void show() {
	nodeptr n = head->next;
	while (n) {
		printf("已注册客户端%s\n", n->name);
		n = n->next;
	}
	
}

/*删除结点*/
void delete_(const char* name) {
	nodeptr now = head->next;
	nodeptr pre = head;
	while (now) {
		if (mystrcmp(now->name, name) == 0) {
			pre->next = now->next;
			free(now);
			now = NULL;
			num -= 1;
			printf("该节点已被删除\n");
		}
		else {
			pre = now;
			now = now->next;
		}
	}
}



/*找到信息，返回地址信息*/
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
	/*根据客户端名称进行hash*/
	/*暂时没有查重机制，但可以增加，如果需要的话*/
	int l = sizeof(name);
	int index=0;
	int i = 0;
	while (i < l) {
		index += name[i] - '0';
		i++;
	}
	printf("计算索引值为%d\n", index);
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
				perror("读操作错误\n");
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
		perror("未查找该客户端\n");
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
			perror("广播信息错误\n");
			exit(-1);
		}
		printf("已经广播给%d个客户端\n", i++);
		nd = nd->next;
	}
	printf("广播结束\n");
	return 0;
}


int check_time() {
	nodeptr n = head->next;
	t = time(NULL);
	
	while (n) {
		if ((t - (n->t)) > 10) {
			memset(writebuffer->msg, 0, sizeof(writebuffer->msg));
			memset(wbuffer, 0, sizeof(wbuffer));
			sprintf(writebuffer->msg,"客户端%s掉线\n", n->name);
			delete_(n->name);
			writebuffer->type = 0;
			memcpy(wbuffer, writebuffer, sizeof(info));
			if (broadcast() < 0) {
				perror("广播信息错误\n");
			}
		}
		n = n->next;
	}
	return 0;
}