/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);

/* $begin tinymain */
// 传入参数为端口号，初始化web服务器，等待接收客户端连接请求
int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]); // 服务器根据端口号打开监听描述符
    while (1)
    { // 循环等待客户端连接
        clientlen = sizeof(clientaddr);
        // 接收客户端连接请求
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept
        // 获取客户端的ip和端口
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        // 根据连接描述符，调用doit函数处理一个HTTP事务
        doit(connfd);  // line:netp:tiny:doit
        Close(connfd); // line:netp:tiny:close
    }
}
/* $end tinymain */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
// 传入参数为连接描述符
void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    // 初始化读缓冲区，并与连接描述符绑定
    Rio_readinitb(&rio, fd);
    // 读取和解析请求行
    if (!Rio_readlineb(&rio, buf, MAXLINE)) // line:netp:doit:readrequest
        return;
    printf("%s", buf); // 打印读取的请求行

    // 从请求行中解析出请求方法、uri以及HTTP版本
    sscanf(buf, "%s %s %s", method, uri, version); // line:netp:doit:parserequest

    // 判断请求方法，这里只对GET方法进行处理
    if (strcasecmp(method, "GET"))
    { // line:netp:doit:beginrequesterr
        // 若不是GET方法，报错501
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    } // line:netp:doit:endrequesterr
    // 读取请求报头，并忽略所有请求报头
    read_requesthdrs(&rio); // line:netp:doit:readrequesthdrs

    // 自此读取请求结束

    /* Parse URI from GET request */
    // 传入uri字符串，解析uri，解析后若为静态内容则返回1，若为动态内容则返回0
    is_static = parse_uri(uri, filename, cgiargs); // line:netp:doit:staticcheck

    // 调用stat函数获取文件信息，若执行失败，则返回文件不存在
    if (stat(filename, &sbuf) < 0)
    { // line:netp:doit:beginnotfound
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    } // line:netp:doit:endnotfound

    if (is_static) // 静态内容
    {              /* Serve static content */

        // 若请求的文件不是普通文件或不具有读权限，返回403
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        { // line:netp:doit:readable
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        // 否则返回静态内容
        serve_static(fd, filename, sbuf.st_size); // line:netp:doit:servestatic
    }
    else // 动态内容
    {    /* Serve dynamic content */

        // 若请求的文件不是普通文件或不具有读权限，返回403
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
        { // line:netp:doit:executable
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        // 否则返回动态内容
        serve_dynamic(fd, filename, cgiargs); // line:netp:doit:servedynamic
    }
}
/* $end doit */

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
// 读请求报头并忽略它们
void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    // 循环读取直到遇到分隔的空行为止，表示请求报头结束
    while (strcmp(buf, "\r\n"))
    { // line:netp:readhdrs:checkterm
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
// 传入uri字符串，解析uri，解析后若为静态内容则返回1，若为动态内容则返回0
// 解析完uri之后，将filename赋值为文件路径，cgiargs赋值为请求动态内容时uri中传入的CGI参数字符串('?'后面的内容)
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    // uri中是否不包含"cgi-bin"(与当前文件同级的一个文件夹，里面是可执行文件，即动态内容)
    if (!strstr(uri, "cgi-bin")) // 不包含"cgi-bin"，则为静态内容
    { /* Static content */       // line:netp:parseuri:isstatic

        strcpy(cgiargs, "");               // line:netp:parseuri:clearcgi // 清空cgiargs(CGI参数字符串)
        strcpy(filename, ".");             // line:netp:parseuri:beginconvert1 // filename="."
        strcat(filename, uri);             // line:netp:parseuri:endconvert1  // filename="./..."
        if (uri[strlen(uri) - 1] == '/')   // line:netp:parseuri:slashcheck // 若末尾字符为"/"，则返回主页文件路径
            strcat(filename, "home.html"); // line:netp:parseuri:appenddefault // filename="./home.html"
        return 1;
    }
    else                    // 包含"cgi-bin"，则为动态内容
    { /* Dynamic content */ // line:netp:parseuri:isdynamic

        ptr = index(uri, '?'); // line:netp:parseuri:beginextract //uri的'?'后接CGI参数字符串
        if (ptr)
        {
            strcpy(cgiargs, ptr + 1); // 将CGI参数字符串复制到cgiargs
            *ptr = '\0';
        }
        else                     // 不存在传入参数
            strcpy(cgiargs, ""); // line:netp:parseuri:endextract // 清空CGI参数字符串cgiargs

        strcpy(filename, "."); // line:netp:parseuri:beginconvert2
        strcat(filename, uri); // line:netp:parseuri:endconvert2 //可执行文件路径
        return 0;
    }
}
/* $end parse_uri */

/*
 * serve_static - copy a file back to the client
 */
/* $begin serve_static */
// 传入参数为连接描述符、文件地址、文件大小，将静态文件内容通过连接描述符发送给客户端
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    // 获取文件类型
    get_filetype(filename, filetype); // line:netp:servestatic:getfiletype

    // 响应行
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); // line:netp:servestatic:beginserve
    Rio_writen(fd, buf, strlen(buf));

    // 响应报头
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n", filesize); // 文件长度
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n\r\n", filetype); // 文件类型
    Rio_writen(fd, buf, strlen(buf));                   // line:netp:servestatic:endserve

    // 响应主体
    /* Send response body to client */
    // 打开文件，得到文件描述符srcfd，O_RDONLY表示以只读方式打开文件
    srcfd = Open(filename, O_RDONLY, 0); // line:netp:servestatic:open

    // 将文件映射到内存，Mmap函数的返回值srcp是映射区的起始地址
    // PROT_READ表示映射区可以被读取，MAP_PRIVATE表示对映射区的修改不会写回原文件
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // line:netp:servestatic:mmap

    // 关闭文件描述符srcfd，因为文件已经映射到内存，所以不再需要文件描述符
    // 执行这项任务失败将导致潜在的致命的内存泄漏
    Close(srcfd); // line:netp:servestatic:close

    // 将内存中的数据(即文件数据)发送到客户端
    // fd是客户端的文件描述符，srcp是内存中的数据起始地址，filesize是数据的大小
    Rio_writen(fd, srcp, filesize); // line:netp:servestatic:write

    // 解除内存映射，srcp是映射区的起始地址，filesize是映射区的大小
    // 这对于避免潜在的致命的内存泄漏是很重要的
    Munmap(srcp, filesize); // line:netp:servestatic:munmap
}

/*
 * get_filetype - derive file type from file name
 */
// 传入文件路径，解析文件类型，存入到filetype字符串中
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html")) // html文件
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif")) // gif文件
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png")) // png文件
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg")) // jpg文件
        strcpy(filetype, "image/jpeg");
    else // 普通文本文件
        strcpy(filetype, "text/plain");
}
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = {NULL};

    /* Return first part of HTTP response */
    // 响应行
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    // 响应报头
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    // 在子进程中，Fork()函数返回0，在父进程中，Fork()函数返回子进程的PID
    if (Fork() == 0) /* Child */
    {                // line:netp:servedynamic:fork

        /* Real server would set all CGI vars here */ // 一个真正的服务器还会在此处设置其他的 CGI 环境变量
        // 设置环境变量，QUERY_STRING是一个CGI环境变量，通常用于传递给CGI程序的参数
        setenv("QUERY_STRING", cgiargs, 1); // line:netp:servedynamic:setenv

        // 将标准输出重定向到客户端，Dup2()函数会关闭STDOUT_FILENO（也就是标准输出），然后将fd复制到STDOUT_FILENO
        // 这样，当CGI程序向标准输出写入数据时，数据实际上会被发送到客户端
        Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */ // line:netp:servedynamic:dup2

        // 因为CGI程序运行在子进程的上下文中，它能够访问所有在调用ex­ecve函数之前就存在的打开文件和环境变量
        // 因此，CGI程序写到标准输出上的任何东西都将直接送到客户端进程，不会受到任何来自父进程的干涉
        // 执行CGI程序。Execve()函数会替换当前进程的映像，使其变为filename指定的程序
        // emptylist是传递给新程序的参数列表，environ是新程序的环境变量列表。
        Execve(filename, emptylist, environ); /* Run CGI program */ // line:netp:servedynamic:execve
    }
    // 父进程等待子进程结束。Wait()函数会使父进程阻塞，直到一个子进程结束
    // 当子进程结束时，Wait()函数返回子进程的PID，并通过第一个参数返回子进程的退出状态
    Wait(NULL); /* Parent waits for and reaps child */ // line:netp:servedynamic:wait
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
// 发送一个 HTTP 响应到客户端，在响应行中包含相应的状态码和状态消息，响应主体中包含一个HTML文件，向浏览器的用户解释这个错误
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // 错误类型编号以及短提示
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n"); // 响应主体中内容的MIME类型
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>"); // 响应主体
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor="
                 "ffffff"
                 ">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg); // 错误类型编号以及短提示
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause); // 长提示以及错误原因(未实现的方法、未找到的文件等)
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */
