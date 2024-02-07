#include "csapp.h"

// 传入连接描述符
void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio; // 读缓冲区

    // rio_readinitb 函数创建了一个空的读缓冲区，并且将一个打开的文件描述符和这个缓冲区联系起来
    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) // 循环从缓冲区中读取
    {
        printf("server received %d bytes, message: %s\n", (int)n, buf); // 从客户端接收到的字节数

        Rio_writen(connfd, buf, n); // 将接收到的客户端发来的数据再发送回客户端
    }
}

// 给定端口，在给定端口打开监听描述符
int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough space for any address */

    char client_hostname[MAXLINE], client_port[MAXLINE]; // 客户端的ip和端口

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]); // 从给定端口打开监听描述符
    while (1)                          // 循环等待客户端连接
    {
        clientlen = sizeof(struct sockaddr_storage);
        // 接收到客户端的连接请求，返回非负连接描述符，并获取sockaddr和addrlen
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        // 将套接字地址结构sockaddr转换成相应的主机和服务名(端口)字符串
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s) \n", client_hostname, client_port); // 输出当前连接的客户端ip和端口

        echo(connfd);  // 从连接描述符读取客户端发来的信息，并将相同的信息发送回去
        Close(connfd); // 当前连接结束，继续等待下一个客户端连接
    }
    exit(0);
}