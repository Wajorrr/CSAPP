#include "csapp.h"
#define MAXTHREADS 32

/* Global shared variables */
long gsum = 0; /* Global sum */ // 全局和

long nelems_per_thread; /* Number of elements to sum */ // 每个线程需要计算的数的数量

long psum[MAXTHREADS];

sem_t mutex; /* Mutex to protect global sum */

/* Thread routine for psum-mutex.c */
void *sum_mutex(void *vargp)
{
    // 线程id
    long myid = *((long *)vargp); /* Extract the thread ID */

    // 当前id在数1...n中对应的起始位置
    long start = myid * nelems_per_thread; /* Start element index */

    //// 当前id在数1...n中对应的末尾位置
    long end = start + nelems_per_thread; /* End element index */
    long i;
    for (i = start; i < end; i++)
    {
        P(&mutex); // 互斥量保护全局和
        gsum += i;
        V(&mutex);
    }
    return NULL;
}

/* Thread routine for psum-array.c */
void *sum_array(void *vargp)
{
    long myid = *((long *)vargp);          /* Extract the thread ID */
    long start = myid * nelems_per_thread; /* Start element index */
    long end = start + nelems_per_thread;  /* End element index */
    long i;
    for (i = start; i < end; i++)
    {
        psum[myid] += i;
    }
    return NULL;
}

/* Thread routine for psum-local.c */
void *sum_local(void *vargp)
{
    long myid = *((long *)vargp);          /* Extract the thread ID */
    long start = myid * nelems_per_thread; /* Start element index */
    long end = start + nelems_per_thread;  /* End element index */
    long i, sum = 0;
    // 使用局部变量来消除不必要的内存引用
    for (i = start; i < end; i++)
    {
        sum += i;
    }
    psum[myid] = sum;
    return NULL;
}

int main(int argc, char **argv)
{
    long i, nelems, log_nelems, nthreads, myid[MAXTHREADS];
    pthread_t tid[MAXTHREADS];
    /* Get input arguments */
    if (argc != 3)
    {
        printf("Usage: %s <nthreads> <log_nelems>\n", argv[0]);
        exit(0);
    }
    nthreads = atoi(argv[1]);              // 线程数
    log_nelems = atoi(argv[2]);            // n的log大小
    nelems = (1L << log_nelems);           // n
    nelems_per_thread = nelems / nthreads; // 每个线程需要计算的数的数量
    sem_init(&mutex, 0, 1);

    /* Create peer threads and wait for them to finish */
    for (i = 0; i < nthreads; i++)
    { // 创建若干个线程
        myid[i] = i;
        // Pthread_create(&tid[i], NULL, sum_mutex, &myid[i]);
        // Pthread_create(&tid[i], NULL, sum_array, &myid[i]);
        Pthread_create(&tid[i], NULL, sum_local, &myid[i]);
    }
    for (i = 0; i < nthreads; i++) // 等待所有线程执行完毕
        Pthread_join(tid[i], NULL);
    for (int i = 0; i < nthreads; i++)
        gsum += psum[i];

    /* Check final answer */
    if (gsum != (nelems * (nelems - 1)) / 2)
        printf("Error: result=%ld\n", gsum);
    else
        printf("Right: result=%ld\n", gsum);
    exit(0);
}