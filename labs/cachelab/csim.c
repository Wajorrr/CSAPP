#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct cache_line
{
    int valid;         // 有效位
    int tag;           // 标记位
    int last_vis_time; // 最近一次访问时间戳
    char *data_;
} cache_line_;
typedef struct cache_set
{
    cache_line_ *cache_line;
} cache_set_;
typedef struct cache
{
    int S; // 包含S个缓存组
    int E; // 每个缓存组包含的缓存行数量
    int B; // 每个缓存行包含的字节数量
    cache_set_ *cache_set;
} cache_;

void init(cache_ *cache, int s, int E, int b) // C语言没有构造函数，只能单独定义一个初始化函数
{
    cache->S = 1 << s;
    cache->E = E;
    cache->B = 1 << b;
    cache->cache_set = (cache_set_ *)malloc(sizeof(cache_set_) * cache->S); // 为S个缓存组申请内存
    for (int i = 0; i < cache->S; i++)
    {
        // 为每个组的E个缓存行申请内存
        cache->cache_set[i].cache_line = (cache_line_ *)malloc(sizeof(cache_line_) * cache->E);
        // 缓存行初始化，并为每个缓存行的数据块申请内存
        for (int j = 0; j < cache->E; j++)
        {
            // 本lab中不涉及实际的数据存储，因此不需要给缓存行的数据块申请空间
            // cache->cache_set[i].cache_line[j].data_ = (char *)malloc(sizeof(char) * cache->B);
            cache->cache_set[i].cache_line[j].valid = 0;
            cache->cache_set[i].cache_line[j].tag = 0;
            cache->cache_set[i].cache_line[j].last_vis_time = 0;
        }
    }
}
void del(cache_ *cache)
{
    for (int i = 0; i < cache->S; i++)
    {
        // // 释放每个缓存行的数据块空间
        // for (int j = 0; j < cache->E; j++)
        // {
        //     free(cache->cache_set[i].cache_line[j].data_);
        // }

        // 释放每个缓存组的缓存行空间
        free(cache->cache_set[i].cache_line);
    }
    free(cache->cache_set); // 释放缓存组空间
    free(cache);            // 释放缓存指针
}

// 定义完了数据结构以及初始化、删除函数，接下来把查找缓存、缓存替换需要的函数定义一下

// 查找缓存，参数为缓存组指针cache_set，标记位tag
int find(cache_ *cache, int set_index, int tag, int time)
{
    for (int i = 0; i < cache->E; i++)
    {
        if (cache->cache_set[set_index].cache_line[i].valid &&
            cache->cache_set[set_index].cache_line[i].tag == tag)
        {
            cache->cache_set[set_index].cache_line[i].last_vis_time = time;
            return 1;
        }
    }
    return 0;
}
int IsFull(cache_ *cache, int set_index) // 判断cache的编号为set_index的缓存组是否已满
{
    for (int i = 0; i < cache->E; i++)
    {
        if (cache->cache_set[set_index].cache_line[i].valid == 0)
            return 0;
    }
    return 1;
}
// 缓存驱逐，参数为缓存组指针cache_set
int evict(cache_ *cache, int set_index)
{ // 最高效的写法应该用哈希表+双向链表，但是这里只能用c语言，hash表需要自己实现，就先直接通过遍历上一次时间戳来实现了
    int min_time = 0x3f3f3f3f, evict_idx = 0;
    for (int i = 0; i < cache->E; i++)
    {
        if (cache->cache_set[set_index].cache_line[i].last_vis_time < min_time)
        {
            min_time = cache->cache_set[set_index].cache_line[i].last_vis_time;
            evict_idx = i;
        }
    }
    return evict_idx;
}
// 数据加载到缓存，参数为缓存组指针cache_set，标记位tag
int insert(cache_ *cache, int set_index, int tag, int time)
{
    int line_idx = 0;
    if (IsFull(cache, set_index))
    {
        line_idx = evict(cache, set_index);
        cache->cache_set[set_index].cache_line[line_idx].tag = tag;
        cache->cache_set[set_index].cache_line[line_idx].last_vis_time = time;
        return 1;
    }
    for (int i = 0; i < cache->E; i++)
    {
        if (cache->cache_set[set_index].cache_line[i].valid == 0)
        {
            line_idx = i;
            break;
        }
    }
    cache->cache_set[set_index].cache_line[line_idx].valid = 1;
    cache->cache_set[set_index].cache_line[line_idx].tag = tag;
    cache->cache_set[set_index].cache_line[line_idx].last_vis_time = time;
    return 0;
}

// 写出了缓存需要的函数定义后，先不着急实现，先对把对trace指令的读取处理实现一下
int change2num(char *s)
{
    int res = 0;
    for (int i = 0; s[i]; i++)
    {
        res *= 10;
        res += s[i] - '0';
    }
    return res;
}

void solveTrace(int s, int E, int b, FILE *fp)
{
    cache_ *cache;
    cache = (cache_ *)malloc(sizeof(cache_));
    init(cache, s, E, b);
    int hit = 0, miss = 0, eviction = 0;
    int time = 0; // 时间戳

    // 处理文件输入
    char opt[31];
    // fgets(char* s,int n,FILE *stream) 从stream读取字符串，读取到n个字符，或者读取到一行末尾，或者读取到文件末尾则结束
    while (fgets(opt, 30, fp))
    {
        time++;
        // printf("%s", opt);
        char c[2];
        int addr, size;
        sscanf(opt, "%s %x,%d", c, &addr, &size);
        // printf("%s %d %d\n", c, addr, size);

        int tag = addr >> (s + b);                                  // 标记位
        int set_index = (addr >> b) & ((unsigned)(-1) >> (32 - s)); // 组编号

        if (c[0] == 'L' || c[0] == 'S')
        {
            if (find(cache, set_index, tag, time))
            {
                hit++;
                // printf("%s %x,%d hit\n", c, addr, size);
            }
            else
            {
                miss++;
                // printf("%s %x,%d miss ", c, addr, size);
                if (insert(cache, set_index, tag, time))
                {
                    eviction++;
                    // printf("eviction");
                }
                // printf("\n");
            }
        }
        else if (c[0] == 'M')
        {
            if (find(cache, set_index, tag, time))
            {
                hit += 2;
                // printf("%s %x,%d hit hit\n", c, addr, size);
            }
            else
            {
                miss++;
                // printf("%s %x,%d miss ", c, addr, size);
                if (insert(cache, set_index, tag, time))
                {
                    eviction++;
                    // printf("eviction ");
                }
                hit++;
                // printf("hit\n");
            }
        }
    }
    printSummary(hit, miss, eviction);
    del(cache);
}

void solve(int argc, char *argv[])
{
    int s, E, b;
    FILE *fp; // 文件指针
    for (int i = 1; i < argc; i++)
    {
        // printf("%d %s\n", i, argv[i]);
        if (argv[i][0] == '-')
        {
            if (argv[i][1] == 's')
            {
                i++;
                s = change2num(argv[i]);
            }
            else if (argv[i][1] == 'E')
            {
                i++;
                E = change2num(argv[i]);
            }
            else if (argv[i][1] == 'b')
            {
                i++;
                b = change2num(argv[i]);
            }
            else if (argv[i][1] == 't')
            {
                i++;
                printf("%s\n", argv[i]);
                fp = fopen(argv[i], "r");
            }
        }
    }
    solveTrace(s, E, b, fp);
}

int main(int argc, char *argv[]) // argc为参数数量、argv为参数，其中argv[0]为程序名
{
    solve(argc, argv);
    // printSummary(0, 0, 0);
    return 0;
}
