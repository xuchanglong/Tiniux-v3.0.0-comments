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

/* IAR Compiler includes. */
#include <intrinsics.h>

#include "TINIUX.h" 

#ifdef __cplusplus
extern "C" {
#endif
/* Constants required to manipulate the NVIC. */
#define FitNVIC_SYSTICK_CTRL            ( ( volatile uOS32_t *) 0xe000e010 )
#define FitNVIC_SYSTICK_LOAD            ( ( volatile uOS32_t *) 0xe000e014 )
#define FitNVIC_SYSTICK_CURRENT_VALUE   ( ( volatile uOS32_t *) 0xe000e018 )
#define FitNVIC_SYSPRI2                 ( ( volatile uOS32_t *) 0xe000ed20 )
#define FitNVIC_SYSTICK_CLK             0x00000004
#define FitNVIC_SYSTICK_INT             0x00000002
#define FitNVIC_SYSTICK_ENABLE          0x00000001
#define FitNVIC_PENDSV_PRI              ( ( ( uOS32_t ) OSMIN_HWINT_PRI ) << 16UL )
#define FitNVIC_SYSTICK_PRI             ( ( ( uOS32_t ) OSMIN_HWINT_PRI ) << 24UL )

/* Constants required to set up the initial stack. */
#define FitINITIAL_XPSR                 ( 0x01000000 )

#ifndef OSSYSTICK_CLOCK_HZ
    #define OSSYSTICK_CLOCK_HZ          OSCPU_CLOCK_HZ
    /* Ensure the SysTick is clocked at the same frequency as the core. */
    #define FitNVIC_SYSTICK_CLK_BIT     ( 1UL << 2UL )
#else
    /* The way the SysTick is clocked is not modified in case it is not the same
    as the core. */
    #define FitNVIC_SYSTICK_CLK_BIT     ( 0 )
#endif

/* Each task maintains its own interrupt status in the lock nesting
variable. */
static uOSBase_t guxIntLocked = 0xaaaaaaaa;

static void FitSetupTimerInterrupt( void );
extern void FitStartFirstTask( void );
static void FitTaskExitError( void );

/*
 * See header file for description.
 */
uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack, OSTaskFunction_t TaskFunction, void *pvParameters )
{
    /* Simulate the stack frame as it would be created by a context switch
    interrupt. */
    pxTopOfStack--; /* Offset added to account for the way the MCU uses the stack on entry/exit of interrupts. */
    *pxTopOfStack = FitINITIAL_XPSR;                /* xPSR */
    pxTopOfStack--;
    *pxTopOfStack = ( uOSStack_t ) TaskFunction;    /* PC */
    pxTopOfStack--;
    *pxTopOfStack = ( uOSStack_t ) FitTaskExitError;/* LR */
    pxTopOfStack -= 5;                              /* R12, R3, R2 and R1. */
    *pxTopOfStack = ( uOSStack_t ) pvParameters;    /* R0 */
    pxTopOfStack -= 8;                              /* R11..R4. */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

static void FitTaskExitError( void )
{
    /* A function that implements a task must not exit or attempt to return to
    its caller as there is nothing to return to.  If a task wants to exit it
    should instead call OSTaskDelete( OS_NULL ).*/

    FitIntMask();
    for( ;; );
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
sOSBase_t FitStartScheduler( void )
{
    /* Make PendSV and SysTick the lowest priority interrupts. */
    *(FitNVIC_SYSPRI2) |= FitNVIC_PENDSV_PRI;
    *(FitNVIC_SYSPRI2) |= FitNVIC_SYSTICK_PRI;

    /* Start the timer that generates the tick ISR.  Interrupts are disabled
    here already. */
    FitSetupTimerInterrupt();

    /* Initialise the lock nesting count ready for the first task. */
    guxIntLocked = 0;

    /* Start the first task. */
    FitStartFirstTask();

    /* Should not get here! */
    return 0;
}
/*-----------------------------------------------------------*/

void FitEndScheduler( void )
{
}
/*-----------------------------------------------------------*/

void FitSchedule( void )
{
    /* Set a PendSV to request a context switch. */
    *(FitNVIC_INT_CTRL) = FitNVIC_PENDSVSET;

    /* Barriers are normally not required but do ensure the code is completely
    within the specified behaviour for the architecture. */
    __DSB();
    __ISB();
}
/*-----------------------------------------------------------*/

void FitIntLock( void )
{
    FitIntMask();
    guxIntLocked++;
    __DSB();
    __ISB();
}
/*-----------------------------------------------------------*/

void FitIntUnlock( void )
{
    guxIntLocked--;
    if( guxIntLocked == 0 )
    {
        FitIntUnmask( 0 );
    }
}
/*-----------------------------------------------------------*/

void FitOSTickISR( void )
{
    uOS32_t ulPreviousMask;

    ulPreviousMask = FitIntMaskFromISR();
    {
        /* Increment the RTOS tick. */
        if( OSIncrementTickCount() != OS_FALSE )
        {
            /* Pend a context switch. */
            *(FitNVIC_INT_CTRL) = FitNVIC_PENDSVSET;
        }
    }
    FitIntUnmaskFromISR( ulPreviousMask );
}
/*-----------------------------------------------------------*/

/*
 * Setup the systick timer to generate the tick interrupts at the required
 * frequency.
 */
static void FitSetupTimerInterrupt( void )
{
    /* Stop and reset the SysTick. */
    *(FitNVIC_SYSTICK_CTRL) = 0UL;
    *(FitNVIC_SYSTICK_CURRENT_VALUE) = 0UL;
    
    /* Configure SysTick to interrupt at the requested rate. */
    *(FitNVIC_SYSTICK_LOAD) = ( OSSYSTICK_CLOCK_HZ / OSTICK_RATE_HZ ) - 1UL;
    *(FitNVIC_SYSTICK_CTRL) = FitNVIC_SYSTICK_CLK | FitNVIC_SYSTICK_INT | FitNVIC_SYSTICK_ENABLE;
}

#ifdef __cplusplus
}
#endif
