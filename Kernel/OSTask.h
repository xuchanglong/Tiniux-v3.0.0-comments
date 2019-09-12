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

#ifndef __OS_TASK_H_
#define __OS_TASK_H_

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
//	运行态
    eTaskStateRuning = 0,
//	就绪态
    eTaskStateReady     ,
//	挂起态
    eTaskStateSuspended ,
//	堵塞态
    eTaskStateBlocked   ,
    eTaskStateRecycle   ,
    eTaskStateNum
}eOSTaskState_t;

/*
 * Task control block.  A task control block (TCB) is allocated for each task,
 * and stores task state information, including a pointer to the task's context
 * (the task's run time environment, including register values)
 */
typedef struct OSTaskControlBlock
{
//	任务堆栈首地址
    volatile uOSStack_t*    puxTopOfStack;        /*< Points to the location of the last item placed on the task stack. THIS MUST BE THE FIRST MEMBER OF THE TCB STRUCT. */

    tOSListItem_t           tTaskListItem;        /*< Used to reference a task from an Ready/Timer/Suspended/Recycle list. */
    tOSListItem_t           tEventListItem;       /*< Used to reference a task from an PendingReady/Event list. */
//	任务的优先级
	uOSBase_t               uxPriority;           /*< The priority of the task.  0 is the lowest priority. */
//	堆栈的起始地址
	uOSStack_t*             puxStartStack;        /*< Points to the start of the stack. */
//	任务名称
	char                    pcTaskName[ OSNAME_MAX_LEN ];

#if ( OSSTACK_GROWTH > 0 )
    uOSBase_t*              puxEndOfStack;        /*< Points to the end of the stack on architectures where the stack grows up from low memory. */
#endif

#if ( OS_MUTEX_ON!=0 )
    uOSBase_t               uxBasePriority;       /*< The priority last assigned to the task - used by the priority inheritance mechanism. */
    uOSBase_t               uxMutexHoldNum;
#endif

    sOSBase_t               xID;
    
#if ( OS_TASK_SIGNAL_ON!=0 )                      /*< Task signal: SEM_SIG MSG_SIG MSG_SIG_OVERWRITE. */
    volatile uOS8_t         ucSigState;           /*< Task signal state: NotWaiting Waiting GotSignal. */
    volatile uOS32_t        uiSigValue;           /*< Task signal value: Msg or count. */
#endif

} tOSTCB_t;

typedef    tOSTCB_t*        OSTaskHandle_t;

uOSBase_t    OSTaskInit( void ) TINIUX_FUNCTION;
OSTaskHandle_t OSTaskCreate(OSTaskFunction_t    pxTaskFunction,
                            void*               pvParameter,
                            const uOS16_t       usStackDepth,
                            uOSBase_t           uxPriority,
                            sOS8_t*             pcTaskName) TINIUX_FUNCTION;
#if ( OS_MEMFREE_ON != 0 )
void         OSTaskDelete( OSTaskHandle_t xTaskToDelete ) TINIUX_FUNCTION;
#endif /* OS_MEMFREE_ON */
void         OSTaskSleep( const uOSTick_t uxTicksToSleep ) TINIUX_FUNCTION;
sOSBase_t    OSTaskSetID(OSTaskHandle_t TaskHandle, sOSBase_t xID) TINIUX_FUNCTION;
sOSBase_t    OSTaskGetID(OSTaskHandle_t const TaskHandle) TINIUX_FUNCTION;
uOSBase_t    OSTaskGetPriority( OSTaskHandle_t const TaskHandle ) TINIUX_FUNCTION;
uOSBase_t    OSTaskGetPriorityFromISR( OSTaskHandle_t const TaskHandle ) TINIUX_FUNCTION;

void         OSTaskListEventAdd( tOSList_t * const ptEventList, const uOSTick_t uxTicksToWait ) TINIUX_FUNCTION;
uOSBool_t    OSTaskListEventRemove( const tOSList_t * const ptEventList ) TINIUX_FUNCTION;
void         OSTaskListReadyAdd(tOSTCB_t* ptTCB) TINIUX_FUNCTION;
uOSBase_t    OSTaskListReadyNum( uOSBase_t uxPriority ) TINIUX_FUNCTION;
uOSBase_t    OSTaskListPendNum( void ) TINIUX_FUNCTION;
uOSBase_t    OSTaskListReadyPoolNum( void ) TINIUX_FUNCTION;
OSTaskHandle_t OSTaskListPendHeadItem( void ) TINIUX_FUNCTION;
OSTaskHandle_t OSTaskListReadyPoolHeadItem( void ) TINIUX_FUNCTION;

void         OSTaskListPendSwitch( void ) TINIUX_FUNCTION;
void         OSTaskSwitchContext( void ) TINIUX_FUNCTION;

#if (OSTIME_SLICE_ON != 0U)
uOSBool_t    OSTaskNeedTimeSlice( void ) TINIUX_FUNCTION;
#endif //(OSTIME_SLICE_ON != 0U)
void         OSTaskSuspend( OSTaskHandle_t TaskHandle ) TINIUX_FUNCTION;
void         OSTaskResume( OSTaskHandle_t TaskHandle ) TINIUX_FUNCTION;
sOSBase_t    OSTaskResumeFromISR( OSTaskHandle_t TaskHandle ) TINIUX_FUNCTION;

uOSBase_t    OSTaskGetCurrentTaskNum( void ) TINIUX_FUNCTION;
OSTaskHandle_t OSGetCurrentTaskHandle( void ) TINIUX_FUNCTION;
void         OSIdleTask( void *pvParameters) TINIUX_FUNCTION;

#if ( OS_MUTEX_ON!= 0 )
void *       OSTaskGetMutexHolder( void ) TINIUX_FUNCTION;
uOSBool_t    OSTaskPriorityInherit( OSTaskHandle_t const MutexHolderTaskHandle ) TINIUX_FUNCTION;
uOSBool_t    OSTaskPriorityDisinherit( OSTaskHandle_t const MutexHolderTaskHandle ) TINIUX_FUNCTION;
void         OSTaskPriorityDisinheritAfterTimeout( OSTaskHandle_t const MutexHolderTaskHandle, uOSBase_t uxHighestPriorityWaitingTask ) TINIUX_FUNCTION;
#endif /* OS_MUTEX_ON */

#if ( OS_TIMER_ON != 0 )
void         OSTaskBlockAndPend( tOSList_t * const ptEventList, uOSTick_t uxTicksToWait, uOSBool_t bNeedSuspend ) TINIUX_FUNCTION;
#endif /* ( OS_TIMER_ON!=0 ) */

#if ( OS_TASK_SIGNAL_ON!=0 )
uOSBool_t    OSTaskSignalWait( uOSTick_t const uxTicksToWait) TINIUX_FUNCTION;
uOSBool_t    OSTaskSignalEmit( OSTaskHandle_t const TaskHandle ) TINIUX_FUNCTION;
uOSBool_t    OSTaskSignalEmitFromISR( OSTaskHandle_t const TaskHandle ) TINIUX_FUNCTION;
uOSBool_t    OSTaskSignalWaitMsg( uOS32_t* puiSigValue, uOSTick_t const uxTicksToWait) TINIUX_FUNCTION;
uOSBool_t    OSTaskSignalEmitMsg( OSTaskHandle_t const TaskHandle, uOS32_t const uiSigValue, uOSBool_t bOverWrite ) TINIUX_FUNCTION;
uOSBool_t    OSTaskSignalEmitMsgFromISR( OSTaskHandle_t const TaskHandle, uOS32_t const uiSigValue, uOSBool_t bOverWrite ) TINIUX_FUNCTION;
uOSBool_t    OSTaskSignalClear( OSTaskHandle_t const TaskHandle ) TINIUX_FUNCTION;
#endif

#ifdef __cplusplus
}
#endif

#endif //__OS_TASK_H_
