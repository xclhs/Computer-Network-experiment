/*
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define PORT 6789


void str_client(FILE *fp,int connfd);
ssize_t writen(int fd, void *vptr, size_t n);
int readline(int fd, void *vptr, int max);

int main(int argc, char** argv) {
int sockfd;
struct sockaddr_in servaddr;

if (argc != 2) {
perror("usage:tcpcli <IPaddress> ");
exit(-1);
}
if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
perror("socket error");
exit(-1);
}

bzero(&servaddr,sizeof(servaddr));
servaddr.sin_family = PF_INET;
servaddr.sin_port = htons(PORT);

if (inet_pton(PF_INET, argv[1], &servaddr.sin_addr) < 0) {
perror("Inet_pton failed!");
exit(-1);
}

if (connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
perror("connect failed!");
exit(-1);
}

str_client(stdin, sockfd);
exit(0);
}

void str_client(FILE *fp, int connfd) {
char send[BUFFER_SIZE], recv[BUFFER_SIZE];
while (fgets(send, BUFFER_SIZE, fp) != NULL) {
if (writen(connfd, send, sizeof(send)) < 0) {
perror("write failed");
exit(-1);
}
if (readline(connfd, recv, BUFFER_SIZE) < 0) {
perror("read failed!");
exit(-1);
}
printf("Receive from server:");
fputs(recv, stdout);
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
*/