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

#ifndef __OS_TIMER_H_
#define __OS_TIMER_H_

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ( OS_MSGQ_ON!=0 )
#if ( OS_TIMER_ON!=0 )

typedef struct tOSTimer
{
    tOSListItem_t            tTimerListItem;
    uOSTick_t                uxTimerTicks;
    uOS16_t                  bPeriod;
    OSTimerFunction_t        pxTimerFunction;            /* << The callback function to execute. */
    void *                   pvParameter;                /* << The value that will be used as the callback functions first parameter. */    
    sOS8_t                   pcTimerName[OSNAME_MAX_LEN];/* name of the timer*/
    sOSBase_t                xID;
}tOSTimer_t;

typedef    tOSTimer_t*       OSTimerHandle_t;

typedef struct tTimerCmdMsg
{
    sOSBase_t                xCmdMsgType;                /*<< The command message type being sent to the timer moniter task. */
    uOSTick_t                uxTicks;                    /*<< An optional value used by a timer commands, for example, when changing the period of a timer. */
    tOSTimer_t *             ptTimer;                    /*<< The timer to which the command will be applied. */
} tOSTimerCmdMsg_t;

uOSBase_t         OSTimerInit( void ) TINIUX_FUNCTION;

OSTimerHandle_t   OSTimerCreate(const uOSTick_t uxTimerTicks, const uOS16_t uiIsPeriod, const OSTimerFunction_t Function, void* pvParameter, sOS8_t* pcName) TINIUX_FUNCTION;
#if ( OS_MEMFREE_ON != 0 )
uOSBool_t         OSTimerDelete(OSTimerHandle_t TimerHandle) TINIUX_FUNCTION;
#endif /* OS_MEMFREE_ON */

uOSBool_t         OSTimerDeleteFromISR(OSTimerHandle_t TimerHandle) TINIUX_FUNCTION;
sOSBase_t         OSTimerSetID(OSTimerHandle_t TimerHandle, sOSBase_t xID) TINIUX_FUNCTION;
sOSBase_t         OSTimerGetID(OSTimerHandle_t const TimerHandle) TINIUX_FUNCTION;

uOSBool_t         OSTimerSetTicks(OSTimerHandle_t const TimerHandle, const uOSTick_t uxTimerTicks) TINIUX_FUNCTION;
uOSBool_t         OSTimerSetTicksFromISR(OSTimerHandle_t const TimerHandle, const uOSTick_t uxTimerTicks) TINIUX_FUNCTION;
uOSBool_t         OSTimerSetPeriod(OSTimerHandle_t const TimerHandle, const uOSTick_t uxTimerPeriod) TINIUX_FUNCTION;
uOSBool_t         OSTimerSetPeriodFromISR(OSTimerHandle_t const TimerHandle, const uOSTick_t uxTimerPeriod) TINIUX_FUNCTION;
uOSBool_t         OSTimerStart(OSTimerHandle_t const TimerHandle) TINIUX_FUNCTION;
uOSBool_t         OSTimerStartFromISR(OSTimerHandle_t const TimerHandle) TINIUX_FUNCTION;
uOSBool_t         OSTimerStop(OSTimerHandle_t const TimerHandle) TINIUX_FUNCTION;
uOSBool_t         OSTimerStopFromISR(OSTimerHandle_t const TimerHandle) TINIUX_FUNCTION;

uOSBool_t         OSTimerCreateMoniteTask( void ) TINIUX_FUNCTION;

#endif //( OS_TIMER_ON!=0 )
#endif//( OS_MSGQ_ON!=0 )

#ifdef __cplusplus
}
#endif

#endif //__OS_TIMER_H_
