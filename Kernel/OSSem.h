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

#ifndef __OS_SEMAPHORE_H_
#define __OS_SEMAPHORE_H_

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ( OS_SEMAPHORE_ON!=0 )

//	参考链接：https://blog.csdn.net/sinat_25771299/article/details/50761593
//	Post	发送信号量，信号量+1
//	Pend	请求信号量，信号量-1，若减一之前信号量为0，则堵塞
typedef struct tOSSem
{
//	信号量的名字
    char                        pcSemName[ OSNAME_MAX_LEN ];
//	正在使用信号量的任务链表
    tOSList_t                   tTaskListEventSemV;   //Semaphore Post TaskList;
//  等待使用信号量的任务链表（堵塞）
    tOSList_t                   tTaskListEventSemP;   //Semaphore Pend TaskList;
//	当前信号量的值
    volatile uOSBase_t          uxCurNum;    
//	信号量最大数值
	uOSBase_t                   uxMaxNum;

    volatile sOSBase_t          xSemPLock;            // Record the number of task which pend from the semaphore while it was locked.
    volatile sOSBase_t          xSemVLock;            // Record the number of task which post to the semaphore while it was locked.

    sOSBase_t                   xID;
} tOSSem_t;

typedef tOSSem_t* OSSemHandle_t;
//	若uxInitialCount==0，该信号量为互斥信号量；
//	若uxInitialCount>1，该信号量为普通信号量。
OSSemHandle_t     OSSemCreate( const uOSBase_t uxInitialCount ) TINIUX_FUNCTION;
OSSemHandle_t     OSSemCreateCount( const uOSBase_t uxMaxNum, const uOSBase_t uxInitialCount ) TINIUX_FUNCTION;

#if ( OS_MEMFREE_ON != 0 )
void              OSSemDelete(OSSemHandle_t SemHandle) TINIUX_FUNCTION;
#endif /* OS_MEMFREE_ON */

sOSBase_t         OSSemSetID(OSSemHandle_t SemHandle, sOSBase_t xID) TINIUX_FUNCTION;
sOSBase_t         OSSemGetID(OSSemHandle_t const SemHandle) TINIUX_FUNCTION;

uOSBool_t         OSSemPend( OSSemHandle_t SemHandle, uOSTick_t uxTicksToWait) TINIUX_FUNCTION;

uOSBool_t         OSSemPost( OSSemHandle_t SemHandle) TINIUX_FUNCTION;
uOSBool_t         OSSemPostFromISR( OSSemHandle_t SemHandle ) TINIUX_FUNCTION;

#endif //(OS_SEMAPHORE_ON!=0)

#ifdef __cplusplus
}
#endif

#endif //__OS_SEMAPHORE_H_
