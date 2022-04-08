#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
 
#define MAXLNE  4096

#define POLL_SIZE	1024

void *client_routine(void *arg) { //client_routine函数是多线程的回调函数，用来处理每一个连接的请求，也就是接收与发送信息功能

	int connfd = *(int *)arg;

	char buff[MAXLNE];

	while (1) {

		int n = recv(connfd, buff, MAXLNE, 0);
        if (n > 0) {
            buff[n] = '\0';
            printf("recv msg from client: %s\n", buff);

	    	send(connfd, buff, n, 0);
        } else if (n == 0) {
            close(connfd);
			break;
        }

	}

	return NULL;
}


int main(int argc, char **argv) 
{
    int listenfd, connfd, n;
    struct sockaddr_in servaddr;
    char buff[MAXLNE];
 
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {//socket创建listen的文件描述符
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }
 
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(10000);//端口信息可以自己设置
 //bind函数绑定IP，端口信息
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }
 
    if (listen(listenfd, 10) == -1) {//listen监听函数，参数10表示未完成连接与已完成连接队列之和，一般设置为5的倍数
        printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }
	while (1) {
		struct sockaddr_in client;
	    socklen_t len = sizeof(client);
	    if ((connfd = accept(listenfd, (struct sockaddr *)&client, &len)) == -1) {
	        printf("accept socket error: %s(errno: %d)\n", strerror(errno), errno);
	        return 0;
	    }
		pthread_t threadid;
		pthread_create(&threadid, NULL, client_routine, (void*)&connfd);//创建线程，并执行client_routine回调函数功能，将connfd作为回调函数的参数传入，client_routine函数将处理相应文件描述符connfd的发送和接收功能
    }
    return 0;
}