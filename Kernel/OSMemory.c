/**/
#include "TINIUX.h"
#include "OSMemory.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The heap is made up as a list of structs of this type.
 * This does not have to be aligned since for getting its size,
 * we only use the macro SIZEOF_OSMEM_ALIGNED, which automatically alignes.*/
//记录该内存块信息的节点。
typedef struct _tOSMem 
{
  uOSMemSize_t NextMem;    /** index (-> gpOSMemBegin[NextMem]) of the next struct */
  							//在系统内存中，下一个tOSMem_t的位置
  uOSMemSize_t PrevMem;    /** index (-> gpOSMemBegin[PrevMem]) of the previous struct */
							//在系统内存中，上一个tOSMem_t的位置
  uOS8_t Used;             /** 1: this memory block is Used; 0: this memory block is unused */
							//该tOSMem_t对应的内存块是否被分配
}tOSMem_t;

/** All allocated blocks will be MIN_SIZE bytes big, at least!
 * MIN_SIZE can be overridden to suit your needs. Smaller values save space,
 * larger values could prevent too small blocks to fragment the RAM too much. */
#ifndef MIN_SIZE
#define MIN_SIZE                  ( 16 )
#endif /* MIN_SIZE */
/** some alignment macros: we define them here for better source code layout */
#define OSMIN_SIZE_ALIGNED        OSMEM_ALIGN_SIZE(MIN_SIZE)
#define SIZEOF_OSMEM_ALIGNED      OSMEM_ALIGN_SIZE(sizeof(tOSMem_t))
#define OSMEM_SIZE_ALIGNED        OSMEM_ALIGN_SIZE(OSMEM_SIZE)

/** If you want to relocate the heap to external memory, simply define
 * OSRAM_HEAP_POINTER as a void-pointer to that location.
 * If so, make sure the memory at that location is big enough (see below on
 * how that space is calculated). */
/*
 * 系统所需的内存指针，该指针可由用户自行配置，指向具体的内存区。
 * 如若未配置，则通过数组的形式进行分配。
 * 数组的长度=用户指定的长度+内存信息表的长度*2+内存对齐最小长度；
 * 上述长度数值均是按照内存对齐方式处理过之后的长度数值；
 * 从上面的宏定义我们可以看出，若用户没有配置OSRAM_HEAP_POINTER的具体数值，
 * 系统则会把该变量指向数组OSRamHeap的首地址；

 * 两个tOSMem_t分别是gpOSMemEnd和gpOSMemLFree。
*/
#ifndef OSRAM_HEAP_POINTER
/** the heap. we need one tOSMem_t at the end and some room for alignment */
//	其中一个SIZEOF_OSMEM_ALIGNED是给gpOSMemEnd准备的。
uOS8_t OSRamHeap[OSMEM_SIZE_ALIGNED + (2U*SIZEOF_OSMEM_ALIGNED) + OSMEM_ALIGNMENT];
#define OSRAM_HEAP_POINTER OSRamHeap
#endif /* OSRAM_HEAP_POINTER */

/** pointer to the heap (OSRamHeap): for alignment, gpOSMemBegin is now a pointer instead of an array */
static uOS8_t *gpOSMemBegin = OS_NULL;
/** the last entry, always unused! */
static tOSMem_t *gpOSMemEnd = OS_NULL;
/** pointer to the lowest free block, this is Used for faster search */
static tOSMem_t *gpOSMemLFree = OS_NULL;

/***************************************************************************** 
Function    : OSMemCombine 
Description : "OSMemCombine" by combining adjacent empty struct mems.
              After this function is through, there should not exist
              one empty tOSMem_t pointing to another empty tOSMem_t.
              this function is only called by OSMemFree() and OSMemTrim(),
              This assumes access to the heap is protected by the calling function
              already.
Input       : ptOSMem -- the point to a tOSMem_t which just has been freed.
Output      : None 
Return      : None 
*****************************************************************************/
#if ( OS_MEMFREE_ON != 0 )
//	对内存碎块进行合并整理。
static void OSMemCombine(tOSMem_t *ptOSMem)
{
//	相邻内存块中高地址内存块
    tOSMem_t *ptNextOSMem = OS_NULL;
//	相邻内存块中低地址内存块
    tOSMem_t *ptPrevOSMem = OS_NULL;
//	若待融合的内存块正在使用，则返回。
    if ( ptOSMem->Used==1 )
    {
        return;
    }

//	与相邻的高地址的内存块合并
    ptNextOSMem = (tOSMem_t *)(void *)&gpOSMemBegin[ptOSMem->NextMem];
//	合并的内存块地址不能一样
//	高地址内存块不能被分配
//	高地址内存块不能是END块
    if (ptOSMem != ptNextOSMem && ptNextOSMem->Used == 0 && (uOS8_t *)ptNextOSMem != (uOS8_t *)gpOSMemEnd) 
    {
	//	更新Free指针
        if (gpOSMemLFree == ptNextOSMem) 
        {
            gpOSMemLFree = ptOSMem;
        }
	//	更新节点
        ptOSMem->NextMem = ptNextOSMem->NextMem;
        ((tOSMem_t *)(void *)&gpOSMemBegin[ptNextOSMem->NextMem])->PrevMem = (uOSMemSize_t)((uOS8_t *)ptOSMem - gpOSMemBegin);
    }

//	与相邻的低地址的内存块合并
    ptPrevOSMem = (tOSMem_t *)(void *)&gpOSMemBegin[ptOSMem->PrevMem];
//	合并的内存块地址不能一样
//	低地址内存块不能被分配
    if (ptPrevOSMem != ptOSMem && ptPrevOSMem->Used == 0) 
    {
	//	更新Free指针
        if (gpOSMemLFree == ptOSMem) 
        {
            gpOSMemLFree = ptPrevOSMem;
        }
	//	更新节点
        ptPrevOSMem->NextMem = ptOSMem->NextMem;
        ((tOSMem_t *)(void *)&gpOSMemBegin[ptOSMem->NextMem])->PrevMem = (uOSMemSize_t)((uOS8_t *)ptPrevOSMem - gpOSMemBegin);
    }
    return;
}
#endif /* OS_MEMFREE_ON */

/***************************************************************************** 
Function    : OSMemInit 
Description : 初始化堆，gpOSMemBegin、gpOSMemEnd以及gpOSMemLFree
Input       : None
Output      : None 
Return      : None 
*****************************************************************************/
//详见网址：https://blog.csdn.net/itworld123/article/details/83998064
uOSBase_t OSMemInit(void)
{
    tOSMem_t *ptOSMemTemp = OS_NULL;

//	对齐内存分区首地址
    gpOSMemBegin = (uOS8_t *)OSMEM_ALIGN_ADDR(OSRAM_HEAP_POINTER);

    /* Initialize the stack tiniux used. */
//	初始化内存分区
    memset(gpOSMemBegin, 0U, OSMEM_SIZE_ALIGNED);

    // initialize the start of the heap 
//	初始化内存分区开始的节点
    ptOSMemTemp = (tOSMem_t *)(void *)gpOSMemBegin;
//	指向首节点，指向末节点，方法是以内存分区开始为0的位置，按照距离进行偏移找到END节点的。
    ptOSMemTemp->NextMem = OSMEM_SIZE_ALIGNED;
    ptOSMemTemp->PrevMem = 0;
    ptOSMemTemp->Used = 0;
    
    // initialize the end of the heap 
//	初始化内存分区末节点，NextMem 和 PrevMem 均指向自身。
    gpOSMemEnd = (tOSMem_t *)(void *)&gpOSMemBegin[OSMEM_SIZE_ALIGNED];
    gpOSMemEnd->Used = 1;
    gpOSMemEnd->NextMem = OSMEM_SIZE_ALIGNED;
    gpOSMemEnd->PrevMem = OSMEM_SIZE_ALIGNED;

    // initialize the lowest-free pointer to the start of the heap 
//	初始化Free指针，使其为堆开始位置。
    gpOSMemLFree = (tOSMem_t *)(void *)gpOSMemBegin;
    
    return 0U;
}

/***************************************************************************** 
Function    : OSMemFree 
Description : Put a tOSMem_t back on the heap. 
Input       : pMem -- the data portion of a tOSMem_t as returned by a previous 
                      call to OSMemMalloc()
Output      : None 
Return      : None 
*****************************************************************************/ 
#if ( OS_MEMFREE_ON != 0 )
void OSMemFree(void *pMem)
{
    tOSMem_t *ptOSMemTemp = OS_NULL;
//	待释放的地址为空则返回。
    if (pMem == OS_NULL) 
    {
        return;
    }
//	待释放的地址不在堆栈中则返回。
    if ((uOS8_t *)pMem < (uOS8_t *)gpOSMemBegin || (uOS8_t *)pMem >= (uOS8_t *)gpOSMemEnd) 
    {
        return;
    }
    
	// protect the heap from concurrent access 
    OSIntLock();
//	得到该内存块对应的信息块（tOSMem_t）。
    ptOSMemTemp = (tOSMem_t *)(void *)((uOS8_t *)pMem - SIZEOF_OSMEM_ALIGNED);
    
//	该信息块为1，即：正在使用，才能释放该内存。
    if( ptOSMemTemp->Used==1 )
    {
   	//	释放！
        ptOSMemTemp->Used = 0;
	//	若该已经释放的内存块低于gpOSMemLFree，则更新Free指针。
        if (ptOSMemTemp < gpOSMemLFree) 
        {
            gpOSMemLFree = ptOSMemTemp;
        }

    //	释放完内存，为了避免内存碎片，要进行内存块整理。
        OSMemCombine(ptOSMemTemp);        
    }
    OSIntUnlock();
    
    return;
}
#endif /* OS_MEMFREE_ON */

/***************************************************************************** 
Function    : OSMemTrim 
Description : 对OSMemMalloc函数返回的内存块进行压缩.
Input       : pMem -- the pointer to memory allocated by OSMemMalloc is to be shrinked
              newsize -- required size after shrinking (needs to be smaller than or
                         equal to the previous size)
Output      : None 
Return      : for compatibility reasons: is always == pMem, at the moment
              or OS_NULL if newsize is > old size, in which case pMem is NOT touched
              or freed!
*****************************************************************************/ 
#if ( OS_MEMFREE_ON != 0 )
void* OSMemTrim(void *pMem, uOSMemSize_t newsize)
{
    uOSMemSize_t size = 0U;
    uOSMemSize_t ptr = 0U, ptr2 = 0U;
    tOSMem_t *ptOSMemTemp = OS_NULL, *ptOSMemTemp2 = OS_NULL;

//	确保压缩完的字节满足内存对齐原则
    newsize = OSMEM_ALIGN_SIZE(newsize);
//	压缩完的内存块至少要大于OSMIN_SIZE_ALIGNED
    if(newsize < OSMIN_SIZE_ALIGNED) 
    {
        newsize = OSMIN_SIZE_ALIGNED;
    }
//	新的内存块大小不能大于堆最大值
    if (newsize > OSMEM_SIZE_ALIGNED) 
    {
        return OS_NULL;
    }
//	待压缩的内存块的首地址若不在内存分区中，则返回原地址
    if ((uOS8_t *)pMem < (uOS8_t *)gpOSMemBegin || (uOS8_t *)pMem >= (uOS8_t *)gpOSMemEnd) 
    {
        return pMem;
    }
//	得到待压缩的内存块对应的信息块（tOSMem_t）
    ptOSMemTemp = (tOSMem_t *)(void *)((uOS8_t *)pMem - SIZEOF_OSMEM_ALIGNED);
//	得到该信息块在内存分区中的偏移地址
    ptr = (uOSMemSize_t)((uOS8_t *)ptOSMemTemp - gpOSMemBegin);
//	得到该内存块真实的大小
    size = ptOSMemTemp->NextMem - ptr - SIZEOF_OSMEM_ALIGNED;
//	若要求压缩完之后的内存块的大小    大于  原内存块的大小，则返回OS_NULL
    if (newsize > size) 
    {
        return OS_NULL;
    }
//	若要求压缩完之后的内存块的大小等于原内存块的大小，则返回原地址
    if (newsize == size) 
    {
        return pMem;
    }
//	程序执行到这里，说明pMem内存块的大小可以压缩到newsize。
	
    // protect the heap from concurrent access 
    OSIntLock();
//	pMem 下一个节点（tOSMem_t）的首地址
    ptOSMemTemp2 = (tOSMem_t *)(void *)&gpOSMemBegin[ptOSMemTemp->NextMem];
//	若pMem下一个节点是空闲的块，由于需要压缩pMem，故压缩后多余的内存块要和ptOSMemTemp2融合。
    if(ptOSMemTemp2->Used == 0) 
    {
        uOSMemSize_t NextMem;
    //	临时存储ptOSMemTemp2下一个节点的位置，因为pMem压缩完之后，
    //	ptOSMemTemp2会想低地址移动，移动完之后再将NextMem赋值给ptOSMemTemp2。
        NextMem = ptOSMemTemp2->NextMem;
    //	pMem 压缩完之后，得到新的pMem的下一个节点位置
        ptr2 = ptr + SIZEOF_OSMEM_ALIGNED + newsize;
	//	更新Free。因为，Free始终指向空闲块中首地址最低的块的首地址，
	//	若此时Free指向了比ptOSMemTemp2更低的地址，则不用管，若Free
	//	指针在高地址，则只能是等于ptOSMemTemp2了。
        if (gpOSMemLFree == ptOSMemTemp2) 
        {
            gpOSMemLFree = (tOSMem_t *)(void *)&gpOSMemBegin[ptr2];
        }
	//	更新ptOSMemTemp2节点。
        ptOSMemTemp2 = (tOSMem_t *)(void *)&gpOSMemBegin[ptr2];
        ptOSMemTemp2->Used = 0;
        ptOSMemTemp2->NextMem = NextMem;
        ptOSMemTemp2->PrevMem = ptr;
        ptOSMemTemp->NextMem = ptr2;
	//	最后一件事的更新链表，即：gpOSMemBegin[ptOSMemTemp2->NextMem])->PrevMem。
	//	前提是ptOSMemTemp2->NextMem不能是END节点
        if (ptOSMemTemp2->NextMem != OSMEM_SIZE_ALIGNED) 
        {
            ((tOSMem_t *)(void *)&gpOSMemBegin[ptOSMemTemp2->NextMem])->PrevMem = ptr2;
        }
        // no need to combine, we've already done that 
    } 
//	下一块已被使用
    else if (newsize + SIZEOF_OSMEM_ALIGNED + OSMIN_SIZE_ALIGNED <= size) 
    {
    //	压缩完之后剩下的块满足最小块。
        ptr2 = ptr + SIZEOF_OSMEM_ALIGNED + newsize;
        ptOSMemTemp2 = (tOSMem_t *)(void *)&gpOSMemBegin[ptr2];
        if (ptOSMemTemp2 < gpOSMemLFree) 
        {
            gpOSMemLFree = ptOSMemTemp2;
        }
        ptOSMemTemp2->Used = 0;
        ptOSMemTemp2->NextMem = ptOSMemTemp->NextMem;
        ptOSMemTemp2->PrevMem = ptr;
        ptOSMemTemp->NextMem = ptr2;
        if (ptOSMemTemp2->NextMem != OSMEM_SIZE_ALIGNED) 
        {
            ((tOSMem_t *)(void *)&gpOSMemBegin[ptOSMemTemp2->NextMem])->PrevMem = ptr2;
        }
    // 	由于原始的ptOSMemTemp->NextMem被使用，故不需要OSMemCombine 
    }
/*    else 
    {
		//pMem的大小若不满足newsize + SIZEOF_OSMEM_ALIGNED + OSMIN_SIZE_ALIGNED的话，
		//说明压缩完之后剩下的空闲块抛去SIZEOF_OSMEM_ALIGNED不足于建立最小的内存块，
		//不可压缩。故不做任何事，返回原地址。
    } 
*/
  OSIntUnlock();
  return pMem;
}
#endif /* OS_MEMFREE_ON */

/***************************************************************************** 
Function    : OSMemMalloc 
Description : Allocate a block of memory with a minimum of 'size' bytes.
			  以“size”为最小值，分配内存块。
Input       : size -- the minimum size of the requested block in bytes.
Output      : None 
Return      : pointer to allocated memory or OS_NULL if no free memory was found.
              the returned value will always be aligned (as defined by OSMEM_ALIGNMENT).
*****************************************************************************/ 
void* OSMemMalloc(uOSMemSize_t size)
{
//	变量初始化
	//返回最终的已分配的内存块的首地址（不包括tOSMem_t）
    uOS8_t * pResult = OS_NULL;
	//gpOSMemBegin的迭代器
    uOSMemSize_t ptr = 0U;
	//分配完内存块之后剩余的空闲块的在内存分区中的位置，也是迭代器
    uOSMemSize_t ptr2 = 0U;
    //开始时是内存分区的首地址，由于内存分区的分配是从低地址向高地址分配，分配完成之后，
    //该地址变成了要分配的内存块的首地址，但是包含tOSMem_t，去掉tOSMem_t之后就是pResult。
	tOSMem_t *ptOSMemTemp = OS_NULL;
	//分配完内存块剩余的空闲块的首地址
	tOSMem_t *ptOSMemTemp2 = OS_NULL;
//	初始化内存分区，详见：https://blog.csdn.net/itworld123/article/details/83998064
    if(gpOSMemEnd==OS_NULL)
    {
        OSMemInit();
        if(gpOSMemEnd==OS_NULL)
        {
            return pResult;
        }
    }
//	不需要分配则返回OS_NULL
    if (size == 0) 
    {
        return pResult;
    }
//  对齐要分配的内存块 
    size = OSMEM_ALIGN_SIZE(size);
//	分配的内存块至少要大于OSMIN_SIZE_ALIGNED
    if(size < OSMIN_SIZE_ALIGNED) 
    {
        // every data block must be at least OSMIN_SIZE_ALIGNED long 
        size = OSMIN_SIZE_ALIGNED;
    }
//	分配的内存块大小超过内存分区的最大值，则返回OS_NULL
    if (size > OSMEM_SIZE_ALIGNED) 
    {
        return pResult;
    }

    // protect the heap from concurrent access 
    OSIntLock();
    
//	2018-11-10
//	(uOSMemSize_t)((uOS8_t *)gpOSMemLFree - gpOSMemBegin)	确定闲置tOSMem_t首地址相对Begin的位置
//	OSMEM_SIZE_ALIGNED - size	搜索的范围不能超过待分配内存块的最高地址处
//	ptr = ((tOSMem_t *)(void *)&gpOSMemBegin[ptr])->NextMem	确定下一个tOSMem_t的位置 
//	总的来说，从当前gpOSMemLFree开始，找到第一个满足“size”大小的空闲的内存块。
    for (ptr = (uOSMemSize_t)((uOS8_t *)gpOSMemLFree - gpOSMemBegin); ptr < OSMEM_SIZE_ALIGNED - size;
        ptr = ((tOSMem_t *)(void *)&gpOSMemBegin[ptr])->NextMem) 
    {
    //	得到内存分区的首地址，即：第一个控制块的首地址。
        ptOSMemTemp = (tOSMem_t *)(void *)&gpOSMemBegin[ptr];

	//	1、ptOSMemTemp->Used	必须为0，即：该内存块必须是空闲块。
	//	2、ptOSMemTemp->NextMem - (ptr + SIZEOF_OSMEM_ALIGNED)) >= size	
	//	计算该空闲块的大小，其中不包括tOSMem_t，故减掉 SIZEOF_OSMEM_ALIGNED
        if ((!ptOSMemTemp->Used) && (ptOSMemTemp->NextMem - (ptr + SIZEOF_OSMEM_ALIGNED)) >= size) 
        { 
        
		//	分配内存的过程就是插入节点（tOSMem_t）的过程。
		//	ptOSMemTemp 				当前节点的首地址（tOSMem_t）
		//	ptOSMemTemp->NextMem		下一个节点的首地址（tOSMem_t）
		//	ptOSMemTemp->NextMem - (ptr + SIZEOF_OSMEM_ALIGNED) 当前节点对应的内存块到下一个内存块对应的tOSMem_t首地址之间的大小
		//	(size + SIZEOF_OSMEM_ALIGNED + OSMIN_SIZE_ALIGNED)	申请内存完内存之后，在申请的内存后面插入新的tOSMem_t，
		//				tOSMem_t和下一个节点之间至少要有一个OSMIN_SIZE_ALIGNED大小的内存块。
		//	如果满足上述要求，就插入一个节点，否则直接将该内存块设置为要申请的内存。
            if (ptOSMemTemp->NextMem - (ptr + SIZEOF_OSMEM_ALIGNED) >= (size + SIZEOF_OSMEM_ALIGNED + OSMIN_SIZE_ALIGNED)) 
            {                
                ptr2 = ptr + SIZEOF_OSMEM_ALIGNED + size;
            // 	创建 ptOSMemTemp2 信息块，该信息块对应的内存块大小至少为 OSMIN_SIZE_ALIGNED
                ptOSMemTemp2 = (tOSMem_t *)(void *)&gpOSMemBegin[ptr2];
                ptOSMemTemp2->Used = 0;
                ptOSMemTemp2->NextMem = ptOSMemTemp->NextMem;
                ptOSMemTemp2->PrevMem = ptr;

                ptOSMemTemp->NextMem = ptr2;
                ptOSMemTemp->Used = 1;
			//	更新节点
                if (ptOSMemTemp2->NextMem != OSMEM_SIZE_ALIGNED) 
                {
                    ((tOSMem_t *)(void *)&gpOSMemBegin[ptOSMemTemp2->NextMem])->PrevMem = ptr2;
                }
            } 
            else 
            {
                ptOSMemTemp->Used = 1;
            }

		//	循环第一次，此时ptOSMemTemp = gpOSMemLFree。
            if (ptOSMemTemp == gpOSMemLFree) 
            {
				//若gpOSMemLFree完成了内存分配，即：Used为1，则更新Free指针。
                while (gpOSMemLFree->Used && gpOSMemLFree != gpOSMemEnd) 
                {
                    gpOSMemLFree = (tOSMem_t *)(void *)&gpOSMemBegin[gpOSMemLFree->NextMem];
                }
            }
		//	返回tOSMem_t对应的的内存块的地址
            pResult = (uOS8_t *)ptOSMemTemp + SIZEOF_OSMEM_ALIGNED;
            break;
        }
    }

    OSIntUnlock();

    return pResult;
}

/***************************************************************************** 
Function    : OSMemCalloc 
Description : Contiguously allocates enough space for count objects that are size bytes
              of memory each and returns a pointer to the allocated memory.
              The allocated memory is filled with bytes of value zero.
Input       : count -- number of objects to allocate.
              size -- size of the objects to allocate.
Output      : None 
Return      : pointer to allocated memory / OS_NULL pointer if there is an error.
*****************************************************************************/ 
//Calloc和Malloc的关系是，前者调用后者并且若申请内存块成功则初始化buffer。
//详见：https://blog.csdn.net/itworld123/article/details/79187109
void* OSMemCalloc(uOSMemSize_t count, uOSMemSize_t size)
{
    void *pMem = OS_NULL;

    // allocate 'count' objects of size 'size' 
    pMem = OSMemMalloc(count * size);
    if (pMem) 
    {
        // zero the memory 
        memset(pMem, 0, count * size);
    }
    return pMem;
}

#ifdef __cplusplus
}
#endif
