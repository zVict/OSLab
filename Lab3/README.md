
## <center> 实验三：动态内存分配器的实现 </center>

* ##### 实验目的

  * 使用显式空闲链表实现一个32位系统堆内存分配器
  

  
* ##### 实验环境

   * 主机：Ubuntu 32位 14.04

   * Linux内核版本：2.6.26

     

* ##### 实验内容

  * 本实验基于CS:APP malloc实验，细节要求如下

      * 基于显式空闲链表实现块的分配和释放
  * 分配块没有脚部，空闲块有脚部
      * 必须实现块的合并与分裂
      * 空闲链表采用后进先出的排序策略
      * 适配方式采用首次适配
      
  * 实验代码

    主要实验代码书上已经给出，下面主要介绍新的宏定义以及mm_init，find_fit，place，coalesce函数，完整代码见附件
    
    * marco
    
      详细见注释
    
      ```c
      /* put data to foot same as PUT */
      #define PUTFT(p,val) (*(unsigned int *)(p) = (val))
      /* put data to head with prev-block status */
      #define PUTHD(p,val) (*(unsigned int *)(p) = (GET(p) & 0x2) | (val))
      
      /* set next block's head's signal */
      #define SET_ALLOC(bp)       (GET(HDRP(NEXT_BLKP(bp))) |= 0x2)			
      #define SET_UNALLOC(bp)		(GET(HDRP(NEXT_BLKP(bp))) &= ~0x2)
      
      /* get prev/succ of explicit list, same as GET */
      #define GET_PREV(p)      (GET(p))
      /* succ need pointer plus 1 */
      #define GET_SUCC(p)      (*((unsigned int *)p + 1))	
      
      #define PUT_PREV(p, val) (PUT(p, (unsigned int)val))
      #define PUT_SUCC(p, val) (*((unsigned int *)p + 1) = (unsigned int)(val))
      
      /* get second bit */
      #define GET_PREV_ALLOC(bp)	(GET(HDRP(bp)) & 0x2)
      ```
    
      
    
    * mm_init
    
      在堆起始填充块添加显式链表的根指针starter，同时为块结构保持一致，为填充块分配前驱为空，后继为空的指针，其余与隐式空闲链表相同
    
      ``` c
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
      ```
    
    * find_fit
    
       直接通过显式链表查找，采用首次适配的方式
    
       ```c
       static void *find_fit(size_t asize)
       {
           unsigned int bp = NULL;
       
           for (bp = GET_SUCC(starter); bp != 0; bp = GET_SUCC(bp))
               /* search explicit list */
           {
               if (asize <= GET_SIZE(HDRP(bp)))
                   break;
           }
       
           return (void *)bp;
       }
       ```
    
    * place
    
      分配块后块的状态变为满，需要从显式链表中移除，并将后块头部的次低位置为1（前块满），其余与隐式空闲链表相同
    
      ```c
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
      ```
    
    * coalesce
    
      空闲块合并时有四种情况（前满后满、前满后空、前空后满、前空后空），不同情况空闲块加入/移出显式链表的操作不同，其余与隐式空闲链表相同
    
      ``` c
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
      ```
    
    * 其他函数
    
      * mm_malloc
    
        因为去掉了分配块的脚部，分配时可以多分配4个字节
    
      * mm_free
    
         要将后块头部的次低位置为0（前块空）
    
      * chain2starter
    
         将新生成的空闲块接到starter
    
      * prev2succ
    
         将新生成的分配块的前驱和后继相接

* ##### 实验结果

   * ![结果]Lab3/pictures/2019-05-13 (2).png

      

* ##### 实验总结

  * 本次实验主要学习了动态内存分配器的原理，熟悉了常见的内存相关API，了解了三种堆内存管理的方式（隐式空闲链表、显式空闲链表、伙伴算法）。细节方面，学习了有：
    * 空闲链表的结构；
    * 块的结构；
    * 不同放置策略及优劣；
    * 空闲块的合并与分裂，防止碎片化；
    * 堆的扩充方式；
    * 内存对齐的必要性；
    * 常见的时间复杂度优化策略（Buddy，BST）
  * 主要反思有：
    * 复习C语言宏定义的规范，体会了在涉及指针操作时宏定义的方便维护与直观；
    * 注意指针与强制类型转换的使用规范

