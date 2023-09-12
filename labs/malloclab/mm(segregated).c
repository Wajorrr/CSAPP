/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7) // 加上ALIGNMENT-1，若低位有值则进位，然后再舍去剩下的低位值，保证结果为8的倍数

#define SIZE_T_SIZE (ALIGN(sizeof(size_t))) // 把size_t类型的size按ALIGNMENT对齐后的size大小

/* Basic constants and macros */
#define WSIZE 4 /* Word and header footer size (bytes) */            // 单字，4字节
#define DSIZE 8 /* Double word size (bytes) */                       // 双字，8字节
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */ // 扩展堆空间时的标准大小，1<<12byte = 4kb = 1page

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
// 将block的size和是否分配位通过或运算打包，包装成一个字，高29位为size，低3位前两位没用，最后1位指示block是否被分配
#define PACK(size, alloc) ((size) | (alloc))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) // 获取block的size
#define GET_ALLOC(p) (GET(p) & 0x1) // 获取block的是否被分配标记

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))              // 将void指针p强转成unsigned int指针，并返回指向的值
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // 将void指针p强转成unsigned int指针，并赋值

/* Given block ptr bp, compute address of its header and footer */
// block的指针bp默认指向有效载荷的起始位置，size默认存储的是整个block，包括头部和脚部整体的大小
#define HDRP(bp) ((char *)(bp)-WSIZE)                        // 给定block的指针bp，返回其头部的起始位置
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // 给定block的指针bp，返回其脚部的起始位置

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE))) // 给定block的指针bp，返回下个block的指针bp
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))   // 给定block的指针bp，返回上个block的指针bp

/* Segregated Free Lists */
#define GET_HEAD(num) (heap_listp + WSIZE * num)  // 给定序号，找到对应链表头节点位置
#define GET_PRE(bp) (*(unsigned int *)(bp))       // 给定bp,找到前驱
#define GET_SUC(bp) (*((unsigned int *)(bp) + 1)) // 给定bp,找到后继
#define SET_PRE(bp, val) (*(unsigned int *)(bp) = (val))
#define SET_SUC(bp, val) (*((unsigned int *)(bp) + 1) = (val))
#define CLASS_NUM 20 // 大小类的数量，要么20左右(大小范围不能超过数据类型所能表示的范围)，要么506到506+10左右(小的块单独一个大小类)

void *heap_listp; // 链表头部

// 找到块大小对应的大小类空闲链表的序号
int search(size_t size)
{
    int i;
    if (CLASS_NUM < 505) // 按幂对大小类分类
    {
        for (i = 4; i < 3 + CLASS_NUM; i++)
        {
            if (size <= (1 << i))
                return i - 4;
        }
        return i - 4; // [0,19]
    }
    else // 小的块单独分，大的块按幂分
    {
        // 0-16、1-18、2-20、...、503-1022、504-1024
        // 小的块
        if (size <= 1024)
            return size / 2 - 8;
        // 大的块
        int r = 1024;
        for (i = 1; i < CLASS_NUM - 505; i++)
        {
            r *= 2;
            if (size <= r)
                return 504 + i;
        }
        return CLASS_NUM - 1;
    }
}

// 将空闲块插入到相应链表的头部
void insert(void *bp)
{
    // 块大小
    size_t size = GET_SIZE(HDRP(bp));
    // 根据块大小找到对应链表的编号
    int num = search(size);
    void *head = GET_HEAD(num);
    void *first = GET(head);
    if (first == NULL) // 若链表为空
    {
        SET_PRE(bp, NULL);
        SET_SUC(bp, NULL);
    }
    else // 若链表非空
    {
        void *first = GET(head);
        SET_SUC(bp, first); // bp的后继指针放链表的头节点
        SET_PRE(first, bp); // 原头节点的前驱指针放bp
        SET_PRE(bp, NULL);  // bp的前驱指针置为NULL
    }
    PUT(head, bp); // 链表的头结点放bp
}

// 分配块，将块从链表中删除
void delete(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp)); // 块大小
    int num = search(size);           // 根据块大小找到对应链表序号
    void *head = GET_HEAD(num);
    void *pre = GET_PRE(bp);
    void *nex = GET_SUC(bp);
    SET_PRE(bp, NULL);
    SET_SUC(bp, NULL);
    if (pre == NULL)
    {
        if (nex != NULL)
            SET_PRE(nex, NULL);
        PUT(head, nex);
    }
    else
    {
        if (nex != NULL)
            SET_PRE(nex, pre);
        SET_SUC(pre, nex);
    }
}

// 分离空闲链表的合并操作
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 获取前一个块是否已被分配
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 获取后一个块是否已被分配
    size_t size = GET_SIZE(HDRP(bp));                   // 当前块的大小，类型size_t(unsigned long，32bits)

    if (prev_alloc && next_alloc) /* Case 1 */ // 前后都是已分配块，直接返回
    {
        insert(bp);
        return bp;
    }
    else if (prev_alloc && !next_alloc) /* Case 2 */ // 前面是已分配块，后面是空闲块
    {
        delete (NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // size加上后面空闲块的size
        PUT(HDRP(bp), PACK(size, 0));          // 更新header中的size
        PUT(FTRP(bp), PACK(size, 0));          // 更新footer中的size
    }
    else if (!prev_alloc && next_alloc) /* Case 3 */ // 前面是空闲块，后面是已分配块
    {
        delete (PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));   // size加上前面空闲块的size
        PUT(FTRP(bp), PACK(size, 0));            // 更新header中的size
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 更新footer中的size
        bp = PREV_BLKP(bp);                      // bp更新为前面块的bp
    }
    else /* Case 4 */ // 前后都是空闲块
    {
        delete (PREV_BLKP(bp));
        delete (NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));   // size加上前后空闲块的size
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 更新header中的size
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); // 更新footer中的size
        bp = PREV_BLKP(bp);                      // bp更新为前面块的bp
    }
    insert(bp);
    return bp;
}

// 扩展words个字大小的堆空间(若words为奇数，则+1对齐为偶数)，返回扩展后合并后(若可以)的空闲块bp
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // 若为奇数则加1
    if ((long)(bp = mem_sbrk(size)) == -1)                    // 扩展size bytes
        return NULL;
    /* Initialize free block header/footer and the epilogue header */
    // 直接将当前指针作为bp，将之前的结尾块作为新空闲块的header
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */           // 将之前的结尾块初始化为空闲块的header
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */           // 将倒数第二个块初始化为空闲块的footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ // 将最后一个块设置为新的结尾块

    /* Coalesce if the previous block was free */
    return coalesce(bp); // 检查是否可以合并，可以合并则合并并返回合并后的指针
}

// 分离空闲链表的初始化操作，需要为大小类头指针预先分配空间
int mm_init(void)
{
    // 在之前4个字的基础上加上大小类头指针所需的空间
    if ((heap_listp = mem_sbrk((4 + CLASS_NUM) * WSIZE)) == (void *)-1)
        return -1;
    // 初始化大小类头指针
    for (int i = 0; i < CLASS_NUM; i++)
    {
        PUT(heap_listp + i * WSIZE, NULL);
    }

    PUT(heap_listp + CLASS_NUM * WSIZE, 0);                      // 双字边界对齐的不使用的填充字
    PUT(heap_listp + ((1 + CLASS_NUM) * WSIZE), PACK(DSIZE, 1)); // 序言块头
    PUT(heap_listp + ((2 + CLASS_NUM) * WSIZE), PACK(DSIZE, 1)); // 序言块尾
    PUT(heap_listp + ((3 + CLASS_NUM) * WSIZE), PACK(0, 1));     // 结尾块

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) // 将堆的大小扩展1个page
        return -1;
    return 0;
}

// 分离空闲链表的查找合适空闲块操作
static void *find_fit(size_t asize)
{
    int num = search(asize); // 找到asize对应的大小类空闲链表
    void *bp;

    while (num < CLASS_NUM) // 如果找不到合适的块，那么就搜索下一个更大的大小类，找大的块进行分配然后分割
    {
        bp = GET(GET_HEAD(num));
        while (bp) // 顺序搜索当前大小类空闲链表
        {
            if (GET_SIZE(HDRP(bp)) >= asize)
            {
                return bp;
            }
            bp = GET_SUC(bp); // 下一块
        }
        num++; // 找不到合适的块，搜索下一个更大的大小类
    }
    return NULL;
}

// 分离空闲链表的分配与分割操作
static void place(void *bp, size_t asize) // 已在当前bp位置找到合适的空闲块，在当前位置分配一个asize大小的块
{
    size_t bsize = GET_SIZE(HDRP(bp)); // 当前空闲块的实际大小

    delete (bp);
    if (bsize - asize >= 2 * DSIZE) // 若分配asize后剩余空间大于最小可分配大小，则分割空闲块
    {
        PUT(HDRP(bp), PACK(asize, 1));                    // 更新header中的size和是否分配位
        PUT(FTRP(bp), PACK(asize, 1));                    // 更新footer中的size和是否分配位
        PUT(HDRP(NEXT_BLKP(bp)), PACK(bsize - asize, 0)); // 更新剩余空间的header
        PUT(FTRP(NEXT_BLKP(bp)), PACK(bsize - asize, 0)); // 更新剩余空间的footer
        insert(NEXT_BLKP(bp));
    }
    else // 若分配asize后剩余空间小于最小可分配大小，则直接分配整个空闲块
    {
        PUT(HDRP(bp), PACK(bsize, 1)); // 更新header中的size和是否分配位
        PUT(FTRP(bp), PACK(bsize, 1)); // 更新footer中的size和是否分配位
    }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
// 分配块，请求具有size载荷大小的空闲块
void *mm_malloc(size_t size)
{
    size_t asize;      /* Adjust block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0) // malloc(0)，return NULL
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE) // 小于2个字，则扩展到4个字大小 //最小块大小是16字节：8字节用来满足对齐要求，而另外8个用来放头部和脚部
        asize = 2 * DSIZE;
    else // 大于2个字，则扩展到2+ceil(size/DSIZE)*2个字大小 (必须为头部块和尾部块留有空间)
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) // 搜索空闲链表，寻找一个合适的空闲块
    {
        place(bp, asize); // 有合适的，那么分配器就放置这个请求块，并可选地分割出多余的部分，返回新分配块的地址。
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    // 不能够发现一个匹配的块，那么就用一个新的空闲块来扩展堆，把请求块放置在这个新的空闲块里，可选地分割这个块
    extendsize = MAX(asize, CHUNKSIZE); // 至少扩展一个page
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    // 然后返回一个指针，指向这个新分配的块
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0)); // header指示为空闲块
    PUT(FTRP(ptr), PACK(size, 0)); // footer指示为空闲块
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{ // ptr为NULL则表示malloc，size为0则表示free
    void *newptr;
    size_t copysize;

    if ((newptr = mm_malloc(size)) == NULL)
        return 0;
    copysize = GET_SIZE(HDRP(ptr));
    if (size < copysize)
        copysize = size;
    memcpy(newptr, ptr, copysize);
    mm_free(ptr);
    return newptr;
}