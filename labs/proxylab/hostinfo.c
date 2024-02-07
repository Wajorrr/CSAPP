#include "csapp.h"

// 输入域名，返回其IP地址和端口号
int main(int argc, char **argv)
{
    struct addrinfo *p, *listp, hints;
    char buf[MAXLINE], service[MAXLINE];
    int rc, flags;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <domain name>\n", argv[0]);
        exit(0);
    }

    /* Get a list of addrinfo records */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* ipv4 only */          // 将列表限制为 IPv4 地址
    hints.ai_socktype = SOCK_STREAM; /* Connect Only */ // 将列表限制为对每个地址最多一个 addrinfo 结构，该结构的套接字地址可以作为连接的一个端点
    hints.ai_flags = AI_NUMERICSERV;                    // 参数 service 默认可以是服务名或端口号。这个标志强制参数 service 为端口号 。

    if ((rc = getaddrinfo(argv[1], NULL, &hints, &listp)) != 0) // 将域名转换为套接字地址结构，即addrinfo构成的链表
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
        exit(0);
    }
    /* Walk the list and display each IP address */
    // NI_NUMERICHOST：getnameinfo 默认试图返回host中的域名。设置该标志会使该函数返回一个数字地址字符串。
    flags = NI_NUMERICHOST;            /* Display address string instead of domain name */
    for (p = listp; p; p = p->ai_next) // 顺序读取链表中的每个ip地址
    {
        Getnameinfo(p->ai_addr, p->ai_addrlen, buf, MAXLINE, service, MAXLINE, flags); // 将一个套接字地址结构转换成相应的主机和服务名字符串

        printf("%s : %s\n", buf, service); // 打印host,service(ip:端口)
    }

    /* Clean up */
    Freeaddrinfo(listp);

    exit(0);
}