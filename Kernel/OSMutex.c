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

#include "TINIUX.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ( OS_MUTEX_ON!=0 )

TINIUX_DATA static sOSBase_t const MUTEX_STATUS_UNLOCKED              = ( ( sOSBase_t ) -1 );
TINIUX_DATA static sOSBase_t const MUTEX_STATUS_LOCKED                = ( ( sOSBase_t ) 0 );
TINIUX_DATA static uOSTick_t const MUTEX_UNLOCK_BLOCK_TIME            = ( ( uOSTick_t ) 0U );

static uOSBool_t OSMutexIsEmpty( OSMutexHandle_t MutexHandle )
{
    uOSBool_t bReturn = OS_FALSE;
    tOSMutex_t * const ptMutex = ( tOSMutex_t * ) MutexHandle;
    
    OSIntLock();
    {
        if( ptMutex->uxCurNum == ( uOSBase_t )  0 )
        {
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

static uOSBool_t OSMutexIsFull( OSMutexHandle_t MutexHandle )
{
    uOSBool_t bReturn = OS_FALSE;
    tOSMutex_t * const ptMutex = ( tOSMutex_t * ) MutexHandle;

    OSIntLock();
    {
        if( ptMutex->uxCurNum == ptMutex->uxMaxNum )
        {
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

#define OSMutexStatusLock( ptMutex )                               \
    OSIntLock();                                                   \
    {                                                              \
        if( ( ptMutex )->xMutexPLock == MUTEX_STATUS_UNLOCKED )    \
        {                                                          \
            ( ptMutex )->xMutexPLock = MUTEX_STATUS_LOCKED;        \
        }                                                          \
        if( ( ptMutex )->xMutexVLock == MUTEX_STATUS_UNLOCKED )    \
        {                                                          \
            ( ptMutex )->xMutexVLock = MUTEX_STATUS_LOCKED;        \
        }                                                          \
    }                                                              \
    OSIntUnlock()


static void OSMutexStatusUnlock( tOSMutex_t * const ptMutex )
{
    /* THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER LOCKED. */

    OSIntLock();
    {
        sOSBase_t xMutexVLock = ptMutex->xMutexVLock;
        
        while( xMutexVLock > MUTEX_STATUS_LOCKED )
        {
            if( OSListIsEmpty( &( ptMutex->tTaskListEventMutexP ) ) == OS_FALSE )
            {
                if( OSTaskListEventRemove( &( ptMutex->tTaskListEventMutexP ) ) != OS_FALSE )
                {
                    OSNeedSchedule();
                }
            }
            else
            {
                break;
            }

            --xMutexVLock;
        }

        ptMutex->xMutexVLock = MUTEX_STATUS_UNLOCKED;
    }
    OSIntUnlock();

    /* Do the same for the MutexP lock. */
    OSIntLock();
    {
        sOSBase_t xMutexPLock = ptMutex->xMutexPLock;
        
        while( xMutexPLock > MUTEX_STATUS_LOCKED )
        {
            if( OSListIsEmpty( &( ptMutex->tTaskListEventMutexV ) ) == OS_FALSE )
            {
                if( OSTaskListEventRemove( &( ptMutex->tTaskListEventMutexV ) ) != OS_FALSE )
                {
                    OSNeedSchedule();
                }

                --xMutexPLock;
            }
            else
            {
                break;
            }
        }

        ptMutex->xMutexPLock = MUTEX_STATUS_UNLOCKED;
    }
    OSIntUnlock();
}


OSMutexHandle_t OSMutexCreate( void )
{
    tOSMutex_t *ptNewMutex = OS_NULL;

    /* Allocate the new queue structure. */
    ptNewMutex = ( tOSMutex_t * ) OSMemMalloc( sizeof( tOSMutex_t ) );
    if( ptNewMutex != OS_NULL )
    {
        /* Information required for priority inheritance. */
        ptNewMutex->MutexHolderHandle = OS_NULL;

        ptNewMutex->uxCurNum = ( uOSBase_t ) 1U;
        ptNewMutex->uxMaxNum = ( uOSBase_t ) 1U;
        
        ptNewMutex->uxMutexLocked = ( uOSBase_t ) OS_FALSE;
        
        ptNewMutex->xMutexPLock = MUTEX_STATUS_UNLOCKED;
        ptNewMutex->xMutexVLock = MUTEX_STATUS_UNLOCKED;

        /* Ensure the event queues start with the correct state. */
        OSListInit( &( ptNewMutex->tTaskListEventMutexV ) );
        OSListInit( &( ptNewMutex->tTaskListEventMutexP ) );
    }
    return (OSMutexHandle_t)ptNewMutex;
}

#if ( OS_MEMFREE_ON != 0 )
void OSMutexDelete( OSMutexHandle_t MutexHandle )
{
    tOSMutex_t * const ptMutex = ( tOSMutex_t * ) MutexHandle;

    OSMemFree( ptMutex );
}
#endif /* OS_MEMFREE_ON */

sOSBase_t OSMutexSetID(OSMutexHandle_t MutexHandle, sOSBase_t xID)
{
    if(MutexHandle == OS_NULL)
    {
        return 1;
    }
    OSIntLock();
    {
        MutexHandle->xID = xID;
    }
    OSIntUnlock();

    return 0;
}

sOSBase_t OSMutexGetID(OSMutexHandle_t const MutexHandle)
{
    sOSBase_t xID = 0;
    
    OSIntLock();
    if(MutexHandle != OS_NULL)
    {
        xID = MutexHandle->xID;
    }
    OSIntUnlock();

    return xID;    
}

static uOSBase_t OSMutexGetDisinheritPriorityAfterTimeout( OSMutexHandle_t const MutexHandle )
{
    uOSBase_t uxHighestPriorityOfWaitingTasks = OSLOWEAST_PRIORITY;

    /* If a task waiting for a mutex causes the mutex holder to inherit a
    priority, but the waiting task times out, then the holder should
    disinherit the priority - but only down to the highest priority of any
    other tasks that are waiting for the same mutex.  For this purpose,
    return the priority of the highest priority task that is waiting for the
    mutex. */
    if( OSListGetLength( &( MutexHandle->tTaskListEventMutexP ) ) > 0 )
    {
        uxHighestPriorityOfWaitingTasks = OSHIGHEAST_PRIORITY - OSlistGetHeadItemValue( &( MutexHandle->tTaskListEventMutexP ) );
    }
    else
    {
        uxHighestPriorityOfWaitingTasks = OSLOWEAST_PRIORITY;
    }

    return uxHighestPriorityOfWaitingTasks;
}
    
uOSBool_t OSMutexLock( OSMutexHandle_t MutexHandle, uOSTick_t uxTicksToWait )
{
    uOSBool_t bEntryTimeSet = OS_FALSE;
    tOSTimeOut_t tTimeOut;
    tOSMutex_t * const ptMutex = ( tOSMutex_t * ) MutexHandle;
    uOSBool_t bInheritanceOccurred = OS_FALSE;

    //the mutex have been locked
    if( ptMutex->MutexHolderHandle == ( void * ) OSGetCurrentTaskHandle() ) 
    {
        ( ptMutex->uxMutexLocked )++;
        return OS_TRUE;
    }
    
    for( ;; )
    {
        OSIntLock();
        {
            const uOSBase_t uxCurNum = ptMutex->uxCurNum;

            if( uxCurNum > ( uOSBase_t ) 0 )
            {
                ptMutex->uxCurNum = uxCurNum - 1;
                ptMutex->MutexHolderHandle = ( sOS8_t * ) OSTaskGetMutexHolder();
                //mutex locked successfully
                ( ptMutex->uxMutexLocked )++;
                                
                if( OSListIsEmpty( &( ptMutex->tTaskListEventMutexV ) ) == OS_FALSE )
                {
                    if( OSTaskListEventRemove( &( ptMutex->tTaskListEventMutexV ) ) != OS_FALSE )
                    {
                        OSSchedule();
                    }
                }

                OSIntUnlock();
                return OS_TRUE;
            }
            else
            {
                if( uxTicksToWait == ( uOSTick_t ) 0 )
                {
                    OSIntUnlock();
                    //the mutex is empty
                    return OS_FALSE;
                }
                else if( bEntryTimeSet == OS_FALSE )
                {
                    OSSetTimeOutState( &tTimeOut );
                    bEntryTimeSet = OS_TRUE;
                }
            }
        }
        OSIntUnlock();

        /* Interrupts and other tasks can unlock or lock from the mutex
        To avoid confusion, we lock the scheduler and the mutex. */
        OSScheduleLock();
        OSMutexStatusLock( ptMutex );

        if( OSGetTimeOutState( &tTimeOut, &uxTicksToWait ) == OS_FALSE )
        {
            if( OSMutexIsEmpty( ptMutex ) != OS_FALSE )
            {
                OSIntLock();
                {
                    bInheritanceOccurred = OSTaskPriorityInherit( ( void * ) ptMutex->MutexHolderHandle );
                }
                OSIntUnlock();
                
                OSTaskListEventAdd( &( ptMutex->tTaskListEventMutexP ), uxTicksToWait );
                OSMutexStatusUnlock( ptMutex );
                if( OSScheduleUnlock() == OS_FALSE )
                {
                    OSSchedule();
                }
            }
            else
            {
                /* Try again. */
                OSMutexStatusUnlock( ptMutex );
                ( void ) OSScheduleUnlock();
            }
        }
        else
        {
            OSMutexStatusUnlock( ptMutex );
            ( void ) OSScheduleUnlock();
            
            if( OSMutexIsEmpty( ptMutex ) != OS_FALSE )
            {
                if( bInheritanceOccurred != OS_FALSE )
                {
                    OSIntLock();
                    {
                        uOSBase_t uxHighestWaitingPriority;

                        /* This task blocking on the mutex caused another
                        task to inherit this task's priority.  Now this task
                        has timed out the priority should be disinherited
                        again, but only as low as the next highest priority
                        task that is waiting for the same mutex. */
                        uxHighestWaitingPriority = OSMutexGetDisinheritPriorityAfterTimeout( ptMutex );
                        OSTaskPriorityDisinheritAfterTimeout( ( void * ) ptMutex->MutexHolderHandle, uxHighestWaitingPriority );
                    }
                    OSIntUnlock();
                }
                    
                //the Mutex is empty
                return OS_FALSE;
            }
        }
    }
}

uOSBool_t OSMutexUnlock( OSMutexHandle_t MutexHandle )
{
    uOSBool_t bEntryTimeSet = OS_FALSE, bNeedSchedule;
    tOSTimeOut_t tTimeOut;
    tOSMutex_t * const ptMutex = ( tOSMutex_t * ) MutexHandle;

    uOSTick_t uxTicksToWait = MUTEX_UNLOCK_BLOCK_TIME;

    /* The calling task is not the holder, the mutex cannot be unlocked here. */
    if( ptMutex->MutexHolderHandle != ( void * ) OSGetCurrentTaskHandle() )
    {
        return OS_FALSE;
    }
    else
    {
        ( ptMutex->uxMutexLocked )--;
        if( ptMutex->uxMutexLocked != ( uOSBase_t ) OS_FALSE )
        {
            return OS_TRUE;
        }
    }

    // now uxMutexLocked is OS_FALSE
    for( ;; )
    {
        OSIntLock();
        {
            const uOSBase_t uxCurNum = ptMutex->uxCurNum;

            if( uxCurNum < ptMutex->uxMaxNum )
            {
                /* The mutex is no longer being held. */
                bNeedSchedule = OSTaskPriorityDisinherit( ( void * ) ptMutex->MutexHolderHandle );
                ptMutex->MutexHolderHandle = OS_NULL;
                ptMutex->uxCurNum = uxCurNum + 1;

                if( OSListIsEmpty( &( ptMutex->tTaskListEventMutexP ) ) == OS_FALSE )
                {
                    if( OSTaskListEventRemove( &( ptMutex->tTaskListEventMutexP ) ) != OS_FALSE )
                    {
                        OSSchedule();
                    }
                }
                else if( bNeedSchedule != OS_FALSE )
                {
                    OSSchedule();
                }

                OSIntUnlock();
                return OS_TRUE;
            }
            else
            {
                if( uxTicksToWait == ( uOSTick_t ) 0 )
                {
                    OSIntUnlock();
                    //the mutex is full
                    return OS_FALSE;
                }
                else if( bEntryTimeSet == OS_FALSE )
                {
                    OSSetTimeOutState( &tTimeOut );
                    bEntryTimeSet = OS_TRUE;
                }
            }
        }
        OSIntUnlock();

        /* Interrupts and other tasks can unlock or lock from the mutex
        To avoid confusion, we lock the scheduler and the mutex. */
        OSScheduleLock();
        OSMutexStatusLock( ptMutex );

        if( OSGetTimeOutState( &tTimeOut, &uxTicksToWait ) == OS_FALSE )
        {
            if( OSMutexIsFull( ptMutex ) != OS_FALSE )
            {
                OSTaskListEventAdd( &( ptMutex->tTaskListEventMutexV ), uxTicksToWait );

                OSMutexStatusUnlock( ptMutex );

                if( OSScheduleUnlock() == OS_FALSE )
                {
                    OSSchedule();
                }
            }
            else
            {
                /* Try again. */
                OSMutexStatusUnlock( ptMutex );
                ( void ) OSScheduleUnlock();
            }
        }
        else
        {
            /* The timeout has expired. */
            OSMutexStatusUnlock( ptMutex );
            ( void ) OSScheduleUnlock();
            //the mutex is full
            return OS_FALSE;
        }
    }
}

#endif //(OS_MUTEX_ON!=0)

#ifdef __cplusplus
}
#endif
