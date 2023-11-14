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
    "7team",
    /* First member's full name */
    "jinsangha",
    /* First member's email address */
    "slemnem@naver.com",
    /* Second member's full name (leave blank if none) */
    "dddd",
    /* Second member's email address (leave blank if none) */
    "aaaa"
};

/* Basic constants and macros*/
#define WSIZE 4         /* Word and header/footer size (bytes) */
#define DSIZE 8         /* Double word size (bytes) */
#define CHUNCKSIZE (1<<12)    /* Extend heap by this amount (bytes) */

#define MAX(x,y) (x > y ? x : y)

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) (size | alloc)

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p) = (val))

/* Read and write a word at address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block bp bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block bp bp, compute address of next and previous block */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))


int mm_init (void);
void *mm_malloc (size_t size);
static void *find_fit(size_t size);
static void place(void * bp, size_t size);
static void *extend_heap(size_t words);
void mm_free (void *bp);
static void *coalesece(void *bp);
void *mm_realloc(void *bp, size_t size);

/* 
 * mm_init - initialize the malloc package.
 */

int mm_init(void)
{
    char *heap_listp;
    /* create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*) -1)
        return -1;
    
    /* allign paddng */
    PUT(heap_listp, 0);
    /* prologue malloc */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    /* epilogue header */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));

    /* Extend the empty heap whith a free block of CUNKSIZE bytes */
    if (extend_heap(CHUNCKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char * bp;

    /*ifnore suprious requests*/
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reps */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);
    
    /* Serch the free list for a fit*/
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    /* No fit found, Get more memory and place the block*/
    extendsize = MAX(asize, CHUNCKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    
    place(bp, asize);

    return bp;
}

static void *find_fit(size_t asize)
{
    void *bp = mem_heap_lo() + 2 * WSIZE;
    while(GET_SIZE(HDRP(bp)) > 0)
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
            return bp;
        bp = NEXT_BLKP(bp);
    }
    return NULL;
}

/*
*  place - insert the new block of asize at bp
*/
static void place(void * bp, size_t asize)
{
    size_t bsize = GET_SIZE(HDRP(bp)); // get size

    if((bsize - asize) >= (2 * DSIZE)) // if space is big enough to devide (bigger than 16 bytes)
    {
        // place new block
        PUT(HDRP(bp),PACK(asize, 1));
        PUT(FTRP(bp),PACK(asize, 1));

        // rest of the space is divided
        PUT(HDRP(NEXT_BLKP(bp)),PACK((bsize - asize), 0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK((bsize - asize), 0));
    }
    else{
        // place new block
        PUT(HDRP(bp),PACK(bsize, 1));
        PUT(FTRP(bp),PACK(bsize, 1));
    }
}

/*
* extend heap - extend the heap when additional heap needed
*/
static void *extend_heap(size_t words)
{
    char *bp;

    /* Allocate an event number of words to maintain alignment*/
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    /* Initialize free block header/footer and epilogue header*/
    PUT(HDRP(bp),PACK(size, 0));
    PUT(FTRP(bp),PACK(size, 0));
    /* epilogue header */
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0, 1));

    /* Coalesce if thr previous block was free*/
    return coalesece(bp);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesece(bp);
}

/*
* coalesece - merging freed blocks
*/
static void *coalesece(void *bp)
{
    // check surrounding block is allocated
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    // this block's size
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) // both of blocks are allocated
    {
        return bp;
    }
    else if (prev_alloc && !next_alloc) // if next block is not allocated
    {
        // merge with next block
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));
    }
    else if (!prev_alloc && next_alloc) // if previous block is not allocated
    {
        // merge with previous block
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    else  // both of blocks are not allocated
    {
        // merge both of them
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) // if pointer is null, just do allocate
        return mm_malloc(size);

    if (size <= 0) // if size is 0, just free allocated
    {
        mm_free(ptr);
        return 0;
    }

    // allocate the size of new malloc
    void *newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    // get size for copy if new malloc is smaller just copy length of new malloc
    size_t copySize = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (size < copySize)
      copySize = size;

    // copy old malloc element to new malloc
    memcpy(newptr, ptr, copySize);

    // free old malloc
    mm_free(ptr);

    return newptr;
}