/*
 * tsh - A tiny shell program with job control
 *
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t
{                          /* The job struct */
    pid_t pid;             /* job PID */
    int jid;               /* job ID [1, 2, ...] */
    int state;             /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE]; /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

// 定义一些错误包装函数
pid_t Fork(void);                                                          // 新建子进程
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);          // 设置显式阻塞的信号集合对应的二进制码
void Sigemptyset(sigset_t *set);                                           // 空集合，不显式阻塞任何信号
void Sigfillset(sigset_t *set);                                            // fillset，显式阻塞所有信号
void Sigaddset(sigset_t *set, int signum);                                 // 将要阻塞的信号添加到集合中
void Execve(const char *filename, char *const argv[], char *const envp[]); // 执行可执行目标文件filename
void Setpgid(pid_t pid, pid_t pgid);                                       // 将进程pid的进程组改成pgid
void Kill(pid_t pid, int sig);                                             // 发送信号sig给进程pid

pid_t Fork(void)
{
    pid_t pid;
    if ((pid = fork()) < 0) // fork函数：子进程返回0，父进程返回子进程的pid，出错时返回-1
        unix_error("Fork error");
    return pid;
}

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0) // 成功时返回0，若出错则为-1
        unix_error("Sigprocmask error");
}

void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0) // 成功时返回0，若出错则为-1
        unix_error("Sigemptyset error");
}

void Sigfillset(sigset_t *set)
{
    if (sigfillset(set) < 0) // 成功时返回0，若出错则为-1
        unix_error("Sigfillset error");
}

void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0) // 成功时返回0，若出错则为-1
        unix_error("Sigaddset error");
}

void Execve(const char *filename, char *const argv[], char *const envp[])
{
    if (execve(filename, argv, envp) < 0) // 成功时不返回，若出错则为-1
        printf("%s: Command not found.\n", argv[0]);
}

void Setpgid(pid_t pid, pid_t pgid)
{
    if (setpgid(pid, pgid) < 0) // 成功时返回0，若出错则为-1
        unix_error("Setpgid error");
}

void Kill(pid_t pid, int sig)
{
    if (kill(pid, sig) < 0) // 成功时返回0，若出错则为-1
        unix_error("Kill error");
}

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2); // 文件描述符中 0表示标准输入,1表示标准输出，2表示标准错误 //这里表示将标准错误输出重定向到标准输出中

    /* Parse the command line */
    // int getopt(int argc, char * const argv[], const char *optstring);
    // getopt() 函数用于解析命令行参数。它的 argc 和 argv 参数通常直接从 main() 的参数直接传递而来。
    // optstring 是可以处理选项字母组成的字符串。该字串里的每个字符对应于一个以 ‘-’ 开头的选项。如果该字串里的任一字符后面有冒号，那么这个选项就要求有参数
    // （如“hd:”对应于 '-h' 和 '-d'， 其中 '-d' 后需接参数）。而如果选项后面接两个冒号，则说明这个选项后的参数是可选的，即可带参数也可不带参数。
    // 如果选项成功找到，返回选项字母；如果所有命令行选项都解析完毕，返回 -1；如果遇到选项字符不在 optstring 中，返回字符 '?'；如果遇到丢失参数，那么返回值依赖于 optstring 中第一个字符，如果第一个字符是 ':' 则返回':'，否则返回'?'并提示出错误信息。
    while ((c = getopt(argc, argv, "hvp")) != EOF)
    {
        switch (c)
        {
        case 'h': /* print help message */
            usage();
            break;
        case 'v': /* emit additional diagnostic info */
            verbose = 1;
            break;
        case 'p':            /* don't print a prompt */
            emit_prompt = 0; /* handy for automatic testing */
            break;
        default:
            usage();
        }
    }

    /* Install the signal handlers */
    // 添加信号处理进程
    /* These are the ones you will need to implement */
    Signal(SIGINT, sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    // 初始化(clear) job list
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1)
    {

        /* Read command line */
        if (emit_prompt)
        {
            printf("%s", prompt);
            // 默认情况下，stdout是行缓冲的。这意味着，发送到 stdout 的输出不会被立即发送到屏幕以供显示（或重定向文件/流），直到它在其中获得换行符。
            // 因此，如果要覆盖默认缓冲行为，则可以使用 fflush 清除缓冲区（立即将所有内容发送到屏幕/文件/流）。
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) // 读取一行到cmdline中
            app_error("fgets error");
        if (feof(stdin))
        { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /* Evaluate the command line */
        eval(cmdline); // 解析
        fflush(stdout);
        fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */
void eval(char *cmdline)
{
    char *argv[MAXARGS]; /* Argument list execve () */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline);
    bg = parseline(buf, argv); // 是否后台运行

    sigset_t mask_all, mask_one, prev_one;
    Sigfillset(&mask_all);
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIGCHLD); // 此时mask_one表示的集合中只包含SIGCHLD信号

    if (argv[0] == NULL) /* Ignore empty lines */
        return;

    if (!builtin_cmd(argv)) // 非内置命令，则视为可执行程序
    {
        Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);      // fork前阻塞SIGCHLD信号，防止并行错误
        if ((pid = Fork()) == 0) /* Child runs user job */ // 子进程执行的部分
        {
            // 由于子进程会继承父进程的信号阻塞情况，因此在子进程中也需要解除SIGCHLD信号的阻塞，因为可能会有子进程再派生子进程的情况
            Sigprocmask(SIG_SETMASK, &prev_one, NULL);
            Setpgid(0, 0); // 创建新进程组，ID设置为子进程PID，并将子进程加入新建的进程组

            Execve(argv[0], argv, environ); // 子进程运行
            exit(0);                        // 子进程执行完毕后一定要退出
        }

        /* Parent waits for foreground job to terminate */
        if (!bg) // 前台运行
        {
            Sigprocmask(SIG_BLOCK, &mask_all, NULL);   // 添加工作前阻塞所有信号
            addjob(jobs, pid, FG, cmdline);            // 添加至作业列表
            Sigprocmask(SIG_SETMASK, &mask_one, NULL); // 由于waitfg中会对SIGCHLD进行解除阻塞并挂起，唤醒后恢复阻塞，因此这里不能直接解除对SIGCHLD的阻塞
            waitfg(pid);                               // 等待前台进程执行完毕

            // int status;
            // pid_t waitpid(pid_t pid, int *statusp, int options);
            // 返回：如果成功，则为子进程的 PID, 如果 WNOHANG, 则为 0, 如果其他错误，则为 -1
            // if (waitpid(pid, &status, 0) < 0) // 前台等待回收僵死进程
            //   unix_error("waitfg: waitpid error");
        }
        else // 后台运行
        {
            Sigprocmask(SIG_BLOCK, &mask_all, NULL); // 添加工作前阻塞所有信号
            addjob(jobs, pid, BG, cmdline);          // 添加至作业列表
            // 注意这里的打印信息应和样例程序一致，参照： make rtest04，格式为 [job_id] (process_id) cmdline
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
            Sigprocmask(SIG_SETMASK, &mask_one, NULL); // 和前面前台job统一逻辑，在后面一起解除对SIGCHLD的阻塞
        }
        Sigprocmask(SIG_SETMASK, &prev_one, NULL); // 解除SIGCHLD信号的阻塞
    }
    return;
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';   /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'')
    {
        buf++;
        delim = strchr(buf, '\''); // delim指向后一个分隔符'\'的位置
    }
    else
    {
        delim = strchr(buf, ' '); // delim指向后一个分隔符' '的位置
    }

    while (delim)
    {
        argv[argc++] = buf;
        *delim = '\0';                // 分隔符替换为'\0'
        buf = delim + 1;              // 下一个参数
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;
        // 和前面一样，buf指向下一个参数开始的位置，delim指向下一个参数和下下个参数之前的分隔符
        if (*buf == '\'')
        {
            buf++;
            delim = strchr(buf, '\'');
        }
        else
        {
            delim = strchr(buf, ' ');
        }
    }
    // end位置置NULL
    argv[argc] = NULL;

    // 判断是否输入了空行
    if (argc == 0) /* ignore blank line */
        return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0) // 判断最后一个参数是否是'&'
    {
        argv[--argc] = NULL;
    }
    return bg;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv)
{
    if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) // bg/fg：将一个停止运行或正在运行的后台job转变为运行的后台/前台job
    {
        do_bgfg(argv);
        return 1;
    }
    if (!strcmp(argv[0], "jobs")) // 打印job列表
    {
        listjobs(jobs);
        return 1;
    }
    if (!strcmp(argv[0], "quit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
        return 1;
    return 0; /* not a builtin command */
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv)
{
    struct job_t *job = NULL; // 要处理的job
    int id;                   // 存储jid或pid
    if (argv[1] == NULL)
    { // 没带参数，参照样例程序，给予提示
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }
    if (argv[1][0] == '%') // 第二个参数的第一个字符是%，说明是jid
    {
        if (sscanf(&argv[1][1], "%d", &id) > 0)
        {
            job = getjobjid(jobs, id); // 通过jobid找到job
            if (job == NULL)
            {
                printf("%%%d: No such job\n", id);
                return;
            }
        }
    }
    else if (!isdigit(argv[1][0])) // 第二个参数的第一个字符既不是%又不是数字，非法输入，参照样例程序，给予提示
    {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }
    else
    { // 第二个参数直接是数字，则为pid
        id = atoi(argv[1]);
        job = getjobpid(jobs, id); // 通过pid找到job
        if (job == NULL)
        {
            printf("(%d): No such process\n", id);
            return;
        }
    }
    // int kill(pid_t pid, int sig);
    // 如果pid大于零，那么函数发送信号号码sig给进程pid。如果pid等于零，那么发送信号sig给调用进程所在进程组中的每个进程，包括调用进程自己。
    // 如果pid小于零，发送信号 sig 给进程组 |pid| (pid的绝对值)中的每个进程
    Kill(-(job->pid), SIGCONT); // 重启进程, 这里发送到进程组pid中的每个进程

    if (!strcmp(argv[0], "bg")) // 后台job重启
    {
        job->state = BG;
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    }
    else // 前台job重启
    {
        job->state = FG;
        waitfg(job->pid); // 有前台进程执行，阻塞tsh主进程，等待前台job执行完毕
    }
    return;
}

/*
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid) // 有前台job在执行时，将tsh主进程挂起，等待前台job执行完毕
{
    sigset_t mask;
    Sigemptyset(&mask);
    while (fgpid(jobs) != 0) // 有前台job正在运行，则阻塞
    {
        sigsuspend(&mask); // 用空集合替换当前的阻塞集合，然后挂起，收到信号后恢复原阻塞集合，退出挂起状态
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 */
void sigchld_handler(int sig)
{
    int olderrno = errno; // 由于errno是全局变量,注意保存和恢复errno
    int status;
    pid_t pid;
    struct job_t *job;
    sigset_t mask, prev;
    sigfillset(&mask);
    // 如果 pid=-1, 那么等待集合就是由父进程所有的子进程组成的
    // WNOHANG | WUNTRACED: 立即返回，如果等待集合中的子进程都没有被停止或终止，则返回值为 0; 如果有一个停止或终止，则返回值为该子进程的 PID
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
    {                                         // 立即返回该子进程的pid
        sigprocmask(SIG_BLOCK, &mask, &prev); // deletejob前阻塞所有信号
        if (WIFEXITED(status))
        { // 正常终止
            deletejob(jobs, pid);
        }
        else if (WIFSIGNALED(status))
        { // 子进程是因为一个未被捕获的信号终止的, 打印相应信息
            printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
            deletejob(jobs, pid);
        }
        else if (WIFSTOPPED(status))
        { // 引起返回的子进程当前是停止的(sleep了或者接受了SIGTSTP信号), 打印相应信息
            printf("Job [%d] (%d) stoped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status));
            job = getjobpid(jobs, pid);
            job->state = ST; // 更新job状态为stopped
        }
        sigprocmask(SIG_SETMASK, &prev, NULL); // 恢复阻塞前的状态
    }
    errno = olderrno; // 恢复errno
    return;
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig)
{
    int olderrno = errno; // 保存和恢复errno
    int pid;
    sigset_t mask_all, prev;
    Sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &prev);
    if ((pid = fgpid(jobs)) != 0) // 若存在前台进程，才发送SIGINT信号
    {
        Sigprocmask(SIG_SETMASK, &prev, NULL);
        Kill(-pid, SIGINT);
    }
    errno = olderrno;
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig)
{
    int olderrno = errno; // 保存和恢复errno
    int pid;
    sigset_t mask_all, prev;
    Sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &prev);
    if ((pid = fgpid(jobs)) > 0) // 若存在前台进程，才发送SIGSTOP信号
    {
        Sigprocmask(SIG_SETMASK, &prev, NULL);
        Kill(-pid, SIGTSTP);
    }
    errno = olderrno;
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs)
{
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid > max)
            max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == 0) // 找空位
        {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(jobs[i].cmdline, cmdline);
            if (verbose)
            {
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
        }
    }
    // job list已满
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == pid)
        {
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs) + 1; // 删除一个job后，要更新nextjid
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].state == FG)
            return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
            return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid)
{
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid == jid)
            return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
// 给定pid，返回pid对应的job的jid
int pid2jid(pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
        {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid != 0)
        {
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state)
            {
            case BG:
                printf("Running ");
                break;
            case FG:
                printf("Foreground ");
                break;
            case ST:
                printf("Stopped ");
                break;
            default:
                printf("listjobs: Internal error: job[%d].state=%d ",
                       i, jobs[i].state);
            }
            printf("%s", jobs[i].cmdline);
        }
    }
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
