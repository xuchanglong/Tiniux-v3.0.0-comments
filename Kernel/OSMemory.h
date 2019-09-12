/**********************************************************************************************************
TINIUX - A tiny and efficient embedded real time operating system (RTOS)
Copyright (C) SenseRate.com All rights reserved.
http://www.tiniux.org -- Documentation, latest information, license and contact details.
http://www.tiniux.com -- Commercial support, development, porting, licensing and training services.
--------------------------------------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met: 
1. Redistributions of source code must retain the above copyright notice, this list of 
conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice, this list 
of conditions and the following disclaimer in the documentation and/or other materials 
provided with the distribution. 
3. Neither the name of the copyright holder nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior written 
permission. 
--------------------------------------------------------------------------------------------------------
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
--------------------------------------------------------------------------------------------------------
 Notice of Export Control Law 
--------------------------------------------------------------------------------------------------------
 TINIUX may be subject to applicable export control laws and regulations, which might 
 include those applicable to TINIUX of U.S. and the country in which you are located. 
 Import, export and usage of TINIUX in any manner by you shall be in compliance with such 
 applicable export control laws and regulations. 
***********************************************************************************************************/

#ifndef __OS_MEMORY_H_
#define __OS_MEMORY_H_

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OSMEM_SIZE            OSTOTAL_HEAP_SIZE

/** OSMEM_SIZE would have to be aligned, but using 64000 here instead of
 * 65535 leaves some room for alignment. */
//因为内存管理自身的相关操作也需要占用内存，这句话读起来好像有点拗口啊。
//在内存管理时，我们为内存块设置了若干个信息记录表（表的数量与内存块的
//数量是一一对应的），信息记录表用于记录该内存块的使用情况，例如该内存块
//的前一个相邻内存块位置，后一个相邻内存块位置，以及该内存块是否已经被
//分配使用等等。冗余的1535个位置即为内存块记录信息使用，若系统中分配的
//内存块较多，则会占用的较多，一般情况下，1535的大小足够内存信息记录表使用了。

#if OSMEM_SIZE > 64000L
typedef uOS32_t uOSMemSize_t;
#else
typedef uOS16_t uOSMemSize_t;
#endif /* OSMEM_SIZE > 64000 */

uOSBase_t  OSMemInit(void);
void *OSMemMalloc(uOSMemSize_t size);
void *OSMemCalloc(uOSMemSize_t count, uOSMemSize_t size);

//OS_MEMFREE_ON  是否允许系统释放内存
#if ( OS_MEMFREE_ON != 0 )
void *OSMemTrim(void *pMem, uOSMemSize_t size);
void  OSMemFree(void *pMem);
#endif /* OS_MEMFREE_ON */


/** Calculate memory size for an aligned buffer - returns the next highest
 * multiple of OSMEM_ALIGNMENT (e.g. OSMEM_ALIGN_SIZE(3) and
 * OSMEM_ALIGN_SIZE(4) will both yield 4 for OSMEM_ALIGNMENT == 4). */
 
//详见链接：https://blog.csdn.net/itworld123/article/details/83902675
#ifndef OSMEM_ALIGN_SIZE
#define OSMEM_ALIGN_SIZE(size) (((size) + OSMEM_ALIGNMENT - 1) & ~(OSMEM_ALIGNMENT-1))
#endif

/** Calculate safe memory size for an aligned buffer when using an unaligned
 * type as storage. This includes a safety-margin on (OSMEM_ALIGNMENT - 1) at the
 * start (e.g. if buffer is UINT16[] and actual data will be UINT32*) */
#ifndef OSMEM_ALIGN_BUFFER
#define OSMEM_ALIGN_BUFFER(size) (((size) + OSMEM_ALIGNMENT - 1))
#endif

/** Align a memory pointer to the alignment defined by OSMEM_ALIGNMENT
 * so that ADDR % OSMEM_ALIGNMENT == 0 */
 
//对齐内存地址
#ifndef OSMEM_ALIGN_ADDR
#define OSMEM_ALIGN_ADDR(addr) ((void *)(((uOS32_t)(addr) + OSMEM_ALIGNMENT - 1) & ~(uOS32_t)(OSMEM_ALIGNMENT-1)))
#endif

#ifdef __cplusplus
}
#endif

#endif //__OS_MEMORY_H_
