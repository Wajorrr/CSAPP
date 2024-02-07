#include "sbuf.h"

/* Create an empty, bounded, shared FIFO buffer with n slots */ // 初始化一个大小为n的缓冲区
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int));                                    // 缓冲区长度为n
    sp->n = n; /* Buffer holds max of n items */                         // 缓冲区总槽数为n
    sp->front = sp->rear = 0; /* Empty buffer iff front == rear */       // 缓冲区首部和尾部
    Sem_init(&sp->mutex, 0, 1); /* Binary semaphore for locking */       // 二元信号量，保证对缓冲区的互斥访问
    Sem_init(&sp->slots, 0, n); /* Initially, buf has n empty slots */   // 缓冲区可用槽数，初始化为总槽数n
    Sem_init(&sp->items, 0, 0); /* Initially, buf has zero data items */ // 缓冲区可用项目数，初始化为0
}

/* Clean up buffer sp */ // 释放缓冲区
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}

/* Insert item onto the rear of shared buffer sp */ // 生产者，向缓冲区插入新项
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots); /* Wait for available slot */ // 先判断可用槽数信号量并-1，若为0，则阻塞
    P(&sp->mutex); /* Lock the buffer */         // 二元互斥信号量，加锁
    sp->buf[(++sp->rear) % (sp->n)] = item;      /* Insert the item */
    V(&sp->mutex); /* Unlock the buffer */       // 二元互斥信号量，解锁、通知
    V(&sp->items); /* Announce available item */ // 判断可用项目数信号量并+1，若为0，则通知消费者
}

/*Remove and return the first item from buffer sp */ // 消费者，从缓冲区获取项
int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items); /* Wait for available item */ // 先判断可用项目数信号量并-1，若为0，则阻塞
    P(&sp->mutex); /* Lock the buffer */         // 二元互斥信号量，加锁
    item = sp->buf[(++sp->front) % (sp->n)];     /* Remove the item */
    V(&sp->mutex); /* Unlock the buffer */       // 二元互斥信号量，解锁、通知
    V(&sp->slots); /* Announce available slot */ // 判断可用槽数数信号量并+1，若为0，则通知生产者
    return item;
}
