#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/poll.h>
#include <pthread.h>
#define MAXLNE  4096

#define POLL_SIZE 1024
int main() {

    int listenfd, connfd, n;

    struct sockaddr_in servaddr;
    char buff[MAXLNE];
 
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }
 
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(10000);
 
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }
 
    if (listen(listenfd, 10) == -1) {
        printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }

 

    struct pollfd fds[POLL_SIZE] = {0};
    fds[0].fd = listenfd;//将listenfd存入fd[0]处
    fds[0].events = POLLIN;

    int max_fd = listenfd;

    
    while(1) {

        int nready = poll(fds, max_fd + 1, -1); //nready是返回的监控文件描述符中有响应的描述符个数
        if(fds[0].revents & POLLIN) {//检查listenfd是否在rset这个集合中，存在则listenfd监控到客户端发来的连接请求
            
            struct sockaddr_in client;
            socklen_t len = sizeof(client);
            if((connfd = accept(listenfd, (struct sockaddr *)&client, &len)) == -1) {
                printf("accept socket error: %s(errno: %d)\n", strerror(errno), errno);
                return 0;
            }
            printf("accept Success fd=  %d\n", connfd);

            fds[connfd].fd = connfd;
            fds[connfd].events = POLLIN;
            if(connfd > max_fd) max_fd = connfd;  //重置监控的fd范围
            if(--nready == 0) continue; //如果只有一个（说明没有其他的可读事件），则继续while（1）循环监控
        }
        int i = 0;
        for(i = listenfd + 1; i <= max_fd; i++) {//处理其余已与客户端建立连接的事件
            if(fds[i].revents & POLLIN) {
                //printf("message arrive  fd=  %d\n", i);
                n = recv(i, buff, MAXLNE, 0);
                if (n > 0) {
                    buff[n] = '\0';
                    printf("recv msg from client: %s\n", buff);
                    
                    send(i, buff, n, 0);//将发送来的数据依次返回发送回去

                   
                } else if (n == 0) {//当客户端调用close时，会返回n=0

                    fds[i].fd = -1;
                    close(i);
                }
                if(--nready == 0) break;
            } 
        }
    }
    return 0;
}

