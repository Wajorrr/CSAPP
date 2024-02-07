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

void sigchld_handler(int sig)
{
    // 信号处理函数，用于处理子进程结束时发送的SIGCHLD信号。
    // 当一个子进程结束时，父进程需要调用waitpid函数来回收子进程的资源，否则会产生僵尸进程
    while (waitpid(-1, 0, WNOHANG) > 0)
        ;
    return;
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    Signal(SIGCHLD, sigchld_handler);
    listenfd = Open_listenfd(argv[1]); // 从给定端口打开监听描述符

    while (1) // 循环等待客户端连接
    {
        clientlen = sizeof(struct sockaddr_storage);
        // 接收到客户端的连接请求，返回非负连接描述符，并获取sockaddr和addrlen
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        if (Fork() == 0) // 创建一个子进程，对客户端进行响应
        {
            // 子进程关闭监听套接字
            Close(listenfd); /* Child closes its listening socket */
            // 调用echo函数来处理客户端的请求
            echo(connfd); /* Child services client */
            // 关闭与客户端的连接(连接描述符)
            Close(connfd); /* Child closes connection with client */
            // 子进程退出
            exit(0); /* Child exits */
        }
        Close(connfd); /* Parent closes connected socket(important!) */
    }
}