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

#include <string.h>
#include "TINIUX.h"

#ifdef __cplusplus
extern "C" {
#endif

TINIUX_DATA tOSTCB_t * volatile gptCurrentTCB                       = OS_NULL;
TINIUX_DATA volatile  uOSBase_t guxCurrentTaskNum                   = ( uOSBase_t ) 0U;

/* Lists for ready and blocked tasks. --------------------*/
TINIUX_DATA static tOSList_t gtOSTaskListReady[ OSHIGHEAST_PRIORITY ];
TINIUX_DATA static tOSList_t gtOSTaskListReadyPool;
TINIUX_DATA static tOSList_t gptOSTaskListSuspended;
TINIUX_DATA static tOSList_t gtOSTaskListPend1;
TINIUX_DATA static tOSList_t gtOSTaskListPend2;
TINIUX_DATA static tOSList_t * volatile gptOSTaskListPend           = OS_NULL;
TINIUX_DATA static tOSList_t * volatile gptOSTaskListLongPeriodPend = OS_NULL;

// delete task
#if ( OS_MEMFREE_ON != 0 )
TINIUX_DATA static tOSList_t gptOSTaskListRecycle;
TINIUX_DATA static volatile  uOSBase_t guxTasksDeleted              = ( uOSBase_t ) 0U;
#endif /* OS_MEMFREE_ON */

#if ( OS_TASK_SIGNAL_ON!=0 )
TINIUX_DATA static uOS8_t const SIG_STATE_NOTWAITING                = ( ( uOS8_t ) 0 );
TINIUX_DATA static uOS8_t const SIG_STATE_WAITING                   = ( ( uOS8_t ) 1 );
TINIUX_DATA static uOS8_t const SIG_STATE_RECEIVED                  = ( ( uOS8_t ) 2 );
#endif

static void OSTaskListInit( void )
{
    uOSBase_t uxPriority = ( uOSBase_t ) 0U;

    for( uxPriority = ( uOSBase_t ) 0U; uxPriority < ( uOSBase_t ) OSHIGHEAST_PRIORITY; uxPriority++ )
    {
        OSListInit( &( gtOSTaskListReady[ uxPriority ] ) );
    }

    OSListInit( &gtOSTaskListPend1 );
    OSListInit( &gtOSTaskListPend2 );
    OSListInit( &gtOSTaskListReadyPool );
#if ( OS_MEMFREE_ON != 0 )
    OSListInit( &gptOSTaskListRecycle );
#endif /* OS_MEMFREE_ON */
    OSListInit( &gptOSTaskListSuspended );

    gptOSTaskListPend = &gtOSTaskListPend1;
    gptOSTaskListLongPeriodPend = &gtOSTaskListPend2;
}

uOSBase_t OSTaskInit( void )
{
    gptCurrentTCB               = OS_NULL;
    gptOSTaskListPend           = OS_NULL;
    gptOSTaskListLongPeriodPend = OS_NULL;
    guxCurrentTaskNum     = ( uOSBase_t ) 0U;

#if ( OS_MEMFREE_ON != 0 )
    guxTasksDeleted             = ( uOSBase_t ) 0U;
#endif /* OS_MEMFREE_ON */

    OSTaskListInit();

    return 0U;
}

static void* OSTaskGetTCBFromHandle(OSTaskHandle_t const pxHandle)
{
    return ( ( ( pxHandle ) == OS_NULL ) ? ( tOSTCB_t * ) gptCurrentTCB : ( tOSTCB_t * ) ( pxHandle ) );
}

static void OSTaskSelectToSchedule()
{
    uOSBase_t uxTopPriority = ( uOSBase_t ) 0U;

    /* Find the highest priority queue that contains ready tasks. */
    uxTopPriority = OSGetTopReadyPriority();
    OSListGetNextItemHolder( &( gtOSTaskListReady[ uxTopPriority ] ), gptCurrentTCB );
}

void OSTaskListReadyAdd(tOSTCB_t* ptTCB)
{
    OSSetReadyPriority( ( ptTCB )->uxPriority );
    OSListInsertItemToEnd( &( gtOSTaskListReady[ ( ptTCB )->uxPriority ] ), &( ( ptTCB )->tTaskListItem ) );
}

static OSTaskHandle_t OSAllocateTCBAndStack( const uOS16_t usStackDepth, uOSStack_t *puxStackBuffer )
{
    OSTaskHandle_t ptNewTCB;
    ptNewTCB = ( OSTaskHandle_t ) OSMemMalloc( sizeof( tOSTCB_t ) );
    
    /* Just to avoid compiler warnings. */
    ( void ) puxStackBuffer;
    
    if( ptNewTCB != OS_NULL )
    {
        ptNewTCB->puxStartStack = ( uOSStack_t * )OSMemMalloc( ( ( uOS16_t )usStackDepth ) * sizeof( uOSStack_t ));

        if( ptNewTCB->puxStartStack == OS_NULL )
        {
            #if ( OS_MEMFREE_ON != 0 )
            OSMemFree( ptNewTCB );
            #endif /* OS_MEMFREE_ON */
            
            ptNewTCB = OS_NULL;
        }
        else
        {
            memset( (void*)ptNewTCB->puxStartStack, ( uOS8_t ) 0xA1U, ( uOS32_t ) usStackDepth * sizeof( uOSStack_t ) );
        }
    }

    return ptNewTCB;
}

static void OSTaskInitTCB( tOSTCB_t * const ptTCB, const char * const pcName, uOSBase_t uxPriority, const uOS16_t usStackDepth )
{
    uOSBase_t x = ( uOSBase_t ) 0;

    // Store the task name in the TCB.
    for( x = ( uOSBase_t ) 0; x < ( uOSBase_t ) OSNAME_MAX_LEN; x++ )
    {
        ptTCB->pcTaskName[ x ] = pcName[ x ];
        if( pcName[ x ] == 0x00 )
        {
            break;
        }
    }
    ptTCB->pcTaskName[ OSNAME_MAX_LEN - 1 ] = '\0';

    if( uxPriority >= ( uOSBase_t ) OSHIGHEAST_PRIORITY )
    {
        uxPriority = ( uOSBase_t ) OSHIGHEAST_PRIORITY - 1;
    }

    ptTCB->uxPriority = uxPriority;

    #if ( OS_MUTEX_ON!= 0 )
    {
        ptTCB->uxBasePriority = uxPriority;
        ptTCB->uxMutexHoldNum = 0;
    }
    #endif // ( OS_MUTEX_ON!= 0 )
    
    #if ( OS_TASK_SIGNAL_ON!=0 )
    {
        ptTCB->ucSigState = SIG_STATE_NOTWAITING;        /*< Task signal state: NotWaiting Waiting Received. */
        ptTCB->uiSigValue = 0;                            /*< Task signal value: Msg or count. */        
    }
    #endif // OS_TASK_SIGNAL_ON!=0
        
    OSListItemInitialise( &( ptTCB->tTaskListItem ) );
    OSListItemInitialise( &( ptTCB->tEventListItem ) );

    OSListItemSetHolder( &( ptTCB->tTaskListItem ), ptTCB );

    /* Event lists are always in priority order. */
    OSListItemSetValue( &( ptTCB->tEventListItem ), ( uOSTick_t ) OSHIGHEAST_PRIORITY - ( uOSTick_t ) uxPriority );
    OSListItemSetHolder( &( ptTCB->tEventListItem ), ptTCB );

    ( void ) usStackDepth;
}

OSTaskHandle_t OSTaskCreate(OSTaskFunction_t    pxTaskFunction,
                            void*               pvParameter,
                            const uOS16_t       usStackDepth,
                            uOSBase_t           uxPriority,
                            sOS8_t*             pcTaskName)
{
    sOSBase_t xStatus = OS_FAIL;
    OSTaskHandle_t ptNewTCB = OS_NULL;
    uOSStack_t *puxTopOfStack = OS_NULL;

    ptNewTCB = (tOSTCB_t * )OSAllocateTCBAndStack( usStackDepth, OS_NULL );

    if( ptNewTCB != OS_NULL )
    {
        #if( OSSTACK_GROWTH < 0 )
        {
            puxTopOfStack = ptNewTCB->puxStartStack + ( usStackDepth - ( uOS16_t ) 1 );
            puxTopOfStack = ( uOSStack_t * ) ( ( ( uOS32_t ) puxTopOfStack ) & ( ~( ( uOS32_t ) OSMEM_ALIGNMENT_MASK ) ) );
        }
        #else
        {
            puxTopOfStack = ptNewTCB->puxStartStack;
            ptNewTCB->puxEndOfStack = ptNewTCB->puxStartStack + ( usStackDepth - 1 );
        }
        #endif

        OSTaskInitTCB( ptNewTCB, pcTaskName, uxPriority, usStackDepth );
        ptNewTCB->puxTopOfStack = FitInitializeStack( puxTopOfStack, pxTaskFunction, pvParameter);
        
        OSIntLock();
        {
            guxCurrentTaskNum++;
            if( gptCurrentTCB == OS_NULL )
            {
                gptCurrentTCB =  ptNewTCB;
            }
            else
            {
                if( OSScheduleGetState() == SCHEDULER_NOT_STARTED )
                {
                    if( gptCurrentTCB->uxPriority <= uxPriority )
                    {
                        gptCurrentTCB = ptNewTCB;
                    }
                }
            }

            OSTaskListReadyAdd( ptNewTCB );

            xStatus = OS_PASS;
        }
        OSIntUnlock();
    }
    else
    {
        xStatus = OS_FAIL;
    }

    if( xStatus == OS_PASS )
    {
        if( OSScheduleGetState() != SCHEDULER_NOT_STARTED )
        {
            if( gptCurrentTCB->uxPriority < uxPriority )
            {
                OSSchedule();
            }
        }
    }
    return ptNewTCB;
}

#if ( OS_MEMFREE_ON != 0 )
void OSTaskDelete( OSTaskHandle_t xTaskToDelete )
{
    tOSTCB_t *ptTCB = OS_NULL;

    OSIntLock();
    {
        ptTCB = OSTaskGetTCBFromHandle( xTaskToDelete );

        if( OSListRemoveItem( &( ptTCB->tTaskListItem ) ) == ( uOSBase_t ) 0 )
        {
            OSResetReadyPriority( ptTCB->uxPriority );
        }

        if( OSListItemGetList( &( ptTCB->tEventListItem ) ) != OS_NULL )
        {
            ( void ) OSListRemoveItem( &( ptTCB->tEventListItem ) );
        }

        if( ptTCB == gptCurrentTCB )
        {
            OSListInsertItemToEnd( &gptOSTaskListRecycle, &( ptTCB->tTaskListItem ) );
            ++guxTasksDeleted;
        }
        else
        {
            --guxCurrentTaskNum;
            OSMemFree(ptTCB->puxStartStack);
            OSUpdateUnblockTime();
        }        
    }
    OSIntUnlock();

    if( OSScheduleGetState() != SCHEDULER_NOT_STARTED )
    {
        if( ptTCB == gptCurrentTCB )
        {
            OSSchedule();
        }
    }
}
#endif /* OS_MEMFREE_ON */

sOSBase_t OSTaskSetID(OSTaskHandle_t TaskHandle, sOSBase_t xID)
{
    if(TaskHandle == OS_NULL)
    {
        return 1;
    }
    OSIntLock();
    {
        TaskHandle->xID = xID;
    }
    OSIntUnlock();

    return 0;
}

sOSBase_t OSTaskGetID(OSTaskHandle_t const TaskHandle)
{
    sOSBase_t xID = (sOSBase_t)0;
    
    OSIntLock();
    if(TaskHandle != OS_NULL)
    {
        xID = TaskHandle->xID;
    }
    OSIntUnlock();

    return xID;
}

#if ( OS_MEMFREE_ON != 0 )
static void OSTaskListRecycleRemove( void )
{
    tOSTCB_t *ptTCB = OS_NULL;
        
    while( guxTasksDeleted > ( uOSBase_t ) 0U )
    {
        OSIntLock();
        {
            ptTCB = ( tOSTCB_t * ) OSListGetHeadItemHolder( ( &gptOSTaskListRecycle ) );
            ( void ) OSListRemoveItem( &( ptTCB->tTaskListItem ) );
            --guxCurrentTaskNum;
            --guxTasksDeleted;
        }
        OSIntUnlock();

        OSMemFree(ptTCB->puxStartStack);
    }

}
#endif /* OS_MEMFREE_ON */

static void OSTaskListPendAdd(tOSTCB_t* ptTCB, const uOSTick_t uxTicksToWait, uOSBool_t bNeedSuspend )
{
    uOSTick_t uxTimeToWake = (uOSTick_t)0U;
    const uOSTick_t uxTickCount = OSGetTickCount();
    
    if(ptTCB == OS_NULL)
    {
        ptTCB = gptCurrentTCB;
    }
    
    if( OSListRemoveItem( &( ptTCB->tTaskListItem ) ) == ( uOSBase_t ) 0 )
    {
        OSResetReadyPriority(ptTCB->uxPriority);
    }

    if( (uxTicksToWait==OSPEND_FOREVER_VALUE) && (bNeedSuspend==OS_TRUE) )
    {
        OSListInsertItemToEnd( &gptOSTaskListSuspended, &( ptTCB->tTaskListItem ) );
    }
    else
    {
        uxTimeToWake = uxTickCount + uxTicksToWait;
        OSListItemSetValue( &( ptTCB->tTaskListItem ), uxTimeToWake );

        if( uxTimeToWake < uxTickCount )
        {
            OSListInsertItem( gptOSTaskListLongPeriodPend, &( ptTCB->tTaskListItem ) );
        }
        else
        {
            OSListInsertItem( gptOSTaskListPend, &( ptTCB->tTaskListItem ) );
            OSUpdateUnblockTime();
        }
    }    
}

void OSTaskListPendSwitch( void )
{
    tOSList_t *ptTempList = OS_NULL;

    ptTempList = gptOSTaskListPend;
    gptOSTaskListPend = gptOSTaskListLongPeriodPend;
    gptOSTaskListLongPeriodPend = ptTempList;
}

void OSTaskListEventAdd( tOSList_t * const ptEventList, const uOSTick_t uxTicksToWait )
{
    OSListInsertItem( ptEventList, &( gptCurrentTCB->tEventListItem ) );

    OSTaskListPendAdd( gptCurrentTCB, uxTicksToWait, OS_TRUE );
}

uOSBool_t OSTaskListEventRemove( const tOSList_t * const ptEventList )
{
    tOSTCB_t *pxUnblockedTCB = OS_NULL;
    uOSBool_t bReturn = OS_FALSE;

    pxUnblockedTCB = ( tOSTCB_t * ) OSListGetHeadItemHolder( ptEventList );

    ( void ) OSListRemoveItem( &( pxUnblockedTCB->tEventListItem ) );

    if( OSScheduleIsLocked() == OS_FALSE )
    {
        ( void ) OSListRemoveItem( &( pxUnblockedTCB->tTaskListItem ) );
        OSTaskListReadyAdd( pxUnblockedTCB );
    }
    else
    {
        OSListInsertItemToEnd( &( gtOSTaskListReadyPool ), &( pxUnblockedTCB->tEventListItem ) );
    }

    if( pxUnblockedTCB->uxPriority > gptCurrentTCB->uxPriority )
    {
        bReturn = OS_TRUE;
        OSNeedSchedule();
    }
    else
    {
        bReturn = OS_FALSE;
    }
    OSUpdateUnblockTime();
    
    return bReturn;
}

uOSBase_t OSTaskListPendNum( void )
{
    return OSListGetLength( gptOSTaskListPend );
}

OSTaskHandle_t OSTaskListPendHeadItem( void )
{
    return ( tOSTCB_t * ) OSListGetHeadItemHolder( gptOSTaskListPend );
}

uOSBase_t OSTaskListReadyPoolNum( void )
{
    return OSListGetLength( &gtOSTaskListReadyPool );
}

OSTaskHandle_t OSTaskListReadyPoolHeadItem( void )
{
    return ( tOSTCB_t * ) OSListGetHeadItemHolder( &gtOSTaskListReadyPool );
}

uOSBase_t OSTaskListReadyNum( uOSBase_t uxPriority )
{
    return OSListGetLength( &( gtOSTaskListReady[ ( uxPriority ) ] ) );
}

static void OSTaskCheckStackStatus()
{
    uOSStack_t* puxStackTemp = (uOSStack_t*)gptCurrentTCB->puxTopOfStack;

    #if( OSSTACK_GROWTH < 0 )
    if( puxStackTemp <= gptCurrentTCB->puxStartStack )
    {// Task stack overflow
        for( ; ; );
    }
    #else
    if( puxStackTemp >= gptCurrentTCB->puxEndOfStack )
    {// Task stack overflow
        for( ; ; );
    }
    #endif
}

void OSTaskSwitchContext( void )
{
    if( OSScheduleIsLocked() != OS_FALSE )
    {
        OSNeedSchedule();
    }
    else
    {
        OSNeedSchedule();

        OSTaskCheckStackStatus();
        OSTaskSelectToSchedule();
    }
}

void OSTaskSleep( const uOSTick_t uxTicksToSleep )
{
    uOSBool_t bAlreadyScheduled = OS_FALSE;

    if( uxTicksToSleep > ( uOSTick_t ) 0U )
    {
        OSScheduleLock();
        {
            OSTaskListPendAdd( gptCurrentTCB, uxTicksToSleep, OS_FALSE );
        }
        bAlreadyScheduled = OSScheduleUnlock();
    }

    if( bAlreadyScheduled == OS_FALSE )
    {
        OSSchedule();
    }
}

uOSBase_t OSTaskGetCurrentTaskNum( void )
{
    uOSBase_t uxReturn = (uOSBase_t)0U;

    OSIntLock();
    {
        uxReturn = guxCurrentTaskNum;
    }
    OSIntUnlock();

    return uxReturn;
}

OSTaskHandle_t OSGetCurrentTaskHandle( void )
{
    OSTaskHandle_t xReturn = OS_NULL;

    /* A critical section is not required as this is not called from
    an interrupt and the current TCB will always be the same for any
    individual execution thread. */
    xReturn = gptCurrentTCB;

    return xReturn;
}

eOSTaskState_t OSTaskGetState( OSTaskHandle_t TaskHandle )
{
    eOSTaskState_t eReturn = eTaskStateRuning;
    tOSList_t *ptStateList = OS_NULL;
    const tOSTCB_t * const ptTCB = ( tOSTCB_t * ) TaskHandle;

    if( ptTCB == gptCurrentTCB )
    {
        eReturn = eTaskStateRuning;
    }
    else
    {
        OSIntLock();
        {
            ptStateList = ( tOSList_t * ) OSListItemGetList( &( ptTCB->tTaskListItem ) );
        }
        OSIntUnlock();

        if( ( ptStateList == gptOSTaskListPend ) || ( ptStateList == gptOSTaskListLongPeriodPend ) )
        {
            eReturn = eTaskStateBlocked;
        }
        else if( ptStateList == &gptOSTaskListSuspended )
        {
            if( OSListItemGetList( &( ptTCB->tEventListItem ) ) == OS_NULL )
            {
                eReturn = eTaskStateSuspended;
            }
            else
            {
                eReturn = eTaskStateBlocked;
            }
        }
#if ( OS_MEMFREE_ON != 0 )
        else if( ptStateList == &gptOSTaskListRecycle )
        {
            eReturn = eTaskStateRecycle;
        }
#endif /* OS_MEMFREE_ON */
        else
        {
            eReturn = eTaskStateReady;
        }
    }

    return eReturn;
}

uOSBase_t OSTaskGetPriority( OSTaskHandle_t const TaskHandle )
{
    tOSTCB_t *ptTCB = OS_NULL;
    uOSBase_t uxReturn = (uOSBase_t)0U;

    OSIntLock();
    {
        /* If null is passed in here then it is the priority of the that
        called uxTaskPriorityGet() that is being queried. */
        ptTCB = OSTaskGetTCBFromHandle( TaskHandle );
        uxReturn = ptTCB->uxPriority;
    }
    OSIntUnlock();

    return uxReturn;
}

uOSBase_t OSTaskGetPriorityFromISR( OSTaskHandle_t const TaskHandle )
{
    tOSTCB_t *ptTCB = OS_NULL;
    uOSBase_t uxReturn = (uOSBase_t)0U;
    uOSBase_t uxIntSave = (uOSBase_t)0U;

    uxIntSave = OSIntMaskFromISR();
    {
        ptTCB = OSTaskGetTCBFromHandle( TaskHandle );
        uxReturn = ptTCB->uxPriority;
    }
    OSIntUnmaskFromISR( uxIntSave );

    return uxReturn;
}

void OSTaskSetPriority( OSTaskHandle_t TaskHandle, uOSBase_t uxNewPriority )
{
    tOSTCB_t *ptTCB = OS_NULL;
    uOSBase_t uxCurrentBasePriority = (uOSBase_t)0U;
    uOSBase_t uxPriorityUsedOnEntry = (uOSBase_t)0U;
    uOSBool_t bNeedSchedule = OS_FALSE;

    if( uxNewPriority >= ( uOSBase_t ) OSHIGHEAST_PRIORITY )
    {
        uxNewPriority = ( uOSBase_t ) OSHIGHEAST_PRIORITY - ( uOSBase_t )1U;
    }

    OSIntLock();
    {
        ptTCB = OSTaskGetTCBFromHandle( TaskHandle );

        #if ( OS_MUTEX_ON!= 0 )
        {
            uxCurrentBasePriority = ptTCB->uxBasePriority;
        }
        #else
        {
            uxCurrentBasePriority = ptTCB->uxPriority;
        }
        #endif

        if( uxCurrentBasePriority != uxNewPriority )
        {
            if( uxNewPriority > uxCurrentBasePriority )
            {
                if( ptTCB != gptCurrentTCB )
                {
                    if( uxNewPriority >= gptCurrentTCB->uxPriority )
                    {
                        bNeedSchedule = OS_TRUE;
                    }
                }
            }
            else if( ptTCB == gptCurrentTCB )
            {
                bNeedSchedule = OS_TRUE;
            }

            uxPriorityUsedOnEntry = ptTCB->uxPriority;

            #if ( OS_MUTEX_ON!= 0 )
            {
                if( ptTCB->uxBasePriority == ptTCB->uxPriority )
                {
                    ptTCB->uxPriority = uxNewPriority;
                }

                ptTCB->uxBasePriority = uxNewPriority;
            }
            #else
            {
                ptTCB->uxPriority = uxNewPriority;
            }
            #endif

            OSListItemSetValue( &( ptTCB->tEventListItem ), ( ( uOSTick_t ) OSHIGHEAST_PRIORITY - ( uOSTick_t ) uxNewPriority ) );

            if( OSListContainListItem( &( gtOSTaskListReady[ uxPriorityUsedOnEntry ] ), &( ptTCB->tTaskListItem ) ) != OS_FALSE )
            {
                if( OSListRemoveItem( &( ptTCB->tTaskListItem ) ) == ( uOSBase_t ) 0U )
                {
                    OSResetReadyPriority( uxPriorityUsedOnEntry );
                }

                OSTaskListReadyAdd( ptTCB );
            }

            if( bNeedSchedule == OS_TRUE )
            {
                OSSchedule();
            }
        }
    }
    OSIntUnlock();
}

#if ( OS_MUTEX_ON!= 0 )
void *OSTaskGetMutexHolder( void )
{
    if( gptCurrentTCB != OS_NULL )
    {
        ( gptCurrentTCB->uxMutexHoldNum )++;
    }
    return gptCurrentTCB;
}

uOSBool_t OSTaskPriorityInherit( OSTaskHandle_t const MutexHolderTaskHandle )
{
    tOSTCB_t * const ptMutexHolderTCB = ( tOSTCB_t * ) MutexHolderTaskHandle;
    uOSBool_t bReturn = OS_FALSE;
    
    if( MutexHolderTaskHandle != OS_NULL )
    {
        if( ptMutexHolderTCB->uxPriority < gptCurrentTCB->uxPriority )
        {
            OSListItemSetValue( &( ptMutexHolderTCB->tEventListItem ), ( uOSTick_t ) OSHIGHEAST_PRIORITY - ( uOSTick_t ) gptCurrentTCB->uxPriority );

            if( OSListContainListItem( &( gtOSTaskListReady[ ptMutexHolderTCB->uxPriority ] ), &( ptMutexHolderTCB->tTaskListItem ) ) != OS_FALSE )
            {
                if( OSListRemoveItem( &( ptMutexHolderTCB->tTaskListItem ) ) == ( uOSBase_t ) 0U )
                {
                    OSResetReadyPriority( ptMutexHolderTCB->uxPriority );
                }

                ptMutexHolderTCB->uxPriority = gptCurrentTCB->uxPriority;
                OSTaskListReadyAdd( ptMutexHolderTCB );
            }
            else
            {
                /* Just inherit the priority. */
                ptMutexHolderTCB->uxPriority = gptCurrentTCB->uxPriority;
            }
            
            /* Inheritance occurred. */
            bReturn = OS_TRUE;
            
        }
        else
        {
            if( ptMutexHolderTCB->uxBasePriority < gptCurrentTCB->uxPriority )
            {
                /* The base priority of the mutex holder is lower than the
                priority of the task attempting to take the mutex, but the
                current priority of the mutex holder is not lower than the
                priority of the task attempting to take the mutex.
                Therefore the mutex holder must have already inherited a
                priority, but inheritance would have occurred if that had
                not been the case. */
                bReturn = OS_TRUE;
            }
        }
    }
    return bReturn;
}

uOSBool_t OSTaskPriorityDisinherit( OSTaskHandle_t const MutexHolderTaskHandle )
{
    tOSTCB_t * const ptMutexHolderTCB = ( tOSTCB_t * ) MutexHolderTaskHandle;
    uOSBool_t bNeedSchedule = OS_FALSE;

    if( MutexHolderTaskHandle != OS_NULL )
    {
        ( ptMutexHolderTCB->uxMutexHoldNum )--;

        if( ptMutexHolderTCB->uxPriority != ptMutexHolderTCB->uxBasePriority )
        {
            if( ptMutexHolderTCB->uxMutexHoldNum == ( uOSBase_t ) 0U )
            {
                if( OSListRemoveItem( &( ptMutexHolderTCB->tTaskListItem ) ) == ( uOSBase_t ) 0U )
                {
                    OSResetReadyPriority( ptMutexHolderTCB->uxPriority );
                }

                ptMutexHolderTCB->uxPriority = ptMutexHolderTCB->uxBasePriority;

                OSListItemSetValue( &( ptMutexHolderTCB->tEventListItem ), ( uOSTick_t ) OSHIGHEAST_PRIORITY - ( uOSTick_t ) ptMutexHolderTCB->uxPriority );
                OSTaskListReadyAdd( ptMutexHolderTCB );

                bNeedSchedule = OS_TRUE;
            }
        }
    }

    return bNeedSchedule;
}

void OSTaskPriorityDisinheritAfterTimeout( OSTaskHandle_t const MutexHolderTaskHandle, uOSBase_t uxHighestPriorityWaitingTask )
{
    tOSTCB_t * const ptMutexHolderTCB = ( tOSTCB_t * ) MutexHolderTaskHandle;
    uOSBase_t uxPriorityUsedOnEntry = (uOSBase_t)0U;
    uOSBase_t uxPriorityToUse = (uOSBase_t)0U;
    const uOSBase_t uxOnlyOneMutexHeld = ( uOSBase_t ) 1U;

    if( MutexHolderTaskHandle != NULL )
    {
        /* Determine the priority to which the priority of the task that
        holds the mutex should be set.  This will be the greater of the
        holding task's base priority and the priority of the highest
        priority task that is waiting to obtain the mutex. */
        if( ptMutexHolderTCB->uxBasePriority < uxHighestPriorityWaitingTask )
        {
            uxPriorityToUse = uxHighestPriorityWaitingTask;
        }
        else
        {
            uxPriorityToUse = ptMutexHolderTCB->uxBasePriority;
        }

        /* Does the priority need to change? */
        if( ptMutexHolderTCB->uxPriority != uxPriorityToUse )
        {
            /* Only disinherit if no other mutexes are held.  This is a
            simplification in the priority inheritance implementation.  If
            the task that holds the mutex is also holding other mutexes then
            the other mutexes may have caused the priority inheritance. */
            if( ptMutexHolderTCB->uxMutexHoldNum == uxOnlyOneMutexHeld )
            {
                uxPriorityUsedOnEntry = ptMutexHolderTCB->uxPriority;
                ptMutexHolderTCB->uxPriority = uxPriorityToUse;

                /* Only reset the event list item value if the value is not
                being used for anything else. */
                if( OSListItemGetValue( &( ptMutexHolderTCB->tEventListItem ) ) == 0UL )
                {
                    OSListItemSetValue( &( ptMutexHolderTCB->tEventListItem ), ( uOSTick_t ) OSHIGHEAST_PRIORITY - ( uOSTick_t ) uxPriorityToUse ); 
                }

                /* If the running task is not the task that holds the mutex
                then the task that holds the mutex could be in either the
                Ready, Blocked or Suspended states.  Only remove the task
                from its current state list if it is in the Ready state as
                the task's priority is going to change and there is one
                Ready list per priority. */
                if( OSListContainListItem( &( gtOSTaskListReady[ uxPriorityUsedOnEntry ] ), &( ptMutexHolderTCB->tTaskListItem ) ) != OS_FALSE )
                {
                    if( OSListRemoveItem( &( ptMutexHolderTCB->tTaskListItem ) ) == ( uOSBase_t ) 0 )
                    {
                        OSResetReadyPriority( ptMutexHolderTCB->uxPriority );
                    }

                    OSTaskListReadyAdd( ptMutexHolderTCB );
                }
            }
        }
    }
}
#endif /* ( OS_MUTEX_ON!=0 ) */

#if (OSTIME_SLICE_ON != 0U)
uOSBool_t OSTaskNeedTimeSlice( void )
{
    return (uOSBool_t)(OSListGetLength( &( gtOSTaskListReady[ gptCurrentTCB-> uxPriority ] ) )>0U);
}
#endif //(OSTIME_SLICE_ON != 0U)

void OSTaskSuspend( OSTaskHandle_t TaskHandle )
{
    tOSTCB_t *ptTCB = OS_NULL;
    uOSBase_t uxTasksNumTemp = guxCurrentTaskNum;
    
    OSIntLock();
    {
        /* If TaskHandle is null then it is the running task that is
        being suspended. */
        ptTCB = OSTaskGetTCBFromHandle( TaskHandle );

        /* Remove task from the ready/timer list */
        if( OSListRemoveItem( &( ptTCB->tTaskListItem ) ) == ( uOSBase_t ) 0 )
        {
            OSResetReadyPriority( ptTCB->uxPriority );
        }

        /* Is the task waiting on an event list */
        if( OSListItemGetList( &( ptTCB->tEventListItem ) ) != OS_NULL )
        {
            ( void ) OSListRemoveItem( &( ptTCB->tEventListItem ) );
        }

        /* place the task in the suspended list. */
        OSListInsertItemToEnd( &gptOSTaskListSuspended, &( ptTCB->tTaskListItem ) );

        #if( OS_TASK_SIGNAL_ON!=0 )
        {
            if( ptTCB->ucSigState == SIG_STATE_WAITING )
            {
                /* The task was blocked to wait for a signal, but is
                now suspended, so no signal was received. */
                ptTCB->ucSigState = SIG_STATE_NOTWAITING;
            }
        }
        #endif
    }
    OSIntUnlock();

    if( OSScheduleGetState() != SCHEDULER_NOT_STARTED )
    {
        /* Update the next expected unblock time */
        OSIntLock();
        {
            OSUpdateUnblockTime();
        }
        OSIntUnlock();
    }

    if( ptTCB == gptCurrentTCB )
    {
        if( OSScheduleGetState() != SCHEDULER_NOT_STARTED )
        {
            OSSchedule();
        }
        else
        {
            if( OSListGetLength( &gptOSTaskListSuspended ) == uxTasksNumTemp )
            {
                gptCurrentTCB = OS_NULL;
            }
            else
            {
                OSTaskSwitchContext();
            }
        }
    }
}

static uOSBool_t OSTaskIsSuspended( const OSTaskHandle_t TaskHandle )
{
    uOSBool_t bReturn = OS_FALSE;
    const tOSTCB_t * const ptTCB = ( tOSTCB_t * ) TaskHandle;

    /* Is the task being resumed actually in the suspended list? */
    if( OSListContainListItem( &gptOSTaskListSuspended, &( ptTCB->tTaskListItem ) ) != OS_FALSE )
    {
        /* Has the task already been resumed from within an ISR? */
        if( OSListContainListItem( &gtOSTaskListReadyPool, &( ptTCB->tEventListItem ) ) == OS_FALSE )
        {
            /* Is it in the suspended list because it is in the    Suspended
            state, or because is is blocked with no timeout? */
            if( OSListContainListItem( OS_NULL, &( ptTCB->tEventListItem ) ) != OS_FALSE )
            {
                bReturn = OS_TRUE;
            }
        }
    }

    return bReturn;
}

void OSTaskResume( OSTaskHandle_t TaskHandle )
{
    tOSTCB_t * const ptTCB = ( tOSTCB_t * ) TaskHandle;

    if( ( ptTCB != OS_NULL ) && ( ptTCB != gptCurrentTCB ) )
    {
        OSIntLock();
        {
            if( OSTaskIsSuspended( ptTCB ) != OS_FALSE )
            {
                /* In a critical section we can access the ready lists. */
                ( void ) OSListRemoveItem(  &( ptTCB->tTaskListItem ) );
                OSTaskListReadyAdd( ptTCB );

                if( ptTCB->uxPriority >= gptCurrentTCB->uxPriority )
                {
                    OSSchedule();
                }
            }
        }
        OSIntUnlock();
    }
}


sOSBase_t OSTaskResumeFromISR( OSTaskHandle_t TaskHandle )
{
    uOSBool_t bNeedSchedule = OS_FALSE;
    tOSTCB_t * const ptTCB = ( tOSTCB_t * ) TaskHandle;
    uOSBase_t uxIntSave  = (uOSBase_t)0U;

    uxIntSave = OSIntMaskFromISR();
    {
        if( OSTaskIsSuspended( ptTCB ) != OS_FALSE )
        {
            /* Check the ready lists can be accessed. */
            if( OSScheduleIsLocked() == OS_FALSE )
            {
                /* Ready lists can be accessed so move the task from the
                suspended list to the ready list directly. */
                if( ptTCB->uxPriority >= gptCurrentTCB->uxPriority )
                {
                    bNeedSchedule = OS_TRUE;
                }

                ( void ) OSListRemoveItem( &( ptTCB->tTaskListItem ) );
                OSTaskListReadyAdd( ptTCB );
            }
            else
            {
                /* The timer or ready lists cannot be accessed so the task
                is held in the pending ready list until the scheduler is
                unsuspended. */
                OSListInsertItemToEnd( &( gtOSTaskListReadyPool ), &( ptTCB->tEventListItem ) );
            }
        }
    }
    OSIntUnmaskFromISR( uxIntSave );

    return bNeedSchedule;
}

#if ( OS_TIMER_ON!=0 )
void OSTaskBlockAndPend( tOSList_t * const ptEventList, uOSTick_t uxTicksToWait, uOSBool_t bNeedSuspend )
{
    OSListInsertItemToEnd( ptEventList, &( gptCurrentTCB->tEventListItem ) );

    OSTaskListPendAdd( gptCurrentTCB, uxTicksToWait, bNeedSuspend );
}
#endif /* ( OS_TIMER_ON!=0 ) */

#if ( OS_LOWPOWER_ON!=0 ) 

#endif //OS_LOWPOWER_ON

void OSIdleTask( void *pvParameters)
{
    /* Just to avoid compiler warnings. */
    ( void ) pvParameters;
    
    for( ;; )
    {
        // if there is not any other task ready, then OS enter idle task;

        if( OSListGetLength( &( gtOSTaskListReady[ OSLOWEAST_PRIORITY ] ) ) > ( uOSBase_t ) 1 )
        {
            OSSchedule();
        }
        
        #if ( OS_MEMFREE_ON != 0 )         
        if(guxTasksDeleted > ( uOSBase_t ) 0U)
        {
            OSTaskListRecycleRemove();
        }
        #endif /* OS_MEMFREE_ON */        
        
        #if ( OS_LOWPOWER_ON!=0 )
        {
            uOSTick_t uxLowPowerTicks = OSGetBlockTickCount();

            if( uxLowPowerTicks >= OS_LOWPOWER_MINI_TICKS )
            {
                OSScheduleLock();
                {
                    /* Now the scheduler is suspended, the expected idle
                    time can be sampled again, and this time its value can
                    be used. */
                    uxLowPowerTicks = OSGetBlockTickCount();

                    if( uxLowPowerTicks >= OS_LOWPOWER_MINI_TICKS )
                    {
                        FitLowPowerIdle( uxLowPowerTicks );
                    }
                }
                ( void ) OSScheduleUnlock();
            }
        }
        #endif /* OS_LOWPOWER_ON */        
    }
}

#if ( OS_TASK_SIGNAL_ON!=0 )

uOSBool_t OSTaskSignalWait( uOSTick_t const uxTicksToWait)
{
    sOSBase_t xTemp = (sOSBase_t)0;
    uOSBool_t bReturn = OS_FALSE;

    OSIntLock();
    {
        /* Only block if the signal count is not already non-zero. */
        if( gptCurrentTCB->uiSigValue == 0UL )
        {
            /* Mark this task as waiting for a signal. */
            gptCurrentTCB->ucSigState = SIG_STATE_WAITING;

            if( uxTicksToWait > ( uOSTick_t ) 0 )
            {
                OSTaskListPendAdd( gptCurrentTCB, uxTicksToWait, OS_TRUE );

                OSSchedule();
            }
        }
    }
    OSIntUnlock();

    OSIntLock();
    {
        xTemp = gptCurrentTCB->uiSigValue;

        if( (uOS32_t)xTemp > 0UL )
        {
            gptCurrentTCB->uiSigValue = xTemp - (uOSBase_t)1;
            
            bReturn = OS_TRUE;
        }

        gptCurrentTCB->ucSigState = SIG_STATE_NOTWAITING;
    }
    OSIntUnlock();

    return bReturn;
}
uOSBool_t OSTaskSignalEmit( OSTaskHandle_t const TaskHandle )
{
    tOSTCB_t * ptTCB = OS_NULL;
    uOSBool_t bReturn = OS_FALSE;
    uOS8_t ucOldState = SIG_STATE_NOTWAITING;

    ptTCB = ( tOSTCB_t * ) TaskHandle;

    OSIntLock();
    {
        ucOldState = ptTCB->ucSigState;

        ptTCB->ucSigState = SIG_STATE_RECEIVED;
        if( ptTCB->uiSigValue>0xF )
        {
            bReturn = OS_FALSE;
        }
        else
        {
            ptTCB->uiSigValue += 1;
            bReturn = OS_TRUE;
        }
        /* If the task is in the blocked state specifically to wait for a
        signal then unblock it now. */
        if( ucOldState == SIG_STATE_WAITING )
        {
            ( void ) OSListRemoveItem( &( ptTCB->tTaskListItem ) );
            OSTaskListReadyAdd( ptTCB );

            OSUpdateUnblockTime();

            if( ptTCB->uxPriority > gptCurrentTCB->uxPriority )
            {
                /* The signaled task has a priority above the currently
                executing task so schedule is required. */
                OSSchedule();
            }
        }
    }
    OSIntUnlock();

    return bReturn;
}
uOSBool_t OSTaskSignalEmitFromISR( OSTaskHandle_t const TaskHandle )
{
    tOSTCB_t * ptTCB = OS_NULL;
    uOS8_t ucOldState = SIG_STATE_NOTWAITING;
    uOSBase_t uxIntSave = (uOSBase_t)0U;
    uOSBool_t bNeedSchedule = OS_FALSE;
    uOSBool_t bReturn = OS_FALSE;

    ptTCB = ( tOSTCB_t * ) TaskHandle;

    uxIntSave = OSIntMaskFromISR();
    {
        ucOldState = ptTCB->ucSigState;
        ptTCB->ucSigState = SIG_STATE_RECEIVED;

        if( ptTCB->uiSigValue>0xF )
        {
            bReturn = OS_FALSE;
        }
        else
        {
            ptTCB->uiSigValue += 1;
            bReturn = OS_TRUE;
        }

        /* If the task is in the blocked state specifically to wait for a
        signal then unblock it now. */
        if( ucOldState == SIG_STATE_WAITING )
        {
            /* The task should not have been on an event list. */
            if( OSScheduleIsLocked() == OS_FALSE )
            {
                ( void ) OSListRemoveItem( &( ptTCB->tTaskListItem ) );
                OSTaskListReadyAdd( ptTCB );
            }
            else
            {
                /* The timer and ready lists cannot be accessed, so hold
                this task pending until the scheduler is resumed. */
                OSListInsertItemToEnd( &( gtOSTaskListReadyPool ), &( ptTCB->tEventListItem ) );
            }

            if( ptTCB->uxPriority > gptCurrentTCB->uxPriority )
            {
                /* The signaled task has a priority above the currently
                executing task so schedule is required. */
                bNeedSchedule = OS_TRUE;
            }
        }
    }
    OSIntUnmaskFromISR( uxIntSave );    
    
    if(bNeedSchedule == OS_TRUE)
    {
        OSSchedule();
    }
    return bReturn;
}
uOSBool_t OSTaskSignalWaitMsg( uOS32_t* puiSigValue, uOSTick_t const uxTicksToWait)
{
    uOSBool_t bReturn = OS_FALSE;

    OSIntLock();
    {
        /* Only block if a signal is not already pending. */
        if( gptCurrentTCB->ucSigState != SIG_STATE_RECEIVED )
        {
            /* clear the value to zero. */
            gptCurrentTCB->uiSigValue = 0;

            /* Mark this task as waiting for a signal. */
            gptCurrentTCB->ucSigState = SIG_STATE_WAITING;

            if( uxTicksToWait > ( uOSTick_t ) 0 )
            {
                OSTaskListPendAdd( gptCurrentTCB, uxTicksToWait, OS_TRUE );

                /* All ports are written to allow schedule in a critical
                section (some will schedule immediately, others wait until the
                critical section exits) - but it is not something that
                application code should ever do. */
                OSSchedule();
            }
        }
    }
    OSIntUnlock();

    OSIntLock();
    {
        if( puiSigValue != OS_NULL )
        {
            /* Output the current signal value. */
            *puiSigValue = gptCurrentTCB->uiSigValue;
        }

        /* If uiSigValue is set then either the task never entered the
        blocked state (because a signal was already pending) or the
        task unblocked because of a signal.  Otherwise the task
        unblocked because of a timeout. */
        if( gptCurrentTCB->ucSigState != SIG_STATE_RECEIVED )
        {
            /* A signal was not received. */
            bReturn = OS_FALSE;
        }
        else
        {
            /* A signal was already pending or a signal was
            received while the task was waiting. */
            gptCurrentTCB->uiSigValue = 0;
            bReturn = OS_TRUE;
        }

        gptCurrentTCB->ucSigState = SIG_STATE_NOTWAITING;
    }
    OSIntUnlock();

    return bReturn;
}
uOSBool_t OSTaskSignalEmitMsg( OSTaskHandle_t const TaskHandle, uOS32_t const uiSigValue, uOSBool_t bOverWrite )
{
    tOSTCB_t * ptTCB = OS_NULL;
    uOSBool_t bReturn = OS_FALSE;
    uOS8_t ucOldState = SIG_STATE_NOTWAITING;

    ptTCB = ( tOSTCB_t * ) TaskHandle;

    OSIntLock();
    {
        ucOldState = ptTCB->ucSigState;

        ptTCB->ucSigState = SIG_STATE_RECEIVED;
        if( ucOldState != SIG_STATE_RECEIVED || bOverWrite == OS_TRUE )
        {
            ptTCB->uiSigValue = uiSigValue;
            bReturn = OS_TRUE;
        }
        else
        {
            /* The value could not be written to the task. */
            bReturn = OS_FALSE;
        }

        /* If the task is in the blocked state specifically to wait for a
        signal then unblock it now. */
        if( ucOldState == SIG_STATE_WAITING )
        {
            ( void ) OSListRemoveItem( &( ptTCB->tTaskListItem ) );
            OSTaskListReadyAdd( ptTCB );

            OSUpdateUnblockTime();

            if( ptTCB->uxPriority > gptCurrentTCB->uxPriority )
            {
                /* The signaled task has a priority above the currently
                executing task so schedule is required. */
                OSSchedule();
            }
        }
    }
    OSIntUnlock();

    return bReturn;
}
uOSBool_t OSTaskSignalEmitMsgFromISR( OSTaskHandle_t const TaskHandle, uOS32_t const uiSigValue, uOSBool_t bOverWrite )
{
    tOSTCB_t * ptTCB = OS_NULL;
    uOS8_t ucOldState = SIG_STATE_NOTWAITING;
    uOSBool_t bReturn = OS_TRUE;
    uOSBase_t uxIntSave = (uOSBase_t)0U;
    uOSBool_t bNeedSchedule = OS_FALSE;
    
    ptTCB = ( tOSTCB_t * ) TaskHandle;

    uxIntSave = OSIntMaskFromISR();
    {
        ucOldState = ptTCB->ucSigState;
        ptTCB->ucSigState = SIG_STATE_RECEIVED;
        if( ucOldState != SIG_STATE_RECEIVED || bOverWrite == OS_TRUE )
        {
            ptTCB->uiSigValue = uiSigValue;
            bReturn = OS_TRUE;
        }
        else
        {
            /* The value could not be written to the task. */
            bReturn = OS_FALSE;
        }

        /* If the task is in the blocked state specifically to wait for a
        signal then unblock it now. */
        if( ucOldState == SIG_STATE_WAITING )
        {
            if( OSScheduleIsLocked() == OS_FALSE )
            {
                ( void ) OSListRemoveItem( &( ptTCB->tTaskListItem ) );
                OSTaskListReadyAdd( ptTCB );
            }
            else
            {
                /* The timer and ready lists cannot be accessed, so hold
                this task pending until the scheduler is resumed. */
                OSListInsertItemToEnd( &( gtOSTaskListReadyPool ), &( ptTCB->tEventListItem ) );
            }

            if( ptTCB->uxPriority > gptCurrentTCB->uxPriority )
            {
                /* The signaled task has a priority above the currently
                executing task so schedule is required. */

                bNeedSchedule = OS_TRUE;
            }
        }
    }
    OSIntUnmaskFromISR( uxIntSave );

    if(bNeedSchedule == OS_TRUE)
    {
        OSSchedule();
    }
    return bReturn;
}
uOSBool_t OSTaskSignalClear( OSTaskHandle_t const TaskHandle )
{
    tOSTCB_t *ptTCB = OS_NULL;
    uOSBool_t bReturn = OS_FALSE;

    /* If null is passed in here then it is the calling task that is having
    its signal state cleared. */
    ptTCB = OSTaskGetTCBFromHandle( TaskHandle );

    OSIntLock();
    {
        if( ptTCB->ucSigState == SIG_STATE_RECEIVED )
        {
            ptTCB->ucSigState = SIG_STATE_NOTWAITING;
            ptTCB->uiSigValue = 0;
            bReturn = OS_TRUE;
        }
        else
        {
            bReturn = OS_FALSE;
        }
    }
    OSIntUnlock();

    return bReturn;    
}
#endif

#ifdef __cplusplus
}
#endif
