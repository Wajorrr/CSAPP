#include "csapp.h"

typedef struct
{
    int *buf; /* Buffer array */                    // 缓冲区
    int n; /* Maximum number of slots */            // 缓冲区总槽数
    int front; /* buf[(front+1)%n] is first item */ // 缓冲区头部
    int rear; /* buf[rear%n] is last item */        // 缓冲区尾部
    sem_t mutex; /* Protects accesses to buf */     // 二元信号量，用来保证对缓冲区的互斥访问
    sem_t slots; /* Counts available slots */       // 缓冲区当前可用槽数
    sem_t items; /* Counts available items */       // 缓冲区中可用项目数
} sbuf_t;

/* Create an empty, bounded, shared FIFO buffer with n slots */ // 初始化一个大小为n的缓冲区
void sbuf_init(sbuf_t *sp, int n);

/* Clean up buffer sp */ // 释放缓冲区
void sbuf_deinit(sbuf_t *sp);

/* Insert item onto the rear of shared buffer sp */ // 生产者，向缓冲区插入新项
void sbuf_insert(sbuf_t *sp, int item);

/*Remove and return the first item from buffer sp */ // 消费者，从缓冲区获取项
int sbuf_remove(sbuf_t *sp);