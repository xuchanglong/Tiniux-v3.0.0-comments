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

#ifndef __OS_TYPE_H_
#define __OS_TYPE_H_

#include "FitType.h"
#include "OSPreset.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TINIUX_DATA
#define TINIUX_FUNCTION

typedef void (*OSTaskFunction_t)( void * );
typedef void (*OSTimerFunction_t)(void * );
typedef void (*OSCallbackFunction_t)( void *, uOS32_t );

typedef struct tTIME_OUT
{
    sOSBase_t xOverflowCount;
    uOSTick_t uxTimeOnEntering;
} tOSTimeOut_t;

typedef enum {OS_FALSE = 0, OS_TRUE = !OS_FALSE}    uOSBool_t;
typedef enum {OS_SUCESS = 0, OS_ERROR = !OS_SUCESS} uOSStatus_t;

#define      OS_NULL                   ( ( void* ) 0 )
#define      OS_PASS                   ( OS_TRUE )
#define      OS_FAIL                   ( OS_FALSE )

#ifndef SETOS_CPU_CLOCK_HZ
  #define    OSCPU_CLOCK_HZ            ( ( unsigned long )36000000UL )
#else
  #define    OSCPU_CLOCK_HZ            ( ( unsigned long ) SETOS_CPU_CLOCK_HZ )
#endif

#ifndef SETOS_TICK_RATE_HZ
  #define    OSTICK_RATE_HZ            ( ( uOSTick_t )1000 )
#else
  #define    OSTICK_RATE_HZ            ( ( uOSTick_t ) SETOS_TICK_RATE_HZ )
#endif

#ifndef FITSTACK_GROWTH
  #define    OSSTACK_GROWTH            ( -1 )	//栈从高位向低位增长
#else
  #define    OSSTACK_GROWTH            ( FITSTACK_GROWTH )	//栈从低位向高位增长
#endif

#ifndef FITBYTE_ALIGNMENT
  #define    OSMEM_ALIGNMENT           ( 8U )
#else
  #define    OSMEM_ALIGNMENT           ( FITBYTE_ALIGNMENT )
#endif
#define      OSMEM_ALIGNMENT_MASK      ( OSMEM_ALIGNMENT-1 )

// Priority range of the TINIUX 0~63
#ifndef SETOS_MAX_PRIORITIES
  #define    OSTASK_MAX_PRIORITY       ( 8U )
#else
  #if (SETOS_MAX_PRIORITIES>64U)
    #define    OSTASK_MAX_PRIORITY     ( 64U )
  #else
    #define    OSTASK_MAX_PRIORITY     ( SETOS_MAX_PRIORITIES )
  #endif
#endif

#define      OSLOWEAST_PRIORITY        ( 0U )
#define      OSHIGHEAST_PRIORITY       ( OSTASK_MAX_PRIORITY )

// Use quick schedule mode or not
#ifndef SETOS_USE_QUICK_SCHEDULE
  #define    OSQUICK_SCHEDULE_ON       ( 0U )
#else
  #define    OSQUICK_SCHEDULE_ON       ( SETOS_USE_QUICK_SCHEDULE )
#endif
#if ( OSQUICK_SCHEDULE_ON!=0 )
    #if (OSHIGHEAST_PRIORITY>16U)
        #define OSQUICK_GET_PRIORITY        ( 3U )
    #else
        #if (OSHIGHEAST_PRIORITY>4U)
            #define OSQUICK_GET_PRIORITY    ( 2U )
        #else
            #define OSQUICK_GET_PRIORITY    ( 1U )
        #endif
    #endif
#else
    #define     OSQUICK_GET_PRIORITY        ( 0U )
#endif

// Use time slice mode or not
#ifndef SETOS_TIME_SLICE_ON
  #define    OSTIME_SLICE_ON           ( 1U )
#else
  #define    OSTIME_SLICE_ON           ( SETOS_TIME_SLICE_ON )
#endif

// The total heap size of the TINIUX
#ifndef SETOS_TOTAL_HEAP_SIZE
  #define    OSTOTAL_HEAP_SIZE         ( 512U )
#else
  #define    OSTOTAL_HEAP_SIZE         ( SETOS_TOTAL_HEAP_SIZE )
#endif

// Mini stack size of a task(Idle task or Monitor task)
#ifndef SETOS_MINIMAL_STACK_SIZE
  #define    OSMINIMAL_STACK_SIZE      ( 32U )
#else
  #define    OSMINIMAL_STACK_SIZE      ( SETOS_MINIMAL_STACK_SIZE )
#endif

// Length of name(eg. task name, semaphore name, MsgQ name, Mutex name)
#ifndef SETOS_MAX_NAME_LEN
  #define    OSNAME_MAX_LEN            ( 10U )
#else
  #define    OSNAME_MAX_LEN            ( SETOS_MAX_NAME_LEN )
#endif

// Enable delete task or not
#ifndef SETOS_ENABLE_MEMFREE
  #define    OS_MEMFREE_ON             ( 0U )
#else
  #define    OS_MEMFREE_ON             ( SETOS_ENABLE_MEMFREE )
#endif

// Use low-power mode or not
#ifndef SETOS_LOWPOWER_MODE
  #define    OS_LOWPOWER_ON            ( 0U )
#else
  #define    OS_LOWPOWER_ON            ( SETOS_LOWPOWER_MODE )
#endif

//define mini ticks could use in low-power mode 
#if ( OS_LOWPOWER_ON!=0 )
    #ifndef SETOS_LOWPOWER_MINI_TICKS
        #define        OS_LOWPOWER_MINI_TICKS    ( 2U )
    #else
        #if (SETOS_LOWPOWER_MINI_TICKS<2)
            #define    OS_LOWPOWER_MINI_TICKS    ( 2U )
        #else
            #define    OS_LOWPOWER_MINI_TICKS    ( SETOS_LOWPOWER_MINI_TICKS )
        #endif
    #endif
#endif

// The value used as pend forever
#ifndef SETOS_PEND_FOREVER_VALUE
  #define    OSPEND_FOREVER_VALUE      ( ( uOSTick_t ) 0xFFFFFFFFUL )
#else
  #define    OSPEND_FOREVER_VALUE      ( ( uOSTick_t ) SETOS_PEND_FOREVER_VALUE )
#endif

// Use semaphore or not
#ifndef SETOS_USE_SEMAPHORE
  #define    OS_SEMAPHORE_ON           ( 1U )
#else
  #define    OS_SEMAPHORE_ON           ( SETOS_USE_SEMAPHORE )
#endif

// Use message queue or not
#ifndef SETOS_USE_MSGQ
  #define    OS_MSGQ_ON                ( 1U )
#else
  #define    OS_MSGQ_ON                ( SETOS_USE_MSGQ )
#endif

// The max message num in a message queue
#ifndef SETOS_MSGQ_MAX_MSGNUM
  #define    OSMSGQ_MAX_MSGNUM         ( 5U )
#else
  #define    OSMSGQ_MAX_MSGNUM         ( SETOS_MSGQ_MAX_MSGNUM )
#endif


// Use mutex or not
#ifndef SETOS_USE_MUTEX
  #define    OS_MUTEX_ON               ( 1U )
#else
  #define    OS_MUTEX_ON               ( SETOS_USE_MUTEX )
#endif

// Use Timer or not
#ifndef SETOS_USE_TIMER
  #define    OS_TIMER_ON               ( 1U )
#else
  #define    OS_TIMER_ON               ( SETOS_USE_TIMER )
#endif

#if ( OS_MSGQ_ON!=0 )
// Used by timer
#ifndef SETOS_CALLBACK_TASK_PRIORITY
  #define    OSCALLBACK_TASK_PRIO      ( OSHIGHEAST_PRIORITY - 1 )
#else
  #define    OSCALLBACK_TASK_PRIO      ( SETOS_CALLBACK_TASK_PRIORITY )
#endif
#endif //( OS_MSGQ_ON!=0 )

#ifndef SETOS_TASK_SIGNAL_ON
  #define    OS_TASK_SIGNAL_ON         ( 1U )
#else
  #define    OS_TASK_SIGNAL_ON         ( SETOS_TASK_SIGNAL_ON )
#endif

// Milliseconds to OS Ticks
#define      OSM2T( x )                ( ( uOSTick_t )( ( x )*( OSTICK_RATE_HZ/1000.0 ) ) )
// Frequency to OS Ticks
#define      OSF2T( x )                ( ( uOSTick_t )( ( OSTICK_RATE_HZ/( x ) ) ) )

#ifdef __cplusplus
}
#endif

#endif /* __OS_TYPE_H_ */
