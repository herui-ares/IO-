#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/select.h>
#include <pthread.h>
#define MAXLNE  4096
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




    fd_set rfds, wfds, rset, wset;  //大小可以更改，一般对应1024个文件描述符
    FD_ZERO(&rfds); //文件描述符清0
    FD_ZERO(&wfds); //文件描述符清0
    FD_SET(listenfd, &rfds); // 将listenfd在rfds处置1

    int max_fd = listenfd;

    while(1) {
        rset = rfds;  
        wset = wfds;  
        //select 是阻塞的，且函数有5个参数
        //第一个参数是一个整数值， 表示集合中所有文件描述符的范围，即所有文件描述符的最大值+1。系统会从0开始监测到你设置的这个参数的描述符范围（0~ maxfd+1）
        //第二个参数是监视文件描述符的一个集合，我们监视其中的文件描述符是不是可读，或者更准确的说，读取是不是不阻塞了。发生可读事件，该rset集合上对应文件描述符的位置将会置1
        int nready = select(max_fd + 1, &rset, &wset, NULL, NULL); //nready是返回的监控文件描述符中有响应的描述符个数
        if(FD_ISSET(listenfd, &rset)) {//检查listenfd是否在rset这个集合中，存在则listenfd监控到客户端发来的连接请求
            
            struct sockaddr_in client;
            socklen_t len = sizeof(client);
            if((connfd = accept(listenfd, (struct sockaddr *)&client, &len)) == -1) {
                printf("accept socket error: %s(errno: %d)\n", strerror(errno), errno);
                return 0;
            }
            printf("accept Success fd=  %d\n", connfd);
            FD_SET(connfd, &rfds);  //将与客户端连接的socket加入select的监控集合中
            
            if(connfd > max_fd) max_fd = connfd;  //重置监控的fd范围
            if(--nready == 0) continue; //如果只有一个（说明没有其他的可读事件），则继续while（1）循环监控
        }
        
        int i = 0;
        for(i = listenfd + 1; i <= max_fd; i++) {//处理其余已与客户端建立连接的事件
            if(FD_ISSET(i, &rset)) {
                printf("message arrive  fd=  %d\n", i);
                n = recv(i, buff, MAXLNE, 0);
                if (n > 0) {
                    buff[n] = '\0';
                    printf("recv msg from client: %s\n", buff);

                //send(i, buff, n, 0);//将发送来的数据依次返回发送回去

                    FD_SET(i, &wfds);  //把相应的写入监控描述符置1，下一次监控时，会检查到可写事件
                } else if (n == 0) {//当客户端调用close时，会返回n=0
                    FD_CLR(i, &rfds);//必须得将处理过的描述符相应的标志位清空
                    FD_CLR(i, &wfds);//必须得将处理过的描述符相应的标志位清空
                    close(i);
                }
                if(--nready == 0) break;
            } else if(FD_ISSET(i, &wset)) {//会在下一次监控时，将数据发送出去
                send(i, buff, n, 0);
                //FD_SET(i, &rfds);
                //FD_CLR(i, &wfds);//必须得将处理过的描述符相应的标志位清空
            }
        }
    }
    return 0;
}

