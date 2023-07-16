#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#define PORT 7890
#define BUFFER_SIZE 1024

static void *doit(void *fd);
int mycmp(const char* s1);
void str_echo(int connctfd);
ssize_t writen(int fd, void *vptr, size_t n);
ssize_t readn(int fd, void *vptr, size_t n);
int  Getlocaltime(char* buffer);




int main(int argc, char**argv) {
	int listenfd, connfd,listenq;
	pthread_t tid;
	struct sockaddr_in server, client;
	socklen_t clilen;

	if (argc != 2) {
		perror("usage: tcp server <connect num>");
		exit(-1);
	}

	listenq = atoi(argv[1]);

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("ERROR:socket failed!");
		exit(-1);
	}

	bzero(&server,sizeof(server));
	server.sin_family = PF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd,(const struct sockaddr*)&server,sizeof(server))<0){
		perror("ERROR:bind failed!");
		exit(-1);
	}

	if ((listen(listenfd,listenq )) < 0) {
		perror("ERROR:listen failed!");
		exit(-1);
	}

	clilen = sizeof(client);
	for (;;) {
		printf("okkk");
		if ((connfd = accept(listenfd, (struct sockaddr*)&client, &clilen)) < 0) {
			perror("ERROR:accept failed");
			exit(-1);
		}
		printf("okkkkkk");
		if(pthread_create(&tid, NULL, &doit, (void*)&connfd) != 0) {
			perror("ERROR:pthread created failed!");
			exit(-1);
		}
	
	}
	close(listenfd);
	return 0;
}

static void *doit(void *fd) {
	int connfd = *((int*)fd);
	pthread_detach(pthread_self());
	str_echo(connfd);
	printf("Thread %ld created!", pthread_self());
	close(connfd);
	printf("Thread %ld terminated!", pthread_self());
	pthread_exit(NULL);
}


void str_echo(int connctfd) {
	char buffer_read[BUFFER_SIZE];
	char buffer_write[BUFFER_SIZE];
	size_t n;
again:
	printf("Thread %ld reading...\n", pthread_self());
	bzero(&buffer_read, sizeof(buffer_read));
	bzero(&buffer_write, sizeof(buffer_write));
	while ((n = read(connctfd, buffer_read, BUFFER_SIZE)) > 0) {
		printf("pid %ld running...\n", pthread_self());
		printf("Receive message:%s\n", buffer_read);
		if (!mycmp(buffer_read)) {
			perror("WARNING:unrecognized command!");;
			if (!(sprintf(buffer_write, "WARNING:unrecognized command!"))) {
				perror("ERROR:write failed!");
				exit(-1);
			}
		}
		else if (Getlocaltime(buffer_write) <= 0) {
			perror("ERROR:writen buffer error");
			exit(-1);
		}
		if (writen(connctfd, buffer_write, sizeof(buffer_write)) <= 0) {
			perror("ERROR:writen error!");
			exit(-1);
		}
		if (n < 0) {
			break;
		}
	}
	if (n < 0 && errno == EINTR) {
		goto again;
	}
	else if (n<0) {
		perror("SERVER ERROR: read failed");
		exit(-1);
	}

	return;
}



ssize_t writen(int fd, void *vptr, size_t n) {//Ð´²Ù×÷
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
			}
			else {
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

