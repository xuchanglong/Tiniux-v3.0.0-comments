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

#ifndef __OS_SCHEDULE_H_
#define __OS_SCHEDULE_H_

#include "OSType.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCHEDULER_LOCKED                    ( ( sOSBase_t ) 0 )
#define SCHEDULER_NOT_STARTED               ( ( sOSBase_t ) 1 )
#define SCHEDULER_RUNNING                   ( ( sOSBase_t ) 2 )

#define OSIntLock()                         FitIntLock()
#define OSIntUnlock()                       FitIntUnlock()

#define OSIntMaskFromISR()                  FitIntMaskFromISR()
#define OSIntUnmaskFromISR( x )             FitIntUnmaskFromISR( x )

#define OSIntMask()                         FitIntMask()
#define OSIntUnmask( x )                    FitIntUnmask( x )

#define OSSchedule()                        FitSchedule()
#define OSScheduleFromISR( b )              FitScheduleFromISR( b )

#define OSIsInsideISR()                     FitIsInsideISR()

uOSBase_t    OSInit( void ) TINIUX_FUNCTION;
uOSBase_t    OSStart( void ) TINIUX_FUNCTION;

uOSBase_t    OSScheduleInit( void ) TINIUX_FUNCTION;
void         OSScheduleLock( void ) TINIUX_FUNCTION;
uOSBool_t    OSScheduleUnlock( void ) TINIUX_FUNCTION;
uOSBool_t    OSScheduleIsLocked( void ) TINIUX_FUNCTION;

sOSBase_t    OSScheduleGetState( void ) TINIUX_FUNCTION;
void         OSNeedSchedule( void ) TINIUX_FUNCTION;

uOSBool_t    OSIncrementTickCount( void ) TINIUX_FUNCTION;
uOSTick_t    OSGetTickCount( void ) TINIUX_FUNCTION;
uOSTick_t    OSGetTickCountFromISR( void ) TINIUX_FUNCTION;

void         OSSetTimeOutState( tOSTimeOut_t * const ptTimeOut ) TINIUX_FUNCTION;
uOSBool_t    OSGetTimeOutState( tOSTimeOut_t * const ptTimeOut, uOSTick_t * const puxTicksToWait ) TINIUX_FUNCTION;


#if ( OS_LOWPOWER_ON!=0 )
void         OSFixTickCount( const uOSTick_t uxTicksToFix ) TINIUX_FUNCTION;
uOSBool_t    OSEnableLowPowerIdle( void ) TINIUX_FUNCTION;
uOSTick_t    OSGetBlockTickCount( void ) TINIUX_FUNCTION;
#endif //OS_LOWPOWER_ON
void         OSUpdateUnblockTime( void ) TINIUX_FUNCTION;

void         OSSetReadyPriority(uOSBase_t uxPriority) TINIUX_FUNCTION;
void         OSResetReadyPriority(uOSBase_t uxPriority) TINIUX_FUNCTION;
uOSBase_t    OSGetTopReadyPriority( void ) TINIUX_FUNCTION;

#ifdef __cplusplus
}
#endif

#endif //__OS_SCHEDULE_H_
