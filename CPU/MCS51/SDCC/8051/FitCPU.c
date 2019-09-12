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

/* Standard includes. */
#include <string.h>
/* Compiler includes. */
#include <stdint.h>
#include "TINIUX.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants required to setup timer 0 to produce the RTOS tick. */
#define FIT_CLOCK_DIVISOR               ( ( uint32_t ) 12 )
#define FIT_MAX_TIMER_VALUE             ( ( uint32_t ) 0xFFFF )
#define FIT_ENABLE_TIMER                ( ( uint8_t ) 0x01 )
#define FIT_TIMER_0_INTERRUPT_ENABLE    ( ( uint8_t ) 0x01 )

/* The value used in the IE register when a task first starts. */
#define FIT_GLOBAL_INTERRUPT_BIT        ( ( uOSStack_t ) 0x80 )

/* The value used in the PSW register when a task first starts. */
#define FIT_INITIAL_PSW                 ( ( uOSStack_t ) 0x00 )

/* Macro to clear the timer 0 interrupt flag. */
#define FIT_CLEAR_INTERRUPT_FLAG()      TF0 = 0;

/* Used during a context switch to store the size of the stack being copied
to or from XRAM. */
__idata static uOS8_t                   ucStackBytes = 0U;

/* Used during a context switch to point to the next byte in XRAM from/to which
a RAM byte is to be copied. */
__xdata static uOSStack_t * __idata     pxXRAMStack = OS_NULL;

/* Used during a context switch to point to the next byte in RAM from/to which
an XRAM byte is to be copied. */
__idata static uOSStack_t * __idata     pxRAMStack = OS_NULL;

/* We require the address of the gptCurrentTCB variable. */
extern volatile tOSTCB_t * volatile     gptCurrentTCB;

__idata static uOS8_t                   gucTLReload = 0;
__idata static uOS8_t                   gucTHReload = 0;

/* Define the stack size which used by tiniux runing. */
#ifndef OSRUNING_STACK_SIZE
  #define    OSRUNING_STACK_SIZE        ( OSMINIMAL_STACK_SIZE + OSMINIMAL_STACK_SIZE>>2 )
#endif

/* Define the room used by tiniux in RAM stack. */
__idata static uOSStack_t               gOSRuningRAMStack[OSRUNING_STACK_SIZE];
__idata static const uOSStack_t         guxStackStartAddr = ( __idata uOSStack_t )  gOSRuningRAMStack ;

/*
 * Setup the hardware to generate an interrupt off timer 2 at the required
 * frequency.
 */
static void FitSetupTimerInterrupt( void );

/*-----------------------------------------------------------*/
/*
 * Macro that copies the current stack from internal RAM to XRAM.  This is
 * required as the 8051 only contains enough internal RAM for a single stack,
 * but we have a stack for every task.
 */
#define FitCopyStackToXRam()                                                                    \
{                                                                                               \
    /* gptCurrentTCB points to a TCB which itself points to the location into                   \
    which the first    stack byte should be copied.  Set pxXRAMStack to point                   \
    to the location into which the first stack byte is to be copied. */                         \
    pxXRAMStack = ( __xdata uOSStack_t * ) *( ( __xdata uOSStack_t ** ) gptCurrentTCB );        \
                                                                                                \
    /* Set pxRAMStack to point to the first byte to be coped from the stack. */                 \
    pxRAMStack = ( __idata uOSStack_t * __idata ) guxStackStartAddr;                            \
                                                                                                \
    /* Calculate the size of the stack we are about to copy from the current                    \
    stack pointer value. */                                                                     \
    ucStackBytes = SP - ( guxStackStartAddr - 1 );                                              \
                                                                                                \
    /* Before starting to copy the stack, store the calculated stack size so                    \
    the stack can be restored when the task is resumed. */                                      \
    *pxXRAMStack = ucStackBytes;                                                                \
                                                                                                \
    /* Copy each stack byte in turn.  pxXRAMStack is incremented first as we                    \
    have already stored the stack size into XRAM. */                                            \
    while( ucStackBytes )                                                                       \
    {                                                                                           \
        pxXRAMStack++;                                                                          \
        *pxXRAMStack = *pxRAMStack;                                                             \
        pxRAMStack++;                                                                           \
        ucStackBytes--;                                                                         \
    }                                                                                           \
}
/*-----------------------------------------------------------*/

/*
 * Macro that copies the stack of the task being resumed from XRAM into
 * internal RAM.
 */
#define FitCopyXRamToStack()                                                                    \
{                                                                                               \
    /* Setup the pointers as per FitCopyStackToXRam(), but this time to                         \
    copy the __idata back out of XRAM and into the stack. */                                    \
    pxXRAMStack = ( __xdata uOSStack_t * ) *( ( __xdata uOSStack_t ** ) gptCurrentTCB );        \
    pxRAMStack = ( __idata uOSStack_t * __idata ) ( guxStackStartAddr - 1 );                    \
                                                                                                \
    /* The first value stored in XRAM was the size of the stack - i.e. the                      \
    number of bytes we need to copy back. */                                                    \
    ucStackBytes = pxXRAMStack[ 0 ];                                                            \
                                                                                                \
    /* Copy the required number of bytes back into the stack. */                                \
    do                                                                                          \
    {                                                                                           \
        pxXRAMStack++;                                                                          \
        pxRAMStack++;                                                                           \
        *pxRAMStack = *pxXRAMStack;                                                             \
        ucStackBytes--;                                                                         \
    } while( ucStackBytes );                                                                    \
                                                                                                \
    /* Restore the stack pointer ready to use the restored stack. */                            \
    SP = ( uint8_t ) pxRAMStack;                                                                \
}
/*-----------------------------------------------------------*/

/*
 * Macro to push the current execution context onto the stack, before the stack
 * is moved to XRAM.
 */
#define FitSaveTaskContex()                                                                     \
{                                                                                               \
    __asm                                                                                       \
        /* Push ACC first, as when restoring the context it must be restored                    \
        last (it is used to set the IE register). */                                            \
        push    ACC                                                                             \
        /* Store the IE register then disable interrupts. */                                    \
        push    IE                                                                              \
        clr        _EA                                                                          \
        push    DPL                                                                             \
        push    DPH                                                                             \
        push    b                                                                               \
        push    ar2                                                                             \
        push    ar3                                                                             \
        push    ar4                                                                             \
        push    ar5                                                                             \
        push    ar6                                                                             \
        push    ar7                                                                             \
        push    ar0                                                                             \
        push    ar1                                                                             \
        push    PSW                                                                             \
    __endasm;                                                                                   \
        PSW = 0;                                                                                \
    __asm                                                                                       \
        push    _bp                                                                             \
    __endasm;                                                                                   \
}
/*-----------------------------------------------------------*/

/*
 * Macro that restores the execution context from the stack.  The execution
 * context was saved into the stack before the stack was copied into XRAM.
 */
#define FitRestoreTaskContext()                                                                 \
{                                                                                               \
    __asm                                                                                       \
        pop        _bp                                                                          \
        pop        PSW                                                                          \
        pop        ar1                                                                          \
        pop        ar0                                                                          \
        pop        ar7                                                                          \
        pop        ar6                                                                          \
        pop        ar5                                                                          \
        pop        ar4                                                                          \
        pop        ar3                                                                          \
        pop        ar2                                                                          \
        pop        b                                                                            \
        pop        DPH                                                                          \
        pop        DPL                                                                          \
        /* The next byte of the stack is the IE register.  Only the global                      \
        enable bit forms part of the task context.  Pop off the IE then set                     \
        the global enable bit to match that of the stored IE register. */                       \
        pop        ACC                                                                          \
        JB        ACC.7,0098$                                                                   \
        CLR        IE.7                                                                         \
        LJMP    0099$                                                                           \
    0098$:                                                                                      \
        SETB    IE.7                                                                            \
    0099$:                                                                                      \
        /* Finally pop off the ACC, which was the first register saved. */                      \
        pop        ACC                                                                          \
        reti                                                                                    \
    __endasm;                                                                                   \
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack, OSTaskFunction_t TaskFunction, void *pvParameters )
{
    uint32_t ulAddress;
    uOSStack_t *pxStartOfStack;

    /* Leave space to write the size of the stack as the first byte. */
    pxStartOfStack = pxTopOfStack;
    pxTopOfStack++;

    /* Simulate how the stack would look after a call to the scheduler tick
    ISR.

    The return address that would have been pushed by the MCU. */
    ulAddress = ( uint32_t ) TaskFunction;
    *pxTopOfStack = ( uOSStack_t ) ulAddress;
    ulAddress >>= 8;
    pxTopOfStack++;
    *pxTopOfStack = ( uOSStack_t ) ( ulAddress );
    pxTopOfStack++;

    /* Next all the registers will have been pushed by FitSaveTaskContex(). */
    *pxTopOfStack = 0xaa;    /* acc */
    pxTopOfStack++;

    /* We want tasks to start with interrupts enabled. */
    *pxTopOfStack = FIT_GLOBAL_INTERRUPT_BIT;
    pxTopOfStack++;

    /* The function parameters will be passed in the DPTR and B register as
    a three byte generic pointer is used. */
    ulAddress = ( uint32_t ) pvParameters;
    *pxTopOfStack = ( uOSStack_t ) ulAddress;    /* DPL */
    ulAddress >>= 8;
    *pxTopOfStack++;
    *pxTopOfStack = ( uOSStack_t ) ulAddress;    /* DPH */
    ulAddress >>= 8;
    pxTopOfStack++;
    *pxTopOfStack = ( uOSStack_t ) ulAddress;    /* b */
    pxTopOfStack++;

    /* The remaining registers are straight forward. */
    *pxTopOfStack = 0x02;    /* R2 */
    pxTopOfStack++;
    *pxTopOfStack = 0x03;    /* R3 */
    pxTopOfStack++;
    *pxTopOfStack = 0x04;    /* R4 */
    pxTopOfStack++;
    *pxTopOfStack = 0x05;    /* R5 */
    pxTopOfStack++;
    *pxTopOfStack = 0x06;    /* R6 */
    pxTopOfStack++;
    *pxTopOfStack = 0x07;    /* R7 */
    pxTopOfStack++;
    *pxTopOfStack = 0x00;    /* R0 */
    pxTopOfStack++;
    *pxTopOfStack = 0x01;    /* R1 */
    pxTopOfStack++;
    *pxTopOfStack = 0x00;    /* PSW */
    pxTopOfStack++;
    *pxTopOfStack = 0xbb;    /* BP */

    /* Dont increment the stack size here as we don't want to include
    the stack size byte as part of the stack size count.

    Finally we place the stack size at the beginning. */
    *pxStartOfStack = ( uOSStack_t ) ( pxTopOfStack - pxStartOfStack );

    /* Unlike most ports, we return the start of the stack as this is where the
    size of the stack is stored. */
    return pxStartOfStack;
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
sOSBase_t FitStartScheduler( void )
{
    /* Initialize the stack tiniux runing. */
    memset(gOSRuningRAMStack, 0, sizeof(uOSStack_t)*OSRUNING_STACK_SIZE);

    /* Setup timer 0 to generate the RTOS tick. */
    FitSetupTimerInterrupt();

    /* Copy the stack for the first task to execute from XRAM into the stack,
    restore the task context from the new stack, then start running the task. */
    FitCopyXRamToStack();
    FitRestoreTaskContext();

    /* Should never get here! */
    return OS_TRUE;
}
/*-----------------------------------------------------------*/

void FitEndScheduler( void )
{
    /* Not implemented for this port. */
}
/*-----------------------------------------------------------*/

/*
 * Manual context switch.  The first thing we do is save the registers so we
 * can use a naked attribute.
 */
void FitSchedule( void ) __naked
{
    /* Save the execution context onto the stack, then copy the entire stack
    to XRAM.  This is necessary as the internal RAM is only large enough to
    hold one stack, and we want one per task.

    PERFORMANCE COULD BE IMPROVED BY ONLY COPYING TO XRAM IF A TASK SWITCH
    IS REQUIRED. */
    FitSaveTaskContex();
    FitCopyStackToXRam();

    /* Call the standard scheduler context switch function. */
    OSTaskSwitchContext();

    /* Copy the stack of the task about to execute from XRAM into RAM and
    restore it's context ready to run on exiting. */
    FitCopyXRamToStack();
    FitRestoreTaskContext();
}
/*-----------------------------------------------------------*/

void FitOSTickISR( void ) __interrupt 1 __naked
{
    /* Preemptive context switch function triggered by the timer 2 ISR.
    This does the same as FitSchedule() (see above) with the addition
    of incrementing the RTOS tick count. */

    FitSaveTaskContex();
    FitCopyStackToXRam();
    
    /* Reload timer value. */
    TL0     = gucTLReload;
    TH0     = gucTHReload;
    
    if( OSIncrementTickCount() != OS_FALSE )
    {
        OSTaskSwitchContext();
    }

//    FIT_CLEAR_INTERRUPT_FLAG();
    FitCopyXRamToStack();
    FitRestoreTaskContext();
}

/*-----------------------------------------------------------*/

static void FitSetupTimerInterrupt( void )
{
    /* Constants calculated to give the required timer capture values. */
    const uint32_t ulTimerClockHZ        = OSCPU_CLOCK_HZ / FIT_CLOCK_DIVISOR;
    const uint32_t ulTimerCountPerTick   = ulTimerClockHZ / OSTICK_RATE_HZ;
    const uint32_t ulReloadValue         = FIT_MAX_TIMER_VALUE - ulTimerCountPerTick;
    const uint8_t ucReloadValueL         = ( uint8_t ) ( ulReloadValue & ( uint32_t ) 0xFF );
    const uint8_t ucReloadValueH         = ( uint8_t ) ( ulReloadValue >> ( uint32_t ) 8 );

    /* Setup timer0 mode. */
    TMOD    &= 0xF0;
    TMOD    |= 0x01;

    /* Setup the overflow reload value. */
    gucTLReload = ucReloadValueL;
    gucTHReload = ucReloadValueH;

    /* The initial load is performed manually. */
    TL0     = ucReloadValueL;     //
    TH0     = ucReloadValueH;

    /* Enable the timer 0 interrupts. */
    ET0     = FIT_TIMER_0_INTERRUPT_ENABLE;

    /* Interrupts are disabled when this is called so the timer can be started
    here. */
    TR0     = FIT_ENABLE_TIMER;
}


#ifdef __cplusplus
}
#endif
