/*
#include <arpa/inet.h> // for struct sockaddr_in
#include <string.h>   // 字节操作函数
#include <sys/socket.h>//socket操作
#include <netinet/in.h> // INET_ADDRSTRLEN
#include <unistd.h>   
#include <stdlib.h>    //atoi ascii to int
#include <stdio.h>		//输入输出处理
#include <errno.h>		//报错
#include <time.h>
#include <string.h>
#include <signal.h>


#define PORT 6789
#define BUFFER_SIZE 1024



void sig_chld(int sign);
ssize_t writen(int fd, void *vptr, size_t n);
ssize_t read(int fd, void *vptr, size_t n);
void str_echo(int connctfd);
int  Getlocaltime(char* buffer);
int main(int argc,char** argv) {
	//声明变量
	int listenfd, connctfd, listenq;
	pid_t pid;
	struct sockaddr_in serv, cli ;
	socklen_t clilen;

	signal(SIGCHLD, sig_chld);//自动清理子进程

	//判断参数
	if (argc != 2)
	{
		perror("usage: tcp server <connect num>");
		exit(-1);
	}

	listenq = atoi(argv[1]);

	//常规创建流程:socket-bind-listen-deal

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("SERVER ERROR:socket failed!");
		exit(-1);
	}

	//初始化
	bzero(&serv,sizeof(serv));
	serv.sin_family = PF_INET;
	serv.sin_port = htons(PORT);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (const struct sock_addr*)&serv, sizeof(serv)) < 0) {
		perror("SERVER ERROR:bind failed");
		exit(-1);
	}

	if (listen(listenfd, listenq) < 0) {
		perror("SERVER ERROR: listen failed");
		exit(-1);
	}

	for (;;) {
		clilen = sizeof(cli);
		if ((connctfd = accept(listenfd, (struct sock_addr *)&cli, &clilen)) < 0) {
			if (errno == EINTR)continue;
			else {
				perror("SERVER ERROR: accept failed");
				exit(-1);
			}
		};
		if ((pid = fork()) == 0)//子进程
		{
			printf("creating process %d success\n",getpid());
			close(listenfd);//减少引用		
			str_echo(connctfd);//处理事务
			printf("terminate process......\n");
			return 0;
		}else if(pid<0){
			perror("SERVER ERROR :process create failed!");
			exit(-1);
		}
		close(connctfd);//关闭
	}
	}



void str_echo(int connctfd) {
	char buffer_read[BUFFER_SIZE];
	char buffer_write[BUFFER_SIZE];
	size_t n;
again:
	bzero(&buffer_read, sizeof(buffer_read));
	bzero(&buffer_write, sizeof(buffer_write));
	printf("running...");
	while ((n = read(connctfd, buffer_read, BUFFER_SIZE)) > 0) {
		printf("Receive message:%s", buffer_read);
		if (!mycmp(buffer_read)) {
			perror("WARNING:unrecognized command!");
			if(sprintf(buffer_write,"WARNING:unrecognized command!")){
			
			};
			}else if (Getlocaltime(buffer_write) <= 0) {
				perror("ERROR:writen buffer error");
				exit(-1);
			}
			if (writen(connctfd, buffer_write, sizeof(buffer_write)) <= 0) {
				perror("ERROR:writen error!");
				exit(-1);
			}
		}
		if (n < 0 && errno == EINTR) {
			goto again;
		}
		else {
			perror("SERVER ERROR: read failed");
			exit(-1);
		}
	}
	return;
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
			}
		}
		nleft -= nwriten;
		ptr += nwriten;
	}
	return n;
}
	
void sig_chld(int sign) {
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("child %d terminated.\n", pid);
		return;
	}

}

int mycmp(const char* s1) {
const char* s = "show me the time";
int i = 0;
while (i <= 15) {
if (s[i] != s1[i]) {
break;
}
i++;
}
return i == 16;
}


ssize_t readn(int fd, void *vptr, size_t n) {
	size_t nleft = n;
	size_t read_ = 0;
	void *ptr = vptr;
	while (nleft>0) {
		if ((read_ = read(fd, ptr, nleft)) <= 0) {
			if (read_ < 0 && errno == EINTR) {
				continue;
			} else {
				perror("SERVER ERROR: read failed!");
				exit(-1);
			}
		}
		nleft -= read_;
		ptr += read_;
	}
	return n;
}


int  Getlocaltime(char* buffer) {
	time_t timep;
	struct tm*p;
	time(&timep);
	p = localtime(&timep);
	return sprintf(buffer, "%d/%d/%d %d:%d:%d", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
}
*/
