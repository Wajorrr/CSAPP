#include "csapp.h"

// 描述符池，保存监听描述符 和 与若干客户端连接时的连接描述符
typedef struct /* Represents a pool of connected descriptors */
{

    int maxfd; /* Largest descriptor in read_set */                 // read_set 中的最大文件描述符
    fd_set read_set; /* Set of all active descriptors */            // 所有活动文件描述符的集合
    fd_set ready_set; /* Subset of descriptors ready for reading */ // 准备好进行读操作的文件描述符的子集
    int nready; /* Number of ready descriptors from select */       // 来自 select 的准备好的文件描述符数量
    int maxi; /* High water index into client array */              // 客户端数组中的高水位索引
    int clientfd[FD_SETSIZE]; /* Set of active descriptors */       // 活动文件描述符的集合
    rio_t clientrio[FD_SETSIZE]; /* Set of active read buffers */   // 活动读缓冲区的集合，每个描述符位置均对应一个读缓冲区
} pool;

// 从各个客户端读取到的累计字节总数
int byte_cnt = 0; /* Counts total bytes received by server */

void init_pool(int listenfd, pool *p)
{
    /* Initially, there are no connected descriptors */
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++)
        p->clientfd[i] = -1;
    /* Initially, listenfd is only member of select read set */
    // 初始时描述符集合中只有一个监听描述符
    // 注意只将监听描述符添加到描述符集合，不添加到描述符池，否则在响应客户端请求时会被作为连接描述符处理
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

// 添加一个与客户端的连接描述符到描述符池中
void add_client(int connfd, pool *p)
{
    int i;
    // 准备好的描述符数量减1，这里减1减去的是监听描述符
    // 将请求连接的客户端的连接描述符添加到描述符池之后，监听描述符就继续重新等待其他客户端连接
    p->nready--;

    for (i = 0; i < FD_SETSIZE; i++) /* Find an available slot */
        if (p->clientfd[i] < 0)      // 遍历描述符池列表，若当前位置值为-1，即未使用过，则为描述符分配当前位置
        {
            /* Add connected descriptor to the pool */
            p->clientfd[i] = connfd;                 // 将描述符池当前位置赋值为当前描述符
            Rio_readinitb(&p->clientrio[i], connfd); // 初始化读取缓存区

            /* Add the descriptor to descriptor set */
            FD_SET(connfd, &p->read_set); // 将当前连接描述符添加到描述符集合中

            // 更新最大的描述符值以及已使用位置的最大索引值
            // 最大描述符值用于在调用select函数时指定传入描述符集合的最大数量
            // 这个最大索引值用于减少在处理客户端请求时对描述符池的遍历次数
            /* Update max descriptor and pool high water mark */
            if (connfd > p->maxfd)
                p->maxfd = connfd;
            if (i > p->maxi)
                p->maxi = i;
            break;
        }
    // 若描述符池已满，则报错
    if (i == FD_SETSIZE) /* Couldn't find an empty slot */
        app_error("add_client error: Too many clients");
}

void check_clients(pool *p)
{
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;
    // 细粒度的I/O多路复用，每次只对一个逻辑流进行一次处理，防止独占
    // 遍历描述符池，依次处理客户端请求
    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++)
    {
        connfd = p->clientfd[i]; // 获取连接描述符
        rio = p->clientrio[i];   // 获取读缓冲区
        /* If the descriptor is ready, echo a text line from it */
        // 若当前位置存在连接描述符，且连接描述符在描述符集合中，则读取客户端消息，并响应
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set)))
        {
            p->nready--; // 已准备好的描述符数量-1

            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) // 从读缓冲区读取客户端消息
            {
                byte_cnt += n; // 从各个客户端读取到的累计字节总数
                printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
                Rio_writen(connfd, buf, n); // 响应，这里的响应是将客户端发来消息直接发送回客户端
            }
            else /*EOF detected, remove descriptor from pool */ // 若读取到EOF，则连接终止
            {
                Close(connfd); // 关闭连接
                // FD_CLR 是一个宏，用于从文件描述符集合中移除一个文件描述符
                // 注意，FD_CLR 不会检查 fd 是否在 set 中。如果 fd 不在 set 中，FD_CLR 也不会有任何效果
                FD_CLR(connfd, &p->read_set); // 将连接描述符从描述符集合中移除
                p->clientfd[i] = -1;          // 将描述符池中对应位置置为空
            }
        }
    }
}

// 主函数，创建服务器，提供I/O多路复用并发处理客户端请求
int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    static pool pool; // 描述符池
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    listenfd = Open_listenfd(argv[1]); // 从指定端口打开一个监听描述符
    init_pool(listenfd, &pool);        // 初始化描述符池，将监听描述符添加到描述符集合中
    while (1)
    {
        /* Wait for lisening/connected descriptor(s) to become ready */
        pool.ready_set = pool.read_set;
        // select返回值为准备好的描述符数量
        pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);

        /* If listening descriptor ready, add new client to pool */
        // 只要监听描述符准备好连接了，则与客户端进行连接，将相应连接描述符添加到池中
        if (FD_ISSET(listenfd, &pool.ready_set))
        {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 接受客户端连接，返回一个连接描述符

            add_client(connfd, &pool); // 将连接描述符添加到描述符池中
        }

        /* Echo a text line from each ready connected descriptor */
        // 依次检查描述符池中的所有连接描述符，对每个客户端请求均进行一次响应，即细粒度的I/O多路复用
        check_clients(&pool);
    }
}