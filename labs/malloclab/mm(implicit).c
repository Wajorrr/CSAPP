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

void *heap_listp; // 链表头部
void *pre_ptr;    // next_fit使用，标记上一次适配完成时的指针位置，注意在空闲块合并时需要对这个指针进行更新

// 空闲块合并，返回合并后整个空闲块的bp
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 获取前一个块是否已被分配
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 获取后一个块是否已被分配
    size_t size = GET_SIZE(HDRP(bp));                   // 当前块的大小，类型size_t(unsigned long，32bits)

    if (prev_alloc && next_alloc) /* Case 1 */ // 前后都是已分配块，直接返回
    {
        return bp;
    }
    else if (prev_alloc && !next_alloc) /* Case 2 */ // 前面是已分配块，后面是空闲块
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // size加上后面空闲块的size
        PUT(HDRP(bp), PACK(size, 0));          // 更新header中的size
        PUT(FTRP(bp), PACK(size, 0));          // 更新footer中的size
    }
    else if (!prev_alloc && next_alloc) /* Case 3 */ // 前面是空闲块，后面是已分配块
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));   // size加上前面空闲块的size
        PUT(FTRP(bp), PACK(size, 0));            // 更新header中的size
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 更新footer中的size
        bp = PREV_BLKP(bp);                      // bp更新为前面块的bp
    }
    else /* Case 4 */ // 前后都是空闲块
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));   // size加上前后空闲块的size
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 更新header中的size
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); // 更新footer中的size
        bp = PREV_BLKP(bp);                      // bp更新为前面块的bp
    }
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

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) // 成功返回0，失败返回-1
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) // 先申请4个字(16bytes)的空间
        return -1;
    PUT(heap_listp, 0); /* Alignment padding */                          // 第一个字是一个双字边界对齐的不使用的填充字
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */ // 序言块头，序言块的size为两个字(8bytes)，标记为已分配
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ // 序言块尾，序言块的size为两个字(8bytes)，标记为已分配
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); /* Epilogue header */     // 结尾块，size为0，标记为已分配
    heap_listp += (2 * WSIZE);

    pre_ptr = heap_listp;

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) // 将堆的大小扩展1个page
        return -1;
    return 0;
}

// 首次适配，顺序找到第一个合适大小的块进行分配
static void *first_fit(size_t asize)
{
    void *ptr;
    for (ptr = heap_listp; GET_SIZE(HDRP(ptr)) > 0; ptr = NEXT_BLKP(ptr)) // 遍历到结尾size=0的结尾块时结束循环
    {
        if (!GET_ALLOC(HDRP(ptr)) && GET_SIZE(HDRP(ptr)) >= asize)
            return ptr;
    }
    return NULL;
}

// 这样子写会报错，不知道为啥。。。
// 看了下知乎上的评论，原来是当block被分配之后，若又被free了，然后在free操作中和前面的空闲block合并了，则当前的header会失效
// 而若find fit时仍按照当前的header进行分配，则会只使用当前空闲块的后一部分，当前空闲块再次被分配时，会出现重复分配
// 下一次适配，从上一次适配的块位置开始进行顺序寻找适配，若没找到则从头开始
static void *next_fit(size_t asize)
{
    void *ptr;
    for (ptr = pre_ptr; GET_SIZE(HDRP(ptr)) > 0; ptr = NEXT_BLKP(ptr)) // 遍历到结尾size=0的结尾块时结束循环
    {
        if (!GET_ALLOC(HDRP(ptr)) && GET_SIZE(HDRP(ptr)) >= asize) // 先从上一次的块开始找
        {
            pre_ptr = ptr;
            return ptr;
        }
    }
    for (ptr = heap_listp; ptr != pre_ptr; ptr = NEXT_BLKP(ptr)) // 若没找到，则再重头开始找
    {
        if (!GET_ALLOC(HDRP(ptr)) && GET_SIZE(HDRP(ptr)) >= asize)
        {
            pre_ptr = ptr;
            return ptr;
        }
    }
    return NULL;
}

// 最佳适配，寻找满足请求所需大小的最小大小的块进行分配
static void *best_fit(size_t asize)
{
    void *ptr;
    void *best_bp = NULL;
    size_t min_size = 0x3f3f3f3f;
    for (ptr = heap_listp; GET_SIZE(HDRP(ptr)) > 0; ptr = NEXT_BLKP(ptr)) // 遍历到结尾size=0的结尾块时结束循环
    {
        if (!GET_ALLOC(HDRP(ptr)) && GET_SIZE(HDRP(ptr)) >= asize)
        {
            if (min_size > GET_SIZE(HDRP(ptr))) // 找适合所需空间的最小空闲块
            {
                min_size = GET_SIZE(HDRP(ptr));
                best_bp = ptr;
            }
        }
    }
    return best_bp;
}

// 显式空闲链表的查找空闲块操作
static void *find_fit(size_t asize)
{
    //  return first_fit(asize);
    return next_fit(asize);
    //  return best_fit(asize);
}

// 显式空闲链表的分配与分割操作
static void place(void *bp, size_t asize) // 已在当前bp位置找到合适的空闲块，在当前位置分配一个asize大小的块
{
    size_t bsize = GET_SIZE(HDRP(bp)); // 当前空闲块的实际大小

    if (bsize - asize >= 2 * DSIZE) // 若分配asize后剩余空间大于最小可分配大小，则分割空闲块
    {
        PUT(HDRP(bp), PACK(asize, 1));                    // 更新header中的size和是否分配位
        PUT(FTRP(bp), PACK(asize, 1));                    // 更新footer中的size和是否分配位
        PUT(HDRP(NEXT_BLKP(bp)), PACK(bsize - asize, 0)); // 更新剩余空间的header
        PUT(FTRP(NEXT_BLKP(bp)), PACK(bsize - asize, 0)); // 更新剩余空间的footer
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
    // coalesce(ptr);
    pre_ptr = coalesce(ptr);
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

// 失败的尝试
// void *mm_realloc(void *ptr, size_t size)
// {
//     void *oldptr = ptr;
//     void *newptr;
//     size_t copySize;
//     if (oldptr == NULL)
//     {
//         newptr = mm_malloc(size);
//         return newptr;
//     }
//     if (size == 0)
//     {
//         mm_free(oldptr);
//         return NULL;
//     }
//     size_t rsize = GET_SIZE(HDRP(oldptr)) - size;
//     if (rsize >= 0)
//     {
//         if (rsize >= 2 * DSIZE)
//         {
//             PUT(HDRP(oldptr), PACK(size, 1));
//             PUT(FTRP(oldptr), PACK(size, 1));
//             PUT(HDRP(NEXT_BLKP(oldptr)), PACK(rsize, 0));
//             PUT(FTRP(NEXT_BLKP(oldptr)), PACK(rsize, 0));
//             coalesce(NEXT_BLKP(oldptr));
//         }
//         return oldptr;
//     }
//     else
//     {
//         size_t nsize = GET_SIZE(HDRP(NEXT_BLKP(oldptr)));
//         if (!GET_ALLOC(HDRP(NEXT_BLKP(oldptr))) && nsize >= -rsize)
//         {
//             if (nsize - (-rsize) >= WSIZE * 2)
//             {
//                 PUT(HDRP(oldptr), PACK(size, 1));
//                 PUT(FTRP(oldptr), PACK(size, 1));
//                 PUT(HDRP(NEXT_BLKP(oldptr)), PACK(nsize - (-rsize), 1));
//                 PUT(FTRP(NEXT_BLKP(oldptr)), PACK(nsize - (-rsize), 1));
//             }
//             else
//             {
//                 PUT(HDRP(oldptr), PACK(GET_SIZE(HDRP(oldptr)) + nsize, 1));
//                 PUT(FTRP(oldptr), PACK(GET_SIZE(HDRP(oldptr)) + nsize, 1));
//             }
//             return oldptr;
//         }
//         else
//         {
//             newptr = mm_malloc(size);
//             if (newptr == NULL)
//                 return NULL;
//             // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
//             copySize = GET_SIZE(HDRP(oldptr));
//             // if (size < copySize)
//             //     copySize = size;
//             memcpy(newptr, oldptr, copySize);
//             mm_free(oldptr);
//             return newptr;
//         }
//     }
// }