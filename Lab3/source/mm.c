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

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x,y) ((x)>(y)?(x):(y))
#define PACK(size,alloc) ((size)|(alloc))

#define GET(p)     (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p) = (val))				//regular PUT

#define PUTFT(p,val) (*(unsigned int *)(p) = (val))				//put data to foot same as PUT
#define PUTHD(p,val) (*(unsigned int *)(p) = (GET(p) & 0x2) | (val))		//put data to head with prev-block status

#define SET_ALLOC(bp)       (GET(HDRP(NEXT_BLKP(bp))) |= 0x2)			//set next block's head's signal 
#define SET_UNALLOC(bp)		(GET(HDRP(NEXT_BLKP(bp))) &= ~0x2)

#define GET_PREV(p)      (GET(p))						//get prev/succ of explicit list, same as GET
#define GET_SUCC(p)      (*((unsigned int *)p + 1))				//succ need pointer plus 1
#define PUT_PREV(p, val) (PUT(p, (unsigned int)val))
#define PUT_SUCC(p, val) (*((unsigned int *)p + 1) = (unsigned int)(val))

#define GET_SIZE(p)  (GET(p) & ~0x7)						//cover lowest 3 bits
#define GET_ALLOC(p) (GET(p) & 0x1)						//get lowest bit

#define GET_PREV_ALLOC(bp)	(GET(HDRP(bp)) & 0x2)				//get second bit

#define HDRP(bp) ((char *)(bp) - WSIZE)						//get head/foot of block pointer
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))		//get bp of next/prev block
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

int mm_init(void);
static void *extend_heap(size_t words);
void mm_free(void *bp);
static void *coalesce(void *bp);
void *mm_malloc(size_t size);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

static char *heap_listp = 0;
static unsigned int *starter = 0;						//root of explicit list

void chain2starter(void* bp)
/* chain freed block to starter */
{
    unsigned int starter_has_succ = GET_SUCC(starter);
    PUT_PREV(bp, starter);
    PUT_SUCC(starter, bp);
    PUT_SUCC(bp, starter_has_succ);
    if(starter_has_succ)
        PUT_PREV(starter_has_succ, bp);
}

void prev2succ(void *bp)
/* chain prev and succ of freed block */
{
    unsigned int bp_has_succ, bp_has_prev;
    bp_has_succ = GET_SUCC(bp);
    bp_has_prev = GET_PREV(bp);
    PUT_SUCC(bp_has_prev, bp_has_succ);
    if(bp_has_succ)
        PUT_PREV(bp_has_succ, bp_has_prev);
}

static void *find_fit(size_t asize)
{
    unsigned int bp = NULL;

    for (bp = GET_SUCC(starter); bp != 0; bp = GET_SUCC(bp))
    {
        if (asize <= GET_SIZE(HDRP(bp)))
            break;
    }

    return (void *)bp;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    prev2succ(bp);

    if ((csize - asize) >= (2*DSIZE)) {
    /* block size bigger than size we need */
        PUTHD(HDRP(bp), PACK(asize, 1));
        PUTFT(FTRP(bp), PACK(asize, 1));
        SET_ALLOC(bp);
        bp = NEXT_BLKP(bp);
        PUTHD(HDRP(bp), PACK(csize-asize, 0));
        PUTFT(FTRP(bp), PACK(csize-asize, 0));
        SET_UNALLOC(bp);
        coalesce(bp);
    }
    else {
        PUTHD(HDRP(bp), PACK(csize, 1));
        PUTFT(FTRP(bp), PACK(csize, 1));
        SET_ALLOC(bp);
    }
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUTHD(HDRP(bp), PACK(size, 0));
    PUTFT(FTRP(bp), PACK(size, 0));
    PUTHD(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

int mm_init(void)
{
    void *bp;
    mem_init();

    if ((heap_listp = (char *)mem_sbrk(6*WSIZE)) == (char *)-1)
        return -1;

    starter = (unsigned int *)mem_heap_lo() + 1;
    PUT(heap_listp, 0);	                         
    PUT_PREV(starter, 0);
    PUT_SUCC(starter, 0);                        
    PUT(heap_listp + (3*WSIZE), PACK(DSIZE, 1)); 
    PUT(heap_listp + (4*WSIZE), PACK(DSIZE, 1)); 
    PUT(heap_listp + (5*WSIZE), PACK(0, 1));     
    heap_listp += (4*WSIZE);
    SET_ALLOC(heap_listp);

    bp = extend_heap(CHUNKSIZE/WSIZE);
    if (bp == NULL)
        return -1;
    return 0;
}


static void *coalesce(void *bp)
{
    void *prev_bp;
    void *next_bp = NEXT_BLKP(bp);
    size_t prev_alloc = GET_PREV_ALLOC(bp);
    size_t next_alloc = GET_ALLOC(HDRP(next_bp));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            
        chain2starter(bp);
    }

    else if (prev_alloc && !next_alloc) {      
        size += GET_SIZE(HDRP(next_bp));
        PUTHD(HDRP(bp), PACK(size, 0));
        PUTFT(FTRP(next_bp), PACK(size,0));
        prev2succ(next_bp);
        chain2starter(bp);
    }

    else if (!prev_alloc && next_alloc) {      
        prev_bp = PREV_BLKP(bp);
        size += GET_SIZE(HDRP(prev_bp));
        PUTFT(FTRP(bp), PACK(size, 0));
        PUTHD(HDRP(prev_bp), PACK(size, 0));
        bp = prev_bp;
        prev2succ(bp);
        chain2starter(bp);
    }

    else {                                     
        prev_bp = PREV_BLKP(bp);
        size += GET_SIZE(HDRP(prev_bp)) +
                GET_SIZE(FTRP(next_bp));
        PUTHD(HDRP(prev_bp), PACK(size, 0));
        PUTFT(FTRP(next_bp), PACK(size, 0));
        prev2succ(next_bp);
        prev2succ(prev_bp);
        bp = prev_bp;
        chain2starter(bp);
    }
    return bp;
}

void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUTHD(HDRP(ptr), PACK(size, 0));
    PUTFT(FTRP(ptr), PACK(size, 0));
    SET_UNALLOC(ptr);
    coalesce(ptr);
}

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE + WSIZE)							//size enlarge to 12 without foot
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (WSIZE) + (DSIZE-1)) / DSIZE);			

    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}
