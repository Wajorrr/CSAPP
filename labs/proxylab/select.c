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

void command(void)
{
    // command函数从标准输入读取一行文本，然后打印出来。
    // 如果读取到 EOF（文件结束标记），则退出程序
    char buf[MAXLINE];
    if (!Fgets(buf, MAXLINE, stdin))
        exit(0);       /* EOF */
    printf("%s", buf); /* Process the input command */
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    fd_set read_set, ready_set; // 描述符集合

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    // 打开一个监听套接字
    listenfd = Open_listenfd(argv[1]);

    // 初始化一个文件描述符集合（read_set），这个集合包含标准输入和监听套接字。
    // 空描述符集合
    FD_ZERO(&read_set); /* Clear read set */
    // 将标准输入stdin描述符加入描述符集合
    FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */
    // 将监听套接字的监听描述符加入描述符集合
    FD_SET(listenfd, &read_set); /* Add listenfd to read set */

    while (1)
    {
        // 更新当前的准备好集合
        ready_set = read_set;
        // 使用select函数等待标准输入或一个客户端连接变得可读
        Select(listenfd + 1, &ready_set, NULL, NULL, NULL);
        // 如果文件描述符fd在set中，FD_ISSET返回非零值，否则返回0
        // 检查标准输入是否已经准备好，变得可读
        if (FD_ISSET(STDIN_FILENO, &ready_set))
            command(); /* Read command line from stdin */
        // 检查客户端连接是否已经准备好，变得可读
        if (FD_ISSET(listenfd, &ready_set))
        {
            clientlen = sizeof(struct sockaddr_storage);
            // 接受客户端连接
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            // 调用echo函数来处理客户端的请求
            echo(connfd); /* Echo client input until EDF */
            // 关闭连接
            Close(connfd);
        }
    }
}