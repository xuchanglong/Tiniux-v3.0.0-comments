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

#if ( FITQUICK_GET_PRIORITY == 1U )
    TINIUX_DATA static volatile  uOSBase_t guxTopReadyPriority  = OSLOWEAST_PRIORITY;
#else
    #if ( OSQUICK_GET_PRIORITY != 0U )
        #if ( OSQUICK_GET_PRIORITY == 1U )
            #define SUBPRI_MAXNUM                       ( 2 )
            #define SUBPRI_TOPPRI                       ( 1 )
            #define SUBPRI_MASK_WIDTH                   ( 1 )
            #define SUBPRI_MASK_VALUE                   ( 1 )
            #define SUBPRI_BITMAP_WIDTH                 ( 2 )
            #define SUBPRI_BITMAP_MAXNUM                ( 4 )
            #define SUBPRI_BITMAP_TOPBIT                ( 0x02 )
        #else
            #if ( OSQUICK_GET_PRIORITY == 2U )
                #define SUBPRI_MAXNUM                   ( 4 )
                #define SUBPRI_TOPPRI                   ( 3 )
                #define SUBPRI_MASK_WIDTH               ( 2 )
                #define SUBPRI_MASK_VALUE               ( 3 )
                #define SUBPRI_BITMAP_WIDTH             ( 4 )
                #define SUBPRI_BITMAP_MAXNUM            ( 16 )
                #define SUBPRI_BITMAP_TOPBIT            ( 0x08 )
            #else //( OSQUICK_GET_PRIORITY == 3U )
                #define SUBPRI_MAXNUM                   ( 8 )
                #define SUBPRI_TOPPRI                   ( 7 )
                #define SUBPRI_MASK_WIDTH               ( 3 )
                #define SUBPRI_MASK_VALUE               ( 7 )
                #define SUBPRI_BITMAP_WIDTH             ( 8 )
                #define SUBPRI_BITMAP_MAXNUM            ( 256 )
                #define SUBPRI_BITMAP_TOPBIT            ( 0x80 )
            #endif
        #endif
        TINIUX_DATA static uOS8_t gucSubPriorityMap[SUBPRI_BITMAP_MAXNUM];
        TINIUX_DATA static volatile uOS8_t gucSubPriorityBit[SUBPRI_MAXNUM];
        TINIUX_DATA static volatile uOS8_t gucSubPriorityGroupBit  = 0U;
    #else
    //default do nothing
    #endif
#endif

TINIUX_DATA static volatile  uOSBool_t gbSchedulerRunning       = OS_FALSE;
TINIUX_DATA static volatile  uOSBase_t guxSchedulerLocked       = ( uOSBase_t ) OS_FALSE;
TINIUX_DATA static volatile  uOSBool_t gbNeedSchedule           = OS_FALSE;
TINIUX_DATA static volatile  uOSTick_t guxTickCount             = ( uOSTick_t ) 0U;
TINIUX_DATA static volatile  sOSBase_t gxOverflowCount          = ( sOSBase_t ) 0U;
TINIUX_DATA static volatile  uOSBase_t guxPendedTicks           = ( uOSBase_t ) 0U;
TINIUX_DATA static volatile  uOSTick_t guxNextUnblockTime       = ( uOSTick_t ) 0U;

uOSBase_t OSInit( void )
{
    uOSBase_t uxReturn = 0;

    uxReturn += OSMemInit( );  
    uxReturn += OSScheduleInit( );  
    uxReturn += OSTaskInit( );
    
#if ( OS_TIMER_ON!=0 )
    uxReturn += OSTimerInit( );
#endif
    
    return uxReturn;
}

uOSBase_t OSScheduleInit( void )
{

#if ( FITQUICK_GET_PRIORITY == 1U )
    guxTopReadyPriority         = OSLOWEAST_PRIORITY;
#else
#if ( OSQUICK_GET_PRIORITY != 0U )
    uOS16_t uiSubPriIndex = 0;
    uOS16_t uiBitIndex = 0;

    for (uiSubPriIndex = 0; uiSubPriIndex < SUBPRI_MAXNUM; uiSubPriIndex++)
    {
        gucSubPriorityBit[uiSubPriIndex] = 0;
    }
    for (uiSubPriIndex = 0; uiSubPriIndex < SUBPRI_BITMAP_MAXNUM; uiSubPriIndex++)
    {
        gucSubPriorityMap[uiSubPriIndex] = 0;
        for (uiBitIndex = 0; uiBitIndex < SUBPRI_BITMAP_WIDTH; uiBitIndex++)
        {
            if (((uiSubPriIndex << uiBitIndex) & SUBPRI_BITMAP_TOPBIT) != 0)
            {
                gucSubPriorityMap[uiSubPriIndex] = SUBPRI_TOPPRI - uiBitIndex;
                break;
            }
        }
    }
#else
    //Default do nothing
#endif
#endif
    
    guxTickCount                = ( uOSTick_t ) 0U;
    gbSchedulerRunning          = OS_FALSE;
    guxPendedTicks              = ( uOSBase_t ) 0U;
    gbNeedSchedule              = OS_FALSE;
    guxSchedulerLocked          = ( uOSBase_t ) OS_FALSE;
    guxNextUnblockTime          = ( uOSTick_t ) 0U;
    gxOverflowCount             = ( sOSBase_t ) 0U;

    return 0;
}

uOSBase_t OSStart( void )
{
    uOSBase_t ReturnValue = (uOSBase_t)0U;
    OSTaskHandle_t TaskHandle = OS_NULL;

    TaskHandle = OSTaskCreate(OSIdleTask, OS_NULL, OSMINIMAL_STACK_SIZE, OSLOWEAST_PRIORITY, "OSIdleTask");
    if(TaskHandle != OS_NULL)
    {
#if ( OS_TIMER_ON!=0 )
        OSTimerCreateMoniteTask();
#endif /* ( OS_TIMER_ON!=0 ) */        
        
        guxNextUnblockTime = OSPEND_FOREVER_VALUE;
        gbSchedulerRunning = OS_TRUE;
        guxTickCount = ( uOSTick_t ) 0U;

        FitStartScheduler();
    }
    else
    {
        ReturnValue = 1U;
    }

    // Should not get here!
    return ReturnValue;
}

static void OSTickCountOverflow( void )
{
    OSTaskListPendSwitch();
    gxOverflowCount++;
    OSUpdateUnblockTime();
}

uOSBool_t OSIncrementTickCount( void )
{
    tOSTCB_t * ptTCB = OS_NULL;
    uOSTick_t uxItemValue = (uOSTick_t)0U;
    uOSBool_t bNeedSchedule = OS_FALSE;

    if( OSScheduleIsLocked() == OS_FALSE )
    {
        const uOSTick_t uxTickCount = guxTickCount + (uOSTick_t)1;
        guxTickCount = uxTickCount;

        if( uxTickCount == ( uOSTick_t ) 0U )
        {
            OSTickCountOverflow();
        }

        if( uxTickCount >= guxNextUnblockTime )
        {
            for( ;; )
            {
                if( OSTaskListPendNum() == 0U )
                {
                    guxNextUnblockTime = OSPEND_FOREVER_VALUE;
                    break;
                }
                else
                {
                    ptTCB = ( tOSTCB_t * ) OSTaskListPendHeadItem();
                    uxItemValue = OSListItemGetValue( &( ptTCB->tTaskListItem ) );

                    if( uxTickCount < uxItemValue )
                    {
                        guxNextUnblockTime = uxItemValue;
                        break;
                    }

                    ( void ) OSListRemoveItem( &( ptTCB->tTaskListItem ) );

                    if( OSListItemGetList( &( ptTCB->tEventListItem ) ) != OS_NULL )
                    {
                        ( void ) OSListRemoveItem( &( ptTCB->tEventListItem ) );
                    }

                    OSTaskListReadyAdd( ptTCB );

                    if( ptTCB->uxPriority >= OSTaskGetPriorityFromISR( OS_NULL ) )
                    {
                        bNeedSchedule = OS_TRUE;
                    }
                }
            }
        }
        #if (OSTIME_SLICE_ON != 0U)
        if( OSTaskNeedTimeSlice() == OS_TRUE )
        {
            bNeedSchedule = OS_TRUE;
        }
        #endif //(OSTIME_SLICE_ON != 0U)
    }
    else
    {
        ++guxPendedTicks;
    }

    if( gbNeedSchedule != OS_FALSE )
    {
        bNeedSchedule = OS_TRUE;
    }

    return bNeedSchedule;
}

uOSTick_t OSGetTickCount( void )
{
    uOSTick_t uxTicks = (uOSTick_t)0U;

    OSIntLock();
    uxTicks = guxTickCount;
    OSIntUnlock();
    
    return uxTicks;
}

uOSTick_t OSGetTickCountFromISR( void )
{
    uOSTick_t uxTicks = (uOSTick_t)0U;
    uOSBase_t uxIntSave = (uOSBase_t)0U;

    uxIntSave = OSIntMaskFromISR();
    uxTicks = guxTickCount;
    OSIntUnmaskFromISR( uxIntSave );
    
    return uxTicks;
}

void OSNeedSchedule( void )
{
    gbNeedSchedule = OS_TRUE;
}

void OSScheduleLock( void )
{
    ++guxSchedulerLocked;
}

uOSBool_t OSScheduleUnlock( void )
{
    tOSTCB_t *ptTCB = OS_NULL;
    uOSBool_t bAlreadyScheduled = OS_FALSE;

    OSIntLock();
    {
        --guxSchedulerLocked;

        if( OSScheduleIsLocked() == OS_FALSE )
        {
            if(OSTaskGetCurrentTaskNum() > 0U)
            {       
                while( OSTaskListReadyPoolNum() != 0U )
                {
                    ptTCB = ( tOSTCB_t * ) OSTaskListReadyPoolHeadItem();
                    ( void ) OSListRemoveItem( &( ptTCB->tEventListItem ) );
                    ( void ) OSListRemoveItem( &( ptTCB->tTaskListItem ) );
                    OSTaskListReadyAdd( ptTCB );

                    if( ptTCB->uxPriority >= OSTaskGetPriority( OS_NULL ) )
                    {
                        gbNeedSchedule = OS_TRUE;
                    }
                }
                if( ptTCB != OS_NULL )
                {
                    OSUpdateUnblockTime();
                }

                {
                    uOSBase_t uxPendedTicks = guxPendedTicks;
                    while( uxPendedTicks > ( uOSBase_t ) 0U )
                    {
                        if( OSIncrementTickCount() != OS_FALSE )
                        {
                            gbNeedSchedule = OS_TRUE;
                        }
                        --uxPendedTicks;
                    }
                    guxPendedTicks = 0;
                }

                if( gbNeedSchedule != OS_FALSE )
                {
                    bAlreadyScheduled = OS_TRUE;
                    OSSchedule();
                }
            }
        }
    }
    OSIntUnlock();

    return bAlreadyScheduled;
}

uOSBool_t OSScheduleIsLocked( void )
{
    return (uOSBool_t)guxSchedulerLocked;
}

sOSBase_t OSScheduleGetState( void )
{
    sOSBase_t xReturn = SCHEDULER_NOT_STARTED;

    if( gbSchedulerRunning == OS_FALSE )
    {
        xReturn = SCHEDULER_NOT_STARTED;
    }
    else
    {
        if( OSScheduleIsLocked() == OS_FALSE )
        {
            xReturn = SCHEDULER_RUNNING;
        }
        else
        {
            xReturn = SCHEDULER_LOCKED;
        }
    }

    return xReturn;
}

#if ( OS_LOWPOWER_ON!=0 )
void OSFixTickCount( const uOSTick_t uxTicksToFix )
{
    const uOSTick_t uxTickCount = guxTickCount;
    const uOSTick_t uxNextUnblockTime = guxNextUnblockTime;
    
    /* Correct the tick count value after a period during which the tick
    was suppressed. */
    if( ( uxTickCount + uxTicksToFix ) <= uxNextUnblockTime )
    {
        guxTickCount += uxTicksToFix;
    }
}

uOSBool_t OSEnableLowPowerIdle( void )
{
    /* The idle task exists in addition to the application tasks. */
    uOSBool_t bReturn = OS_TRUE;

    /* If a context switch is pending or a task is waiting for the scheduler
    to be unsuspended then abandon the low power entry. */
    if( OSTaskListReadyPoolNum() != 0 )
    {
        /* A task was made ready while the scheduler was suspended. */
        bReturn = OS_FALSE;
    }
    else if( gbNeedSchedule != OS_FALSE )
    {
        /* A schedule was pended while the scheduler was suspended. */
        bReturn = OS_FALSE;
    }

    return bReturn;
}

uOSTick_t OSGetBlockTickCount( void )
{
    uOSTick_t xReturn = (uOSTick_t)0U;
    const uOSTick_t uxTickCount = guxTickCount;
    const uOSTick_t uxNextUnblockTime = guxNextUnblockTime;
    
    if( OSTaskGetPriority( OS_NULL ) > OSLOWEAST_PRIORITY )
    {
        xReturn = (uOSTick_t)0U;
    }
    else if( OSTaskListReadyNum( OSLOWEAST_PRIORITY ) > 1 )
    {
        /* There are other idle priority tasks in the ready state.  If
        time slicing is used then the very next tick interrupt must be
        processed. */
        xReturn = (uOSTick_t)0U;
    }
    else
    {
        xReturn = uxNextUnblockTime - uxTickCount;
    }

    return xReturn;
}
#endif //OS_LOWPOWER_ON

void OSSetTimeOutState( tOSTimeOut_t * const ptTimeOut )
{
    ptTimeOut->xOverflowCount = gxOverflowCount;
    ptTimeOut->uxTimeOnEntering = guxTickCount;
}

uOSBool_t OSGetTimeOutState( tOSTimeOut_t * const ptTimeOut, uOSTick_t * const puxTicksToWait )
{
    uOSBool_t bReturn = OS_FALSE;

    OSIntLock();
    {
        const uOSTick_t uxTickCount = guxTickCount;
        const uOSTick_t uxElapsedTime = uxTickCount - ptTimeOut->uxTimeOnEntering;

        if( *puxTicksToWait == OSPEND_FOREVER_VALUE )
        {
            bReturn = OS_FALSE;
        }
        else if( ( gxOverflowCount != ptTimeOut->xOverflowCount ) && ( uxTickCount >= ptTimeOut->uxTimeOnEntering ) )
        {
            bReturn = OS_TRUE;
        }
        else if( uxElapsedTime < *puxTicksToWait )
        {
            *puxTicksToWait -= uxElapsedTime;
            OSSetTimeOutState( ptTimeOut );
            bReturn = OS_FALSE;
        }
        else
        {
            *puxTicksToWait = 0;
            bReturn = OS_TRUE;
        }
    }
    OSIntUnlock();

    return bReturn;
}

void OSUpdateUnblockTime( void )
{
    tOSTCB_t *ptTCB = OS_NULL;

    if( OSTaskListPendNum() == 0U )
    {
        guxNextUnblockTime = OSPEND_FOREVER_VALUE;
    }
    else
    {
        ( ptTCB ) = ( tOSTCB_t * ) OSTaskListPendHeadItem();
        guxNextUnblockTime = OSListItemGetValue( &( ( ptTCB )->tTaskListItem ) );
    }
}

void OSSetReadyPriority(uOSBase_t uxPriority)
{
#if ( FITQUICK_GET_PRIORITY == 1U )
    guxTopReadyPriority |= ( 1UL << ( uxPriority ) ) ;
#else
#if ( OSQUICK_GET_PRIORITY != 0U )
    uOS16_t iSubPriorityGroup = uxPriority >> SUBPRI_MASK_WIDTH;
    uOS16_t iPriorityRemainder = uxPriority & SUBPRI_MASK_VALUE;
    gucSubPriorityBit[iSubPriorityGroup] |= 1U << iPriorityRemainder;
    gucSubPriorityGroupBit |= 1U << iSubPriorityGroup;
#else
    //Default do nothing
#endif
#endif

    /* Just to avoid compiler warnings. */
    ( void ) uxPriority;
}

void OSResetReadyPriority(uOSBase_t uxPriority)
{
#if ( FITQUICK_GET_PRIORITY == 1U )
    if( OSTaskListReadyNum( uxPriority ) == ( uOSBase_t ) 0 )
    {
        guxTopReadyPriority &= ~( 1UL << ( uxPriority ) );
    }
#else
#if ( OSQUICK_GET_PRIORITY != 0U )
    if( OSTaskListReadyNum( uxPriority ) == ( uOSBase_t ) 0 )
    {
        uOS16_t iSubPriorityGroup = uxPriority >> SUBPRI_MASK_WIDTH;
        uOS16_t iPriorityRemainder = uxPriority & SUBPRI_MASK_VALUE;
        gucSubPriorityBit[iSubPriorityGroup] &= ~(1U << iPriorityRemainder);
        if (gucSubPriorityBit[iSubPriorityGroup] == 0U)
        {
            gucSubPriorityGroupBit &= ~(1U << iSubPriorityGroup);
        }
    }
#else
    //Default do nothing
#endif
#endif

    /* Just to avoid compiler warnings. */
    ( void ) uxPriority;
}

uOSBase_t OSGetTopReadyPriority( void )
{
    uOSBase_t uxTopPriority = OSHIGHEAST_PRIORITY - 1;

#if ( FITQUICK_GET_PRIORITY == 1U )
    FitGET_HIGHEST_PRIORITY( uxTopPriority, guxTopReadyPriority );
#else
#if ( OSQUICK_GET_PRIORITY != 0U )
    uOS16_t iSubPriorityGroup = gucSubPriorityMap[gucSubPriorityGroupBit];
    uOS16_t iPriorityRemainderBit = gucSubPriorityBit[iSubPriorityGroup];
    uxTopPriority = gucSubPriorityMap[iPriorityRemainderBit] + (iSubPriorityGroup << SUBPRI_MASK_WIDTH);
#else
    /* Default, Find the highest priority queue that contains ready tasks. */
    while( OSTaskListReadyNum( uxTopPriority ) == 0U )
    {
        --uxTopPriority;
    }
#endif
#endif
    return uxTopPriority;
}

#ifdef __cplusplus
}
#endif

