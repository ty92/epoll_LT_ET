#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
void str_cli(FILE *fp, int sockfd)
{
        char sendline[BUFSIZ],recvline[BUFSIZ];
		fd_set reset;
		int maxfd;

        while(1) {
			FD_ZERO(&reset);
			FD_SET(fileno(fp),&reset);
			FD_SET(sockfd,&reset);
			maxfd = fileno(fp) > sockfd ? fileno(fp) : sockfd;

			select(maxfd+1,&reset,NULL,NULL,NULL);
			if(FD_ISSET(fileno(fp),&reset)) {
				if(fgets(sendline,BUFSIZ,fp) != NULL) 
		            write(sockfd,sendline,strlen(sendline));
			}
			if(FD_ISSET(sockfd,&reset)) {
				memset(recvline,0x00,sizeof(recvline));
				if(read(sockfd,recvline,BUFSIZ) == 0)
					printf("readline:%m\n"),exit(1);
				fputs(recvline,stdout);
			}
		}
}
int main(int argc, char **argv)
{
        int sockfd;
        struct sockaddr_in servaddr;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(12000);
        inet_aton("192.168.234.129",&servaddr.sin_addr);
        connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
        str_cli(stdin,sockfd);
        exit(0);
}
