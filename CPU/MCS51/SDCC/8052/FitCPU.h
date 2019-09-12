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

#ifndef __FIT_CPU_H_
#define __FIT_CPU_H_

#include "FitType.h"

#ifdef __cplusplus
extern "C" {
#endif


/*-----------------------------------------------------------*/

/* Critical section management. */

#define FitIntLock()                __asm        \
                                    push    ACC  \
                                    push    IE   \
                                    __endasm;    \
                                    EA = 0;

#define FitIntUnlock()                __asm        \
                                    pop     ACC    \
                                    __endasm;      \
                                    ACC &= 0x80;   \
                                    IE |= ACC;     \
                                    __asm          \
                                    pop     ACC    \
                                    __endasm;

#define FitIntMask()                EA = 0;
#define FitIntUnmask()              EA = 1;

#define FitIntMaskFromISR()         EA = 0;
#define FitIntUnmaskFromISR( x )    EA = 1; ( void ) x;

#define FitIsInsideISR()            0;

/*-----------------------------------------------------------*/

/* Task utilities. */
void FitSchedule( void ) __naked;
#define FitScheduleFromISR( b )     if( b ) FitSchedule()
/*-----------------------------------------------------------*/

#define FitNOP()                __asm    \
                                    nop  \
                                __endasm;

/*-----------------------------------------------------------*/

uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack, OSTaskFunction_t TaskFunction, void *pvParameters );
sOSBase_t FitStartScheduler( void );
void FitOSTickISR( void ) __interrupt 5 __naked;

#ifdef __cplusplus
}
#endif

#endif //__FIT_CPU_H_
