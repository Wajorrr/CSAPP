#include "csapp.h"

// 给定服务器的域名/ip地址 以及服务器端口，连接服务器，从标准输入读取信息，发送到服务器，并将服务器回送的信息输出到标准输出
int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio; // rio_t类型为一个读缓冲区

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1]; // 服务器域名/ip地址
    port = argv[2]; // 服务器端口

    clientfd = Open_clientfd(host, port); // 请求连接服务器
    // rio_readinitb 函数创建了一个空的读缓冲区，并且将一个打开的文件描述符和这个缓冲区联系起来
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) // 从stdin循环读取MAXLINE长度的数据
    {
        Rio_writen(clientfd, buf, strlen(buf)); // 将读取的数据发送给服务器
        rio_readlineb(&rio, buf, MAXLINE);      // 再从缓冲区中读出服务器回送写入到缓冲区的数据
        printf("received from server: %s", buf);
        // Fputs(buf, stdout);                     // 将从缓存区中读出的数据输出到stdout
    }
    close(clientfd);
    exit(0);
}