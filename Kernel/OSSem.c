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

#if ( OS_SEMAPHORE_ON!=0 )

/* Semaphores do not actually store or copy data, so have an item size of zero. */
//TINIUX_DATA static uOSBase_t const SEMAPHORE_QUEUE_ITEM_LENGTH    = ( ( uOSBase_t ) 0U );
TINIUX_DATA static uOSBase_t const SEMAPHORE_QUEUE_LENGTH         = ( ( uOSBase_t ) 1U );
TINIUX_DATA static uOSTick_t const SEMAPOST_BLOCK_TIME            = ( ( uOSTick_t ) 0U );

TINIUX_DATA static sOSBase_t const SEM_STATUS_UNLOCKED            = ( ( sOSBase_t ) -1 );
TINIUX_DATA static sOSBase_t const SEM_STATUS_LOCKED              = ( ( sOSBase_t ) 0 );

static uOSBool_t OSSemIsEmpty( OSSemHandle_t SemHandle )
{
    uOSBool_t bReturn = OS_FALSE;
    tOSSem_t * const ptSem = ( tOSSem_t * ) SemHandle;

    OSIntLock();
    {
        if( ptSem->uxCurNum == ( uOSBase_t )  0 )
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

static uOSBool_t OSSemIsFull( OSSemHandle_t SemHandle )
{
    uOSBool_t bReturn = OS_FALSE;
    tOSSem_t * const ptSem = ( tOSSem_t * ) SemHandle;

    OSIntLock();
    {
        if( ptSem->uxCurNum == ptSem->uxMaxNum )
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

#define OSSemStateLock( ptSem )                             \
    OSIntLock();                                            \
    {                                                       \
        if( ( ptSem )->xSemPLock == SEM_STATUS_UNLOCKED )   \
        {                                                   \
            ( ptSem )->xSemPLock = SEM_STATUS_LOCKED;       \
        }                                                   \
        if( ( ptSem )->xSemVLock == SEM_STATUS_UNLOCKED )   \
        {                                                   \
            ( ptSem )->xSemVLock = SEM_STATUS_LOCKED;       \
        }                                                   \
    }                                                       \
    OSIntUnlock()


static void OSSemStateUnlock( tOSSem_t * const ptSem )
{
    /* THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER LOCKED. */

    OSIntLock();
    {
        sOSBase_t xSemVLock = ptSem->xSemVLock;
        
        while( xSemVLock > SEM_STATUS_LOCKED )
        {
            if( OSListIsEmpty( &( ptSem->tTaskListEventSemP ) ) == OS_FALSE )
            {
                if( OSTaskListEventRemove( &( ptSem->tTaskListEventSemP ) ) != OS_FALSE )
                {
                    OSNeedSchedule();
                }
            }
            else
            {
                break;
            }

            --xSemVLock;
        }

        ptSem->xSemVLock = SEM_STATUS_UNLOCKED;
    }
    OSIntUnlock();

    /* Do the same for the SemP lock. */
    OSIntLock();
    {
        sOSBase_t xSemPLock = ptSem->xSemPLock;
        
        while( xSemPLock > SEM_STATUS_LOCKED )
        {
            if( OSListIsEmpty( &( ptSem->tTaskListEventSemV ) ) == OS_FALSE )
            {
                if( OSTaskListEventRemove( &( ptSem->tTaskListEventSemV ) ) != OS_FALSE )
                {
                    OSNeedSchedule();
                }

                --xSemPLock;
            }
            else
            {
                break;
            }
        }

        ptSem->xSemPLock = SEM_STATUS_UNLOCKED;
    }
    OSIntUnlock();
}

sOSBase_t OSSemReset( OSSemHandle_t SemHandle, uOSBool_t bNewQueue )
{
    tOSSem_t * const ptSem = ( tOSSem_t * ) SemHandle;

    OSIntLock();
    {
        ptSem->uxCurNum = ( uOSBase_t ) 0U;
        ptSem->xSemPLock = SEM_STATUS_UNLOCKED;
        ptSem->xSemVLock = SEM_STATUS_UNLOCKED;

        if( bNewQueue == OS_FALSE )
        {
            if( OSListIsEmpty( &( ptSem->tTaskListEventSemV ) ) == OS_FALSE )
            {
                if( OSTaskListEventRemove( &( ptSem->tTaskListEventSemV ) ) != OS_FALSE )
                {
                    OSSchedule();
                }
            }
        }
        else
        {
            OSListInit( &( ptSem->tTaskListEventSemV ) );
            OSListInit( &( ptSem->tTaskListEventSemP ) );
        }
    }
    OSIntUnlock();

    return OS_TRUE;
}

OSSemHandle_t OSSemCreateCount( const uOSBase_t uxMaxNum, const uOSBase_t uxInitialCount )
{
    tOSSem_t *ptNewSem = OS_NULL;
    OSSemHandle_t xReturn = OS_NULL;

    ptNewSem = ( tOSSem_t * ) OSMemMalloc( sizeof( tOSSem_t ) );

    if( ptNewSem != OS_NULL )
    {
        ptNewSem->uxMaxNum = uxMaxNum;

        ( void ) OSSemReset( ptNewSem, OS_TRUE );

        ptNewSem->uxCurNum = uxInitialCount;

        xReturn = ptNewSem;
    }

    return xReturn;
}

OSSemHandle_t OSSemCreate( const uOSBase_t uxInitialCount )
{
    uOSBase_t uxInitialCountTemp = uxInitialCount;
    if(uxInitialCountTemp>SEMAPHORE_QUEUE_LENGTH)
    {
        uxInitialCountTemp = SEMAPHORE_QUEUE_LENGTH;
    }
    return OSSemCreateCount( ( uOSBase_t )SEMAPHORE_QUEUE_LENGTH, ( uOSBase_t )uxInitialCountTemp );
}

#if ( OS_MEMFREE_ON != 0 )
void OSSemDelete( OSSemHandle_t SemHandle )
{
    tOSSem_t * const ptSem = ( tOSSem_t * ) SemHandle;

    OSMemFree( ptSem );
}
#endif /* OS_MEMFREE_ON */

sOSBase_t OSSemSetID(OSSemHandle_t SemHandle, sOSBase_t xID)
{
    if(SemHandle == OS_NULL)
    {
        return 1;
    }
    OSIntLock();
    {
        SemHandle->xID = xID;
    }
    OSIntUnlock();

    return 0;
}

sOSBase_t OSSemGetID(OSSemHandle_t const SemHandle)
{
    sOSBase_t xID = 0;
    
    OSIntLock();
    if(SemHandle != OS_NULL)
    {
        xID = SemHandle->xID;
    }
    OSIntUnlock();

    return xID;
}

uOSBool_t OSSemPend( OSSemHandle_t SemHandle, uOSTick_t uxTicksToWait )
{
    uOSBool_t bEntryTimeSet = OS_FALSE;
    tOSTimeOut_t tTimeOut;
    tOSSem_t * const ptSem = ( tOSSem_t * ) SemHandle;

    for( ;; )
    {
        OSIntLock();
        {
            const uOSBase_t uxCurNum = ptSem->uxCurNum;
            
            if( uxCurNum > ( uOSBase_t ) 0 )
            {
                ptSem->uxCurNum = uxCurNum - 1;

                if( OSListIsEmpty( &( ptSem->tTaskListEventSemV ) ) == OS_FALSE )
                {
                    if( OSTaskListEventRemove( &( ptSem->tTaskListEventSemV ) ) != OS_FALSE )
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
                    //the semaphore is empty
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

        /* Interrupts and other tasks can post to or pend from the semaphore
        To avoid confusion, we lock the scheduler and the semaphore. */
        OSScheduleLock();
        OSSemStateLock( ptSem );

        if( OSGetTimeOutState( &tTimeOut, &uxTicksToWait ) == OS_FALSE )
        {
            if( OSSemIsEmpty( ptSem ) != OS_FALSE )
            {
                OSTaskListEventAdd( &( ptSem->tTaskListEventSemP ), uxTicksToWait );
                OSSemStateUnlock( ptSem );
                if( OSScheduleUnlock() == OS_FALSE )
                {
                    OSSchedule();
                }
            }
            else
            {
                /* Try again. */
                OSSemStateUnlock( ptSem );
                ( void ) OSScheduleUnlock();
            }
        }
        else
        {
            OSSemStateUnlock( ptSem );
            ( void ) OSScheduleUnlock();
            
            if( OSSemIsEmpty( ptSem ) != OS_FALSE )
            {
                //the semaphore is empty
                return OS_FALSE;
            }
        }
    }
}

uOSBool_t OSSemPost( OSSemHandle_t SemHandle )
{
    uOSBool_t bEntryTimeSet = OS_FALSE;
    tOSTimeOut_t tTimeOut;
    tOSSem_t * const ptSem = ( tOSSem_t * ) SemHandle;

    uOSTick_t uxTicksToWait = SEMAPOST_BLOCK_TIME;

    for( ;; )
    {
        OSIntLock();
        {
            const uOSBase_t uxCurNum = ptSem->uxCurNum;
            
            if( uxCurNum < ptSem->uxMaxNum )
            {
                ptSem->uxCurNum = uxCurNum + 1;

                if( OSListIsEmpty( &( ptSem->tTaskListEventSemP ) ) == OS_FALSE )
                {
                    if( OSTaskListEventRemove( &( ptSem->tTaskListEventSemP ) ) != OS_FALSE )
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
                    //the semaphore is full
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

        /* Interrupts and other tasks can post to or pend from the semaphore
        To avoid confusion, we lock the scheduler and the semaphore. */
        OSScheduleLock();
        OSSemStateLock( ptSem );

        if( OSGetTimeOutState( &tTimeOut, &uxTicksToWait ) == OS_FALSE )
        {
            if( OSSemIsFull( ptSem ) != OS_FALSE )
            {
                OSTaskListEventAdd( &( ptSem->tTaskListEventSemV ), uxTicksToWait );

                OSSemStateUnlock( ptSem );

                if( OSScheduleUnlock() == OS_FALSE )
                {
                    OSSchedule();
                }
            }
            else
            {
                /* Try again. */
                OSSemStateUnlock( ptSem );
                ( void ) OSScheduleUnlock();
            }
        }
        else
        {
            /* The timeout has expired. */
            OSSemStateUnlock( ptSem );
            ( void ) OSScheduleUnlock();
            //the semaphore is full
            return OS_FALSE;
        }
    }
}

uOSBool_t OSSemPostFromISR( OSSemHandle_t SemHandle )
{
    uOSBool_t bReturn = OS_FALSE;
    uOSBase_t uxIntSave = (uOSBase_t)0U;
    tOSSem_t * const ptSem = ( tOSSem_t * ) SemHandle;

    uOSBool_t bNeedSchedule = OS_FALSE;

    uxIntSave = OSIntMaskFromISR();
    {
        const uOSBase_t uxCurNum = ptSem->uxCurNum;
        
        if( uxCurNum < ptSem->uxMaxNum )
        {
            const sOSBase_t xSemVLock = ptSem->xSemVLock;
    
            ptSem->uxCurNum = uxCurNum + 1;

            if( xSemVLock == SEM_STATUS_UNLOCKED )
            {
                if( OSListIsEmpty( &( ptSem->tTaskListEventSemP ) ) == OS_FALSE )
                {
                    if( OSTaskListEventRemove( &( ptSem->tTaskListEventSemP ) ) != OS_FALSE )
                    {
                        bNeedSchedule = OS_TRUE;
                    }
                }
            }
            else
            {
                ptSem->xSemVLock = ( sOSBase_t )(xSemVLock + 1);
            }

            bReturn = OS_TRUE;
        }
        else
        {
            //the semaphore is full
            bReturn = OS_FALSE;
        }
    }
    OSIntUnmaskFromISR( uxIntSave );

    if(SCHEDULER_RUNNING == OSScheduleGetState())
    {
        OSScheduleFromISR( bNeedSchedule );
    }

    return bReturn;
}

#endif //(OS_SEMAPHORE_ON!=0)

#ifdef __cplusplus
}
#endif
