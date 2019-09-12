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

#if ( OS_MSGQ_ON!=0 )
#if ( OS_TIMER_ON!=0 )

TINIUX_DATA static sOSBase_t const TMCMD_MSGQ_NO_DELAY          = ( ( sOSBase_t ) 0 );
TINIUX_DATA static sOSBase_t const TMCMD_MSGQ_LENGTH            = ( ( sOSBase_t ) 8 );

TINIUX_DATA static sOSBase_t const TMCMD_START                  = ( ( sOSBase_t ) 1 );
TINIUX_DATA static sOSBase_t const TMCMD_RESET                  = ( ( sOSBase_t ) 2 );
TINIUX_DATA static sOSBase_t const TMCMD_STOP                   = ( ( sOSBase_t ) 3 );
TINIUX_DATA static sOSBase_t const TMCMD_CHANGE_PERIOD          = ( ( sOSBase_t ) 4 );

#if ( OS_MEMFREE_ON != 0 )
TINIUX_DATA static sOSBase_t const TMCMD_DELETE                 = ( ( sOSBase_t ) 5 );
#endif /* OS_MEMFREE_ON */

TINIUX_DATA static sOSBase_t const TMCMD_FIRST_FROM_ISR_TYPE    = ( ( sOSBase_t ) 6 );
TINIUX_DATA static sOSBase_t const TMCMD_START_FROM_ISR         = ( ( sOSBase_t ) 6 );
TINIUX_DATA static sOSBase_t const TMCMD_RESET_FROM_ISR         = ( ( sOSBase_t ) 7 );
TINIUX_DATA static sOSBase_t const TMCMD_STOP_FROM_ISR          = ( ( sOSBase_t ) 8 );
TINIUX_DATA static sOSBase_t const TMCMD_CHANGE_PERIOD_FROM_ISR = ( ( sOSBase_t ) 9 );

#if ( OS_MEMFREE_ON != 0 )
TINIUX_DATA static sOSBase_t const TMCMD_DELETE_FROM_ISR        = ( ( sOSBase_t ) 10 );
#endif /* OS_MEMFREE_ON */

TINIUX_DATA static tOSList_t         gtOSTimerList1;
TINIUX_DATA static tOSList_t         gtOSTimerList2;
TINIUX_DATA static tOSList_t *        gptOSTimerList            = OS_NULL;
TINIUX_DATA static tOSList_t *        gptOSOFTimerList          = OS_NULL;

TINIUX_DATA static OSMsgQHandle_t     gOSTimerCmdMsgQHandle     = OS_NULL;
TINIUX_DATA static OSTaskHandle_t    gOSTimerMoniteTaskHandle   = OS_NULL;

uOSBase_t OSTimerInit( void )
{
    gptOSTimerList               = OS_NULL;
    gptOSOFTimerList             = OS_NULL;

    gOSTimerCmdMsgQHandle        = OS_NULL;
    gOSTimerMoniteTaskHandle     = OS_NULL;
    
    return 0U;
}

static void OSTimerInitListsAndCmdMsgQ( void )
{
    OSIntLock();
    {
        if( gOSTimerCmdMsgQHandle == OS_NULL )
        {
            OSListInit( &gtOSTimerList1 );
            OSListInit( &gtOSTimerList2 );
            gptOSTimerList = &gtOSTimerList1;
            gptOSOFTimerList = &gtOSTimerList2;

            gOSTimerCmdMsgQHandle = OSMsgQCreate(TMCMD_MSGQ_LENGTH, sizeof(tOSTimerCmdMsg_t));
        }
    }
    OSIntUnlock();
}

static void OSTimerInitTCB( OSTimerHandle_t NewTimerHandle,
                            const uOSBase_t uxTimerTicks, 
                            const uOS16_t   uiIsPeriod, 
                            const OSTimerFunction_t Function, 
                            void*           pvParameter, 
                            sOS8_t*         pcName )
{
    uOSBase_t x = ( uOSBase_t ) 0;
    
    if( NewTimerHandle != OS_NULL )
    {
        if( gOSTimerCmdMsgQHandle==OS_NULL )
        {
            OSTimerInitListsAndCmdMsgQ();
        }

        for( x = ( uOSBase_t ) 0; x < ( uOSBase_t ) OSNAME_MAX_LEN; x++ )
        {
            NewTimerHandle->pcTimerName[ x ] = pcName[ x ];
            if( pcName[ x ] == 0x00 )
            {
                break;
            }
        }
        NewTimerHandle->pcTimerName[ OSNAME_MAX_LEN - 1 ] = '\0';

        if( uiIsPeriod > 0 )
        {
            NewTimerHandle->bPeriod     = OS_TRUE;
        }
        else
        {
            NewTimerHandle->bPeriod     = OS_FALSE;
        }

        NewTimerHandle->uxTimerTicks     = uxTimerTicks;
        NewTimerHandle->pxTimerFunction  = Function;
        NewTimerHandle->pvParameter      = pvParameter;

        OSListItemInitialise( &( NewTimerHandle->tTimerListItem ) );
    }
}

OSTimerHandle_t OSTimerCreate(const uOSTick_t uxTimerTicks, const uOS16_t uiIsPeriod, const OSTimerFunction_t Function, void* pvParameter, sOS8_t* pcName)
{
    OSTimerHandle_t TimerHandle = OS_NULL;

    if(uxTimerTicks == (uOSTick_t)0U)
    {
        return OS_NULL;
    }
    TimerHandle = (OSTimerHandle_t)OSMemMalloc(sizeof(tOSTimer_t));
    if (TimerHandle != OS_NULL) 
    {
        OSTimerInitTCB( TimerHandle, uxTimerTicks, uiIsPeriod, Function, pvParameter, pcName);
    }    

    return TimerHandle;
}

uOSBool_t OSTimerSendCmdMsg( OSTimerHandle_t xTimer, const sOSBase_t xCmdMsgType, const uOSTick_t xOptionalValue, const uOSTick_t uxTicksToWait )
{
    uOSBool_t bReturn = OS_FALSE;
    tOSTimerCmdMsg_t tCmdMsg;

    if( gOSTimerCmdMsgQHandle != OS_NULL )
    {
        tCmdMsg.xCmdMsgType     = xCmdMsgType;
        tCmdMsg.uxTicks         = xOptionalValue;
        tCmdMsg.ptTimer         = ( tOSTimer_t * ) xTimer;

        if( xCmdMsgType < TMCMD_FIRST_FROM_ISR_TYPE )
        {
            if( OSScheduleGetState() == SCHEDULER_RUNNING )
            {
                bReturn = OSMsgQSend( gOSTimerCmdMsgQHandle, &tCmdMsg, uxTicksToWait );
            }
            else
            {
                bReturn = OSMsgQSend( gOSTimerCmdMsgQHandle, &tCmdMsg, TMCMD_MSGQ_NO_DELAY );
            }
        }
        else
        {
            bReturn = OSMsgQSendFromISR( gOSTimerCmdMsgQHandle, &tCmdMsg );
        }
    }

    return bReturn;
}

static void OSTimerListSwitch( void )
{
    uOSTick_t uxNextExpireTime =(uOSTick_t)0U, xReloadTime = (uOSTick_t)0U;
    tOSList_t *pxTempList = OS_NULL;
    tOSTimer_t *ptTimer = OS_NULL;
    uOSBool_t bReturn = OS_FALSE;

    while( OSListIsEmpty( gptOSTimerList ) == OS_FALSE )
    {
        uxNextExpireTime = OSlistGetHeadItemValue( gptOSTimerList );

        ptTimer = ( tOSTimer_t * ) OSListGetHeadItemHolder( gptOSTimerList );
        ( void ) OSListRemoveItem( &( ptTimer->tTimerListItem ) );

        ptTimer->pxTimerFunction( ptTimer->pvParameter );

        if( ptTimer->bPeriod == ( sOSBase_t ) OS_TRUE )
        {
            xReloadTime = ( uxNextExpireTime + ptTimer->uxTimerTicks );
            if( xReloadTime > uxNextExpireTime )
            {
                OSListItemSetValue( &( ptTimer->tTimerListItem ), xReloadTime );
                OSListItemSetHolder( &( ptTimer->tTimerListItem ), ptTimer );
                OSListInsertItem( gptOSTimerList, &( ptTimer->tTimerListItem ) );
            }
            else
            {
                bReturn = OSTimerSendCmdMsg( ptTimer, TMCMD_START, uxNextExpireTime, TMCMD_MSGQ_NO_DELAY );
                ( void ) bReturn;
            }
        }
    }

    pxTempList = gptOSTimerList;
    gptOSTimerList = gptOSOFTimerList;
    gptOSOFTimerList = pxTempList;
}

static uOSTick_t OSTimerGetCurTime( uOSBool_t * const pbTimerListsSwitched )
{
    uOSTick_t uxTimeNow = ( uOSTick_t ) 0U;
    TINIUX_DATA static uOSTick_t uxLastTime = ( uOSTick_t ) 0U; 

    uxTimeNow = OSGetTickCount();

    if( uxTimeNow < uxLastTime )
    {
        OSTimerListSwitch();
        *pbTimerListsSwitched = OS_TRUE;
    }
    else
    {
        *pbTimerListsSwitched = OS_FALSE;
    }

    uxLastTime = uxTimeNow;

    return uxTimeNow;
}

static uOSBool_t OSTimerAddToList( tOSTimer_t * const ptTimer, const uOSTick_t uxNextExpiryTime, const uOSTick_t uxTimeNow, const uOSTick_t uxCommandTime )
{
    uOSBool_t bProcessTimerNow = OS_FALSE;

    OSListItemSetValue( &( ptTimer->tTimerListItem ), uxNextExpiryTime );
    OSListItemSetHolder( &( ptTimer->tTimerListItem ), ptTimer );

    if( uxNextExpiryTime <= uxTimeNow )
    {
        if( ( ( uOSTick_t ) ( uxTimeNow - uxCommandTime ) ) >= ptTimer->uxTimerTicks ) 
        {
            bProcessTimerNow = OS_TRUE;
        }
        else
        {
            OSListInsertItem( gptOSOFTimerList, &( ptTimer->tTimerListItem ) );
        }
    }
    else
    {
        if( ( uxTimeNow < uxCommandTime ) && ( uxNextExpiryTime >= uxCommandTime ) )
        {
            bProcessTimerNow = OS_TRUE;
        }
        else
        {
            OSListInsertItem( gptOSTimerList, &( ptTimer->tTimerListItem ) );
        }
    }

    return bProcessTimerNow;
}

static void OSTimerExpiredProcess( const uOSTick_t uxNextExpireTime, const uOSTick_t uxTimeNow )
{
    uOSBool_t bReturn = OS_FALSE;
    tOSTimer_t * const ptTimer = ( tOSTimer_t * ) OSListGetHeadItemHolder( gptOSTimerList );

    ( void ) OSListRemoveItem( &( ptTimer->tTimerListItem ) );

    if( ptTimer->bPeriod == ( sOSBase_t ) OS_TRUE )
    {
        if( OSTimerAddToList( ptTimer, ( uxNextExpireTime + ptTimer->uxTimerTicks ), uxTimeNow, uxNextExpireTime ) != OS_FALSE )
        {
            bReturn = OSTimerSendCmdMsg( ptTimer, TMCMD_START, uxNextExpireTime, TMCMD_MSGQ_NO_DELAY );
            ( void ) bReturn;
        }
    }

    ptTimer->pxTimerFunction( ptTimer->pvParameter );
}

static void OSTimerProcessOrBlock( const uOSTick_t uxNextExpireTime, uOSBool_t bListWasEmpty )
{
    uOSTick_t uxTimeNow = (uOSTick_t)0U;
    uOSBool_t bTimerListsSwitched = OS_FALSE;

    OSScheduleLock();
    {
        uxTimeNow = OSTimerGetCurTime( &bTimerListsSwitched );
        if( bTimerListsSwitched == OS_FALSE )
        {
            if( ( bListWasEmpty == OS_FALSE ) && ( uxNextExpireTime <= uxTimeNow ) )
            {
                ( void ) OSScheduleUnlock();
                OSTimerExpiredProcess( uxNextExpireTime, uxTimeNow );
            }
            else
            {
                if( bListWasEmpty != OS_FALSE )
                {
                    bListWasEmpty = OSListIsEmpty( gptOSOFTimerList );
                }

                OSMsgQWait( gOSTimerCmdMsgQHandle, ( uxNextExpireTime - uxTimeNow ), bListWasEmpty );

                if( OSScheduleUnlock() == OS_FALSE )
                {
                    OSSchedule();
                }
            }
        }
        else
        {
            ( void ) OSScheduleUnlock();
        }
    }
}

static uOSTick_t OSTimerGetNextExpireTime( uOSBool_t * const pbListWasEmpty )
{
    uOSTick_t uxNextExpireTime = (uOSTick_t)0U;

    *pbListWasEmpty = OSListIsEmpty( gptOSTimerList );
    if( *pbListWasEmpty == OS_FALSE )
    {
        uxNextExpireTime = OSlistGetHeadItemValue( gptOSTimerList );
    }
    else
    {
        uxNextExpireTime = ( uOSTick_t ) 0U;
    }

    return uxNextExpireTime;
}

static void OSTimerReceiveCmdMsg( void )
{
    tOSTimerCmdMsg_t tCmdMsg;
    tOSTimer_t *ptTimer = OS_NULL;
    uOSBool_t bTimerListsSwitched = OS_FALSE;
    uOSBool_t bReturn = OS_FALSE;
    uOSTick_t uxTimeNow = (uOSTick_t)0U;

    while( OSMsgQReceive( gOSTimerCmdMsgQHandle, &tCmdMsg, TMCMD_MSGQ_NO_DELAY ) != OS_FALSE ) 
    {

        if( tCmdMsg.xCmdMsgType >= ( sOSBase_t ) 0 )
        {
            ptTimer = tCmdMsg.ptTimer;

            if( OSListContainListItem( OS_NULL, &( ptTimer->tTimerListItem ) ) == OS_FALSE )
            {
                ( void ) OSListRemoveItem( &( ptTimer->tTimerListItem ) );
            }

            uxTimeNow = OSTimerGetCurTime( &bTimerListsSwitched );

            if( tCmdMsg.xCmdMsgType==TMCMD_START || tCmdMsg.xCmdMsgType==TMCMD_START_FROM_ISR ||
                tCmdMsg.xCmdMsgType==TMCMD_RESET || tCmdMsg.xCmdMsgType==TMCMD_RESET_FROM_ISR )
            {
                if( OSTimerAddToList( ptTimer,  tCmdMsg.uxTicks + ptTimer->uxTimerTicks, uxTimeNow, tCmdMsg.uxTicks ) != OS_FALSE )
                {
                    ptTimer->pxTimerFunction( ptTimer->pvParameter );

                    if( ptTimer->bPeriod == ( sOSBase_t ) OS_TRUE )
                    {
                        bReturn = OSTimerSendCmdMsg( ptTimer, TMCMD_START, tCmdMsg.uxTicks + ptTimer->uxTimerTicks, TMCMD_MSGQ_NO_DELAY );
                        ( void ) bReturn;
                    }
                }
            }
            else if( tCmdMsg.xCmdMsgType==TMCMD_STOP || tCmdMsg.xCmdMsgType==TMCMD_STOP_FROM_ISR)
            {/* The timer has already been removed from the active list. */
            }
            else if( tCmdMsg.xCmdMsgType==TMCMD_CHANGE_PERIOD || tCmdMsg.xCmdMsgType==TMCMD_CHANGE_PERIOD_FROM_ISR)
            {
                ptTimer->uxTimerTicks = tCmdMsg.uxTicks;

                ( void ) OSTimerAddToList( ptTimer, ( uxTimeNow + ptTimer->uxTimerTicks ), uxTimeNow, uxTimeNow );                    
            }
            #if ( OS_MEMFREE_ON != 0 )
            else if( tCmdMsg.xCmdMsgType==TMCMD_DELETE || tCmdMsg.xCmdMsgType==TMCMD_DELETE_FROM_ISR)
            {/* The timer has already been removed from the active list. */
                OSMemFree(ptTimer);                
            }
            #endif /* OS_MEMFREE_ON */
        }
    }
}


static void OSTimerMoniteTask( void *pvParameters)
{
    uOSTick_t uxNextExpireTime = (uOSTick_t)0U;
    uOSBool_t bListWasEmpty = OS_FALSE;

    ( void ) pvParameters;

    for( ;; )
    {
        /* Query the timers list to see if it contains any timers, and if so,
        obtain the time at which the next timer will expire. */
        uxNextExpireTime = OSTimerGetNextExpireTime( &bListWasEmpty );

        /* If a timer has expired, process it.  Otherwise, block this task
        until either a timer does expire, or a command is received. */
        OSTimerProcessOrBlock( uxNextExpireTime, bListWasEmpty );

        /* Receive the command queue. */
        OSTimerReceiveCmdMsg();
    }
}

uOSBool_t OSTimerCreateMoniteTask( void )
{
    uOSBool_t bReturn = OS_FALSE;

    if( gOSTimerCmdMsgQHandle == OS_NULL )
    {
        OSTimerInitListsAndCmdMsgQ();
    }
        
    if( (gOSTimerCmdMsgQHandle != OS_NULL) && (gOSTimerMoniteTaskHandle == OS_NULL))
    {
        gOSTimerMoniteTaskHandle = OSTaskCreate(OSTimerMoniteTask, OS_NULL, OSMINIMAL_STACK_SIZE, OSCALLBACK_TASK_PRIO, "SRCbMsgTask" );
    }
    
    if( gOSTimerMoniteTaskHandle != OS_NULL )
    {
        bReturn = OS_TRUE;
    }

    return bReturn;
}

#if ( OS_MEMFREE_ON != 0 )
uOSBool_t OSTimerDelete(OSTimerHandle_t TimerHandle)
{
    return OSTimerSendCmdMsg( TimerHandle, TMCMD_DELETE, 0U, OSPEND_FOREVER_VALUE );
}
uOSBool_t OSTimerDeleteFromISR(OSTimerHandle_t TimerHandle)
{
    return OSTimerSendCmdMsg( TimerHandle, TMCMD_DELETE_FROM_ISR, 0U, 0U );
}
#endif /* OS_MEMFREE_ON */

uOSBool_t OSTimerSetTicks(OSTimerHandle_t const TimerHandle, const uOSTick_t uxTimerTicks)
{
    if( uxTimerTicks==(uOSTick_t)0U )
    {
        return OS_FALSE;
    }
    else
    {
        return OSTimerSendCmdMsg( TimerHandle, TMCMD_CHANGE_PERIOD, uxTimerTicks, OSPEND_FOREVER_VALUE );
    }
}
uOSBool_t OSTimerSetTicksFromISR(OSTimerHandle_t const TimerHandle, const uOSTick_t uxTimerTicks)
{
    if( uxTimerTicks==(uOSTick_t)0U )
    {
        return OS_FALSE;
    }
    else
    {
        return OSTimerSendCmdMsg( TimerHandle, TMCMD_CHANGE_PERIOD_FROM_ISR, uxTimerTicks, 0U );
    }
}

uOSBool_t OSTimerSetPeriod(OSTimerHandle_t const TimerHandle, const uOSTick_t uxTimerPeriod)
{
    if( uxTimerPeriod==(uOSTick_t)0U )
    {
        return OS_FALSE;
    }
    else
    {
        return OSTimerSendCmdMsg( TimerHandle, TMCMD_CHANGE_PERIOD, uxTimerPeriod, OSPEND_FOREVER_VALUE );
    }
}
uOSBool_t OSTimerSetPeriodFromISR(OSTimerHandle_t const TimerHandle, const uOSTick_t uxTimerPeriod)
{
    if( uxTimerPeriod==(uOSTick_t)0U )
    {
        return OS_FALSE;
    }
    else
    {
        return OSTimerSendCmdMsg( TimerHandle, TMCMD_CHANGE_PERIOD_FROM_ISR, uxTimerPeriod, 0U );
    }
}

uOSBool_t OSTimerStart(OSTimerHandle_t const TimerHandle)
{
    return OSTimerSendCmdMsg( TimerHandle, TMCMD_START, ( OSGetTickCount() ), OSPEND_FOREVER_VALUE );
}
uOSBool_t OSTimerStartFromISR(OSTimerHandle_t const TimerHandle)
{
    return OSTimerSendCmdMsg( TimerHandle, TMCMD_START_FROM_ISR, ( OSGetTickCount() ), 0U );
}

uOSBool_t OSTimerStop(OSTimerHandle_t const TimerHandle)
{
    return OSTimerSendCmdMsg( TimerHandle, TMCMD_STOP, 0U, OSPEND_FOREVER_VALUE );
}
uOSBool_t OSTimerStopFromISR(OSTimerHandle_t const TimerHandle)
{
    return OSTimerSendCmdMsg( TimerHandle, TMCMD_STOP_FROM_ISR, 0U, 0U );
}

sOSBase_t OSTimerSetID(OSTimerHandle_t TimerHandle, sOSBase_t xID)
{
    if(TimerHandle == OS_NULL)
    {
        return 1;
    }
    OSIntLock();
    {
        TimerHandle->xID = xID;
    }
    OSIntUnlock();

    return 0;
}
sOSBase_t OSTimerGetID(OSTimerHandle_t const TimerHandle)
{
    sOSBase_t xID = 0;
    
    OSIntLock();
    if(TimerHandle != OS_NULL)
    {
        xID = TimerHandle->xID;
    }
    OSIntUnlock();

    return xID;
}

#endif //( OS_TIMER_ON!=0 )
#endif//( OS_MSGQ_ON!=0 )
    
#ifdef __cplusplus
}
#endif

