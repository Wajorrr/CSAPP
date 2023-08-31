/*
 * myint.c - Another handy routine for testing your tiny shell
 *
 * usage: myint <n>
 * Sleeps for <n> seconds and sends SIGINT to itself.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main(int argc, char **argv)
{
    int i, secs;
    pid_t pid;

    if (argc != 2) // 只能输入1个参数
    {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        exit(0);
    }
    secs = atoi(argv[1]); // 参数转化为数字

    for (i = 0; i < secs; i++) // sleep数字秒
        sleep(1);

    pid = getpid();

    if (kill(pid, SIGINT) < 0) // 给相应进程发SIGINT信号
        fprintf(stderr, "kill (int) error");

    exit(0);
}
