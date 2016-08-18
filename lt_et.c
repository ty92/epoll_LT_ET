/***
 * server program:
 *  gcc lt_et.c -o serv
 *  ./serv ip port
 *
 **/
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_SOCKET_NUMBERS 1024
#define BUF_SIZE 10

int setnonblocking(int fd);
void addfd(int epollfd, int fd, int flag);
void lt(struct epoll_event * events, int number, int epollfd, int listenfd);
void et(struct epoll_event * events, int number, int epollfd, int listenfd);

int main(int argc, char **argv)
{
	if(argc <= 2) {
		printf("please input ip address and port num\n");
		exit(EXIT_FAILURE);
	}

	const char *ip = argv[1];
	int port = atoi(argv[2]);

	int ret = 0;
	struct sockaddr_in address;
	bzero(&address,sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET,ip,&address.sin_addr);
	address.sin_port = htons(port);

	int listenfd = socket(AF_INET,SOCK_STREAM,0);
	assert(listenfd >= 0);

	ret = bind(listenfd,(struct sockaddr*)&address,sizeof(address));
	assert(ret != -1);

	ret = listen(listenfd,5);
	assert(ret != -1);

	struct epoll_event events[MAX_SOCKET_NUMBERS];
	int epollfd = epoll_create(5);
	assert( epollfd != -1);

	addfd(epollfd,listenfd,true);

	while(1) {
		int ret = epoll_wait(epollfd, events,MAX_SOCKET_NUMBERS,-1);   //阻塞，又返回则有事件需要处理

		if(ret < 0) {
			printf("epoll wait error\n");
			exit(EXIT_FAILURE);
		}

		//lt(events,ret,epollfd,listenfd);
		et(events,ret,epollfd,listenfd);
	}
	close(listenfd);

	return 0;
}

int setnonblocking(int fd){
	int old_option = fcntl(fd,F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd,F_SETFL,new_option);
	return new_option;
}

void addfd(int epollfd, int fd, int flag) {
	struct epoll_event event;
	memset(&event,0x00,sizeof(event));
	event.data.fd = fd;
	event.events = EPOLLIN;
	if(flag) {
		event.events |= EPOLLET;
	}
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
	setnonblocking(fd);   //将fd设置为非阻塞，因为epoll_wait()时已经阻塞，返回必定有就绪事件发生，此处就不需要继续阻塞
}

void lt(struct epoll_event *events, int number, int epollfd, int listenfd) {
	char buf[BUF_SIZE];
	int i;

	printf("number %d\n",number);

	for(i=0; i<number; i++) {
		int sockfd = events[i].data.fd;
		if(sockfd == listenfd) {   //监听套接字就绪，说明有client连接
			struct sockaddr_in client_address;
			socklen_t client_addresslen = sizeof(client_address);
			int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addresslen);
			if(connfd < 0) {
				printf("accept error\n");
				exit(EXIT_FAILURE);
			}
			addfd(epollfd,connfd,0);
		} else if(events[i].events & EPOLLIN) {  //连接描述符就绪
			printf("LT once\n");
			memset(buf,0x00,sizeof(buf));
			int ret = recv(sockfd,buf,sizeof(buf)-1,0);
			if(ret <= 0) {
				printf("recv 0 \n");
				close(sockfd);
				continue;
			}
			send(sockfd,buf,strlen(buf),0);
			printf("recv data form %d buf is %s\n",sockfd,buf);
		}else {
			printf("somthing else happen\n");
		}
	}
}

void et(struct epoll_event * events, int number, int epollfd, int listenfd) {
	char buf[BUF_SIZE];
	int i;

	for(i=0; i<number;i++) {
		int sockfd = events[i].data.fd;
		if(sockfd == listenfd) {
			struct sockaddr_in client_address;
			socklen_t client_addresslen = sizeof(client_address);
			int connfd = accept(listenfd,(struct sockaddr*)&client_address,&client_addresslen);
			addfd(epollfd,connfd,1);
		}else if(events[i].events & EPOLLIN) {
			printf("ET once \n");
		//	while(1) {
				memset(buf,0x00,sizeof(buf));
				int ret = recv(sockfd,buf,sizeof(buf)-1,0);
				if(ret < 0){
					if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
						printf("read later\n");
						break;
					}
					close(sockfd);
					break;
				}else if(ret == 0) {
					close(sockfd);
				}else {
					send(sockfd,buf,strlen(buf),0);
					printf("recv data from %d buf is %s\n",sockfd,buf);
				}
		//	}
		}else {
			printf("somthing else happen\n");
		}
	}
}
