#include "sbuf.h"

#define NTHREADS 4
#define SBUFSIZE 16

static int byte_cnt; /* Byte counter */
static sem_t mutex;  /* and the mutex that protects it */
sbuf_t sbuf;         /* Shared buffer of connected descriptors */

static void init_echo_cnt(void)
{
    Sem_init(&mutex, 0, 1);
    byte_cnt = 0;
}

void echo_cnt(int connfd)
{
    int n;
    char buf[MAXLINE];
    rio_t rio;
    // pthread_once_t once = PTHREAD_ONCE_INIT;和Pthread_once(&once, init_echo_cnt);
    // 这两行代码确保init_echo_cnt函数只被执行一次。这是一种常见的模式，用于初始化只需要进行一次的操作，例如初始化互斥锁。
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    Pthread_once(&once, init_echo_cnt);

    Rio_readinitb(&rio, connfd); // 初始化读缓冲区
    // 从读缓冲区读取客户端发来的请求
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        P(&mutex); // 对 更新byte_cnt变量和打印输出 进行加锁保护
        byte_cnt += n;
        printf("server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
        V(&mutex);
        Rio_writen(connfd, buf, n); // 响应客户端
    }
}

// 新线程的处理逻辑，
void *thread(void *vargp)
{
    Pthread_detach(pthread_self()); // 线程分离

    while (1) // 循环处理，获取描述符缓存区中的连接描述符，然后读取客户端请求并响应
    {
        int connfd = sbuf_remove(&sbuf); /* Remove connfd from buffer */ // 从描述符缓冲区中获取一个连接描述符
        echo_cnt(connfd); /* Service client */                           // 响应客户端请求
        Close(connfd);                                                   // 关闭连接
    }
}

int main(int argc, char **argv)
{
    int i, listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    if (argc != 2)
    {
        fprintf(stderr, "usage : %s <port>\n", argv[0]);
        exit(0);
    }
    listenfd = Open_listenfd(argv[1]); // 从指定端口打开监听描述符
    sbuf_init(&sbuf, SBUFSIZE);        // 初始化生产消费者模式缓冲区
    // 创建多个线程并发等待获取缓冲区中的连接描述符
    for (i = 0; i < NTHREADS; i++) /* Create worker threads */
        Pthread_create(&tid, NULL, thread, NULL);
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        // 接受客户端连接，得到连接描述符
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        // 将连接描述符插入到描述符缓冲区中
        sbuf_insert(&sbuf, connfd); /* Insert connfd in buffer */
    }
}
