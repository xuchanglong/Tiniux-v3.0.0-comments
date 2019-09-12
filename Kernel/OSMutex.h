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

#ifndef __OS_MUTEX_H_
#define __OS_MUTEX_H_

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ( OS_MUTEX_ON!=0 )

typedef struct tOSMutex
{
    char                        pcMutexName[ OSNAME_MAX_LEN ];
    sOS8_t *                    MutexHolderHandle;
    
    tOSList_t                   tTaskListEventMutexV;   // Mutex Unlock TaskList;
    tOSList_t                   tTaskListEventMutexP;   // Mutex Lock TaskList;

    volatile uOSBase_t          uxCurNum;    
    uOSBase_t                   uxMaxNum;
    
    uOSBase_t                   uxMutexLocked;          // Mutex lock count
    
    volatile sOSBase_t          xMutexPLock;            // Record the number of task which lock from the mutex while it was locked.
    volatile sOSBase_t          xMutexVLock;            // Record the number of task which unlock to the mutex while it was locked.

    sOSBase_t                   xID;
} tOSMutex_t;

typedef    tOSMutex_t*          OSMutexHandle_t;

OSMutexHandle_t   OSMutexCreate( void ) TINIUX_FUNCTION;
#if ( OS_MEMFREE_ON != 0 )
void              OSMutexDelete( OSMutexHandle_t MutexHandle ) TINIUX_FUNCTION;
#endif /* OS_MEMFREE_ON */

sOSBase_t         OSMutexSetID(OSMutexHandle_t MutexHandle, sOSBase_t xID) TINIUX_FUNCTION;
sOSBase_t         OSMutexGetID(OSMutexHandle_t const MutexHandle) TINIUX_FUNCTION;

uOSBool_t         OSMutexLock( OSMutexHandle_t MutexHandle, uOSTick_t uxTicksToWait) TINIUX_FUNCTION;
uOSBool_t         OSMutexUnlock( OSMutexHandle_t MutexHandle) TINIUX_FUNCTION;

#endif //( OS_MUTEX_ON!=0 )

#ifdef __cplusplus
}
#endif

#endif //__OS_MUTEX_H_
