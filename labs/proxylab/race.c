/* WARNING: This code is buggy! */
#include "csapp.h"
#define N 4
/* Thread routine */
void *thread(void *vargp)
{
    int myid = *((int *)vargp); // 这里使用传过来的i引用进行赋值，与主线程中的循环产生了竞争
    Free(vargp);                // 动态分配空间后注意需要及时释放
    printf("Hello from thread %d\n", myid);
    return NULL;
}
int main()
{
    pthread_t tid[N];
    int i, *ptr;
    // // 竞争版本
    // for (i = 0; i < N; i++)                        // 这里循环对i进行+1了
    //     Pthread_create(&tid[i], NULL, thread, &i); // 将i传引用到对等线程作为线程id

    // 无竞争版本
    for (i = 0; i < N; i++)
    {
        ptr = Malloc(sizeof(int)); // 显式动态地为每个整数 ID 分配一个独立的块
        *ptr = i;
        Pthread_create(&tid[i], NULL, thread, ptr);
    }
    for (i = 0; i < N; i++)
        Pthread_join(tid[i], NULL);
    exit(0);
}
