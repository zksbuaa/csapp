/*
 * mm-naive.c - A malloc package base on segregated fit and first fit.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"


team_t team = {
    "noteam",
    "zks",
    "qaq",
    "",
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define MAX(x, y) (((x)>(y))?(x):(y))
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE((char*)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))

#define PREV_FREE(bp) (*(unsigned int*)(bp))
#define NEXT_FREE(bp) (*(unsigned int*)((char*)(bp) + WSIZE))

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
void add_free_list(void *bp, size_t pure_size);
static void *find_fit(size_t size);
static void *place(void *bp, size_t size);
static void delete_free_list(void *bp);


size_t chunksize = (1<<12);
void *heap_listp;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(12*WSIZE)) == (void*)-1)
        return -1;
    for (int i = 0; i < 9; ++i)
        PUT((char*)heap_listp + (i*WSIZE), 0);
    PUT((char*)heap_listp + (9*WSIZE), PACK(DSIZE, 1));
    PUT((char*)heap_listp + (10*WSIZE), PACK(DSIZE, 1));
    PUT((char*)heap_listp + (11*WSIZE), PACK(0, 1));
    void *bp;
    if ((bp = extend_heap(chunksize/WSIZE)) == NULL)
        return -1;
    return 0;
}



/* 
 * mm_malloc - Policy: segregated fit and first fit.
 */
void *mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;
    size_t align_size = ALIGN(size);
    void *bp;
    if ((bp = find_fit(align_size)) != NULL) {
        bp = place(bp, align_size);
        return bp;
    }
    /* No fit found. Get more memory to place the block */
    size_t extendsize = MAX(chunksize, align_size + DSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    bp = place(bp, align_size);
    return bp;
}


/*
 * mm_free - Coalescing next free blocks immediately.
 */
void mm_free(void *ptr)
{
    if (!GET_ALLOC(HDRP(ptr))) {
        printf("Error: pointer \"ptr\" is not allocated.\n");
        return;
    }
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    ptr = coalesce(ptr);
    add_free_list(ptr, GET_SIZE(HDRP(ptr)));
}

/*
 * mm_realloc - Coalescing the next block if possible.
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size == 0) {
        mm_free(ptr);
        return ptr;
    }
    size_t newsize = ALIGN(size) + DSIZE;
    size_t oldsize = GET_SIZE(HDRP(ptr));
    if (oldsize > newsize) {
        if (oldsize >= newsize + 2*DSIZE) {
            PUT(HDRP(ptr), PACK(newsize, 1));
            PUT(FTRP(ptr), PACK(newsize, 1));
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(oldsize - newsize, 0));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(oldsize - newsize, 0));
            add_free_list(NEXT_BLKP(ptr), oldsize - newsize - DSIZE);
            return ptr;
        } else {
            return ptr;
        }
    } else if (oldsize < newsize) {
        // 只考虑合并后面的块，不考虑合并前边的块，否则memcpy位置可能重叠且开销过大
        size_t next_size = (GET_ALLOC(HDRP(NEXT_BLKP(ptr))))?0:GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        size_t total_size = oldsize + next_size;
        if (total_size >= newsize) {
            delete_free_list(NEXT_BLKP(ptr));
            PUT(HDRP(ptr), PACK(total_size, 1));
            PUT(FTRP(ptr), PACK(total_size, 1));
            return ptr;
        } else {
            void *bp = mm_malloc(newsize - DSIZE);
            memcpy(bp, ptr, oldsize - DSIZE);
            mm_free(ptr);
            return bp;
        }
    } else {
        return ptr;
    }
}


void *extend_heap(size_t words) {
    char *bp;
    size_t size;
    size = (words%2)?(words+1)*WSIZE:words*WSIZE;
    if ((bp = (char*)mem_sbrk(size)) == (char*)-1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    bp = (char*)coalesce(bp);
    size_t pure_size = GET_SIZE(HDRP(bp)) - DSIZE;
    add_free_list(bp, pure_size);
    return bp;
}


void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && next_alloc) {
        return bp;
    } else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_free_list(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        delete_free_list(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(FTRP(PREV_BLKP(bp)));
        delete_free_list(PREV_BLKP(bp));
        delete_free_list(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

void add_free_list(void *bp, size_t pure_size) {
    size_t group;
    if (pure_size <= 64)
        group = 1;
    else if (pure_size <= 128)
        group = 2;
    else if (pure_size <= 256)
        group = 3;
    else if (pure_size <= 512)
        group = 4;
    else if (pure_size <= 1024)
        group = 5;
    else if (pure_size <= 2048)
        group = 6;
    else if (pure_size <= 4096)
        group = 7;
    else
        group = 8;
    unsigned int *tmp = *(unsigned int*)((char*)heap_listp + (group*WSIZE));
    PUT((char*)heap_listp + (group*WSIZE), bp);
    PREV_FREE(bp) = (unsigned int)((char*)heap_listp + (group*WSIZE));
    NEXT_FREE(bp) = tmp;
    if (tmp)
        PREV_FREE(tmp) = bp;
}

void delete_free_list(void *bp) {
    if ((char*)PREV_FREE(bp) > (char*)heap_listp && (char*)PREV_FREE(bp) < (char*)heap_listp+9*WSIZE)
        *(unsigned int*)PREV_FREE(bp) = NEXT_FREE(bp);
    else
        NEXT_FREE(PREV_FREE(bp)) = NEXT_FREE(bp);
    if (NEXT_FREE(bp))
        PREV_FREE(NEXT_FREE(bp)) = PREV_FREE(bp);
}

void *find_fit(size_t size) {
    int group;
    if (size <= 64)
        group = 1;
    else if (size <= 128)
        group = 2;
    else if (size <= 256)
        group = 3;
    else if (size <= 512)
        group = 4;
    else if (size <= 1024)
        group = 5;
    else if (size <= 2048)
        group = 6;
    else if (size <= 4096)
        group = 7;
    else
        group = 8;
    for (; group <= 8; group++) {
        unsigned int *free_list = *(unsigned int*)((char*)(heap_listp) + (group*WSIZE));
        while (free_list) {
            if (GET_SIZE(HDRP(free_list)) - DSIZE >= size) {
                return (void*)free_list;
            } else {
                free_list = NEXT_FREE(free_list);
            }
        }
    }
    return NULL;
}

void *place(void *bp, size_t size) {
    size_t total_size = GET_SIZE(HDRP(bp));
    delete_free_list(bp);
    if (total_size >= size + 3*DSIZE) {
        if (size < 96) {
            // 根据binary-bal和binary2-bal的内存访问模式做特殊优化，将较小的块放在前边，较大的块放在后面，可以减少释放内存后的外部碎片
            PUT(HDRP(bp), PACK(size + DSIZE, 1));
            PUT(FTRP(bp), PACK(size + DSIZE, 1));
            size_t pure_size = total_size - size - DSIZE - DSIZE;
            PUT(HDRP(NEXT_BLKP(bp)), PACK(pure_size + DSIZE, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(pure_size + DSIZE, 0));
            add_free_list(NEXT_BLKP(bp), pure_size);
            return bp;
        } else {
            PUT(HDRP(bp), PACK(total_size - size - DSIZE, 0));
            PUT(FTRP(bp), PACK(total_size - size - DSIZE, 0));
            add_free_list(bp, total_size - size - 2*DSIZE);
            PUT(HDRP(NEXT_BLKP(bp)), PACK(size + DSIZE, 1));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size + DSIZE, 1));
            return NEXT_BLKP(bp);
        }
    } else {
        PUT(HDRP(bp), PACK(total_size, 1));
        PUT(FTRP(bp), PACK(total_size, 1));
        return bp;
    }
}
