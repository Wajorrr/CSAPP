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

/* Thread routine */
// 新线程的处理逻辑
void *thread(void *vargp)
{
    // 用临时变量存储连接描述符
    int connfd = *((int *)vargp);
    // 线程将自己设置为分离状态，这样当线程结束时，系统会自动回收其资源
    Pthread_detach(pthread_self());
    // 释放空间
    Free(vargp);
    echo(connfd);  // 调用echo函数处理客户端的请求
    Close(connfd); // 关闭连接
    return NULL;
}

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    listenfd = Open_listenfd(argv[1]); // 从给定端口打开监听描述符
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        // 动态分配，记录线程id
        // 这个指针是动态分配的，以避免在多个线程之间共享同一个栈变量
        connfdp = Malloc(sizeof(int));
        // 接收到客户端的连接请求，返回非负连接描述符，并获取sockaddr和addrlen
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        // 创建线程，将连接描述符传递给新线程
        // 新线程的参数是一个指向连接文件描述符的指针
        Pthread_create(&tid, NULL, thread, connfdp);
    }
}
