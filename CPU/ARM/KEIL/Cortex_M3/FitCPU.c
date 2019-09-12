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

/* Compiler includes. */
#include <stdint.h>

#include "TINIUX.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants required to manipulate the core.  Registers first... */
#define FitNVIC_SYSTICK_CTRL_REG            ( * ( ( volatile uOS32_t * ) 0xe000e010 ) )
#define FitNVIC_SYSTICK_LOAD_REG            ( * ( ( volatile uOS32_t * ) 0xe000e014 ) )
#define FitNVIC_SYSTICK_CURRENT_VALUE_REG   ( * ( ( volatile uOS32_t * ) 0xe000e018 ) )
#define FitNVIC_SYSPRI2_REG                 ( * ( ( volatile uOS32_t * ) 0xe000ed20 ) )
/* ...then bits in the registers. */
#define FitNVIC_SYSTICK_CLK_BIT             ( 1UL << 2UL )
#define FitNVIC_SYSTICK_INT_BIT             ( 1UL << 1UL )
#define FitNVIC_SYSTICK_ENABLE_BIT          ( 1UL << 0UL )
#define FitNVIC_SYSTICK_COUNT_FLAG_BIT      ( 1UL << 16UL )
#define FitNVIC_PENDSVCLEAR_BIT             ( 1UL << 27UL )
#define FitNVIC_PEND_SYSTICK_CLEAR_BIT      ( 1UL << 25UL )

/* Masks off all bits but the VECTACTIVE bits in the ICOS register. */
#define FitVECTACTIVE_MASK                  ( 0x1FUL )

#define FitNVIC_PENDSV_PRI                  ( ( ( uOS32_t ) OSMIN_HWINT_PRI ) << 16UL )
#define FitNVIC_SYSTICK_PRI                 ( ( ( uOS32_t ) OSMIN_HWINT_PRI ) << 24UL )

/* Constants required to set up the initial stack. */
#define FitINITIAL_XPSR                     ( 0x01000000 )
#define FitINITIAL_EXEC_RETURN              ( 0xfffffffd )

/* Scheduler utilities. */
#define FitNVIC_INT_CTRL_REG                ( * ( ( volatile uOS32_t * ) 0xe000ed04 ) )
#define FitNVIC_PENDSVSET_BIT               ( 1UL << 28UL )

/* The systick is a 24-bit counter. */
#define FitMAX_24_BIT_NUMBER                ( 0xffffffUL )

/* Constants required to handle lock sections. */
#define FitNO_CRITICAL_NESTING              ( ( unsigned long ) 0 )
static volatile uOSBase_t guxIntLocked = 9999UL;

/* Constants used with memory barrier intrinsics. */
#define FitSY_FULL_READ_WRITE               ( 15 )

#ifndef OSSYSTICK_CLOCK_HZ
    #define OSSYSTICK_CLOCK_HZ              OSCPU_CLOCK_HZ
    /* Ensure the SysTick is clocked at the same frequency as the core. */
    #define FitNVIC_SYSTICK_CLK_BIT         ( 1UL << 2UL )
#else
    /* The way the SysTick is clocked is not modified in case it is not the same
    as the core. */
    #define FitNVIC_SYSTICK_CLK_BIT         ( 0 )
#endif

/* A fiddle factor to estimate the number of SysTick counts that would have
occurred while the SysTick counter is stopped during tickless idle
calculations. */
#define FitMISSED_COUNTS_FACTOR             ( 45UL )

#if( OS_LOWPOWER_ON!=0 )
    /* The number of SysTick increments that make up one tick period.*/
    static uint32_t gulTimerCountsPerTick = 0;
    /* The maximum number of tick periods that can be suppressed is limited by the
     * 24 bit resolution of the SysTick timer.*/
    static uint32_t guxMaxLowPowerTicks = 0;
    /* Compensate for the CPU cycles that pass while the SysTick is stopped.*/
    static uint32_t gulTimerCountsCompensation = 0;
#endif /* OS_LOWPOWER_ON */

/*
 * Setup the timer to generate the tick interrupts.
 */
static void FitSetupTimerInterrupt( void );

/*
 * Start first task is a separate function so it can be tested in isolation.
 */
static void FitStartFirstTask( void );

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void FitTaskExitError( void );



uOSStack_t *FitInitializeStack( uOSStack_t *pxTopOfStack,
        OSTaskFunction_t TaskFunction, void *pvParameters )
{
    /* Simulate the stack frame as it would be created by a context switch
    interrupt. */
    pxTopOfStack--; /* Offset added to account for the way the MCU uses the stack on entry/exit of interrupts. */
    *pxTopOfStack = FitINITIAL_XPSR;    /* xPSR */
    pxTopOfStack--;
    *pxTopOfStack = ( uOSStack_t ) TaskFunction;    /* PC */
    pxTopOfStack--;
    *pxTopOfStack = ( uOSStack_t ) FitTaskExitError;    /* LR */

    pxTopOfStack -= 5;    /* R12, R3, R2 and R1. */
    *pxTopOfStack = ( uOSStack_t ) pvParameters;    /* R0 */
    pxTopOfStack -= 8;    /* R11, R10, R9, R8, R7, R6, R5 and R4. */

    return pxTopOfStack;    
}

static void FitTaskExitError( void )
{
    /* A function that implements a task must not exit or attempt to return to
    its caller as there is nothing to return to.  If a task wants to exit it
    should instead call OSTaskDelete( OS_NULL ). */
    
    FitIntMask();
    for( ;; );
}

__asm void FitSVCHandler( void )
{
    PRESERVE8

    ldr    r3, =gptCurrentTCB    /* Restore the context. */
    ldr r1, [r3]                 /* Use gptCurrentTCB to get the gptCurrentTCB address. */
    ldr r0, [r1]                 /* The first item in gptCurrentTCB is the task top of stack. */
    ldmia r0!, {r4-r11}          /* Pop the registers that are not automatically saved on exception entry and the lock nesting count. */
    msr psp, r0                  /* Restore the task stack pointer. */
    isb
    mov r0, #0
    msr    basepri, r0
    orr r14, #0xd
    bx r14    
}

__asm void FitStartFirstTask( void )
{    
    PRESERVE8

    /* Use the NVIC offset register to locate the stack. */
    ldr r0, =0xE000ED08
    ldr r0, [r0]
    ldr r0, [r0]
    /* Set the msp back to the start of the stack. */
    msr msp, r0
    /* Globally enable interrupts. */
    cpsie i
    cpsie f
    dsb
    isb
    /* Call SVC to start the first task. */
    svc 0
    nop
    nop    
}

uOSBase_t FitStartScheduler( void )
{
    /* Make PendSV and SysTick the lowest priority interrupts. */
    FitNVIC_SYSPRI2_REG |= FitNVIC_PENDSV_PRI;
    FitNVIC_SYSPRI2_REG |= FitNVIC_SYSTICK_PRI;

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

void FitEndScheduler( void )
{

}

/*
 * Called by OSSchedule() to manually force a context switch.
 *
 * When a context switch is performed from the task level the saved task
 * context is made to look as if it occurred from within the tick ISR.  This
 * way the same restore context function can be used when restoring the context
 * saved from the ISR or that saved from a call to FitSchedule.
 */
void FitSchedule( void )                                                            
{                                                                                
    /* Set a PendSV to request a context switch. */                                
    FitNVIC_INT_CTRL_REG = FitNVIC_PENDSVSET_BIT;                                

}


void FitIntLock( void )
{
    FitIntMask();
    guxIntLocked++;
}

void FitIntUnlock( void )
{
    guxIntLocked--;
    if( guxIntLocked == 0 )
    {
        FitIntUnmask( 0 );
    }    
}


__asm void FitPendSVHandler( void )
{
    extern guxIntLocked;
    extern gptCurrentTCB;
    extern OSTaskSwitchContext;

    PRESERVE8

    mrs r0, psp
    isb

    ldr    r3, =gptCurrentTCB      /* Get the location of the current TCB. */
    ldr    r2, [r3]

    stmdb r0!, {r4-r11}            /* Save the remaining registers. */
    str r0, [r2]                   /* Save the new top of stack into the first member of the TCB. */

    stmdb sp!, {r3, r14}
    mov r0, #OSMAX_HWINT_PRI
    msr basepri, r0
    dsb
    isb
    bl OSTaskSwitchContext
    mov r0, #0
    msr basepri, r0
    ldmia sp!, {r3, r14}

    ldr r1, [r3]
    ldr r0, [r1]                   /* The first item in pxCurrentTCB is the task top of stack. */
    ldmia r0!, {r4-r11}            /* Pop the registers and the lock nesting count. */
    msr psp, r0
    isb
    bx r14
    nop            
}


void FitOSTickISR()
{
    // Increment the RTOS tick count, then look for the highest priority
    // task that is ready to run. 
    ( void ) OSIntMaskFromISR();
    {
        /* Increment the RTOS tick. */
        if( OSIncrementTickCount() != OS_FALSE )
        {
            /* A context switch is required.  Context switching is performed in
            the PendSV interrupt.  Pend the PendSV interrupt. */
            FitNVIC_INT_CTRL_REG = FitNVIC_PENDSVSET_BIT;
        }
    }
    OSIntUnmaskFromISR( 0 );
}

#if ( OS_LOWPOWER_ON!=0 )
__weak void FitTicklessIdle( uOSTick_t uxLowPowerTicks )
{
    uint32_t ulReloadValue, ulCompleteLowPowerTicks, ulCompleteLowPowerTimeCounts;

    /* Make sure the SysTick reload value does not overflow the counter. */
    if( uxLowPowerTicks > guxMaxLowPowerTicks )
    {
        uxLowPowerTicks = guxMaxLowPowerTicks;
    }

    /* Stop the timer that is generating the tick interrupt. */
    FitNVIC_SYSTICK_CTRL_REG &= ~FitNVIC_SYSTICK_ENABLE_BIT;

    /* Calculate the reload value required to wait uxLowPowerTicks tick periods.  
    -1 is used because this code will execute part way through one of the tick periods. */
    ulReloadValue = FitNVIC_SYSTICK_CURRENT_VALUE_REG + ( gulTimerCountsPerTick * ( uxLowPowerTicks - 1UL ) );
    if( ulReloadValue > gulTimerCountsCompensation )
    {
        ulReloadValue -= gulTimerCountsCompensation;
    }

    /* Enter a critical section that will not effect interrupts bringing the MCU out of sleep mode. */
    __disable_irq();
    __dsb( FitSY_FULL_READ_WRITE );
    __isb( FitSY_FULL_READ_WRITE );

    /* Ensure it is still ok to enter the sleep mode. */
    if( OSEnableLowPowerIdle() == OS_FALSE )
    {
        /* A task has been moved out of the Blocked state since this macro was
        executed, or a context siwth is being held pending.  Do not enter a
        sleep state.  Restart the tick and exit the critical section. */

        /* Restart from whatever is left in the count register to complete this tick period. */
        FitNVIC_SYSTICK_LOAD_REG = FitNVIC_SYSTICK_CURRENT_VALUE_REG;

        /* Restart SysTick. */
        FitNVIC_SYSTICK_CTRL_REG |= FitNVIC_SYSTICK_ENABLE_BIT;

        /* Reset the reload register to the value required for normal tick periods. */
        FitNVIC_SYSTICK_LOAD_REG = gulTimerCountsPerTick - 1UL;

        /* Exit the critical section, Re-enable interrupts*/
        __enable_irq();
    }
    else
    {
        /* Configure an interrupt to bring the microcontroller out of its low
        power state at the time the kernel next needs to execute.  */
        /* The interrupt must be generated from a source that remains operational
        when the microcontroller is in a low power state. */
        
        /* Set the new reload value. */
        FitNVIC_SYSTICK_LOAD_REG = ulReloadValue;

        /* Clear the SysTick count flag and set the count value back to zero. */
        FitNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

        /* Restart SysTick. */
        FitNVIC_SYSTICK_CTRL_REG |= FitNVIC_SYSTICK_ENABLE_BIT;

        /* Enter the low power state, sleep until something happens. */
        {
            __dsb( FitSY_FULL_READ_WRITE );
            __wfi();
            __isb( FitSY_FULL_READ_WRITE );
        }

        /* Re-enable interrupts to allow the interrupt that brought the MCU
        out of sleep mode to execute immediately.*/
        __enable_irq();
        __dsb( FitSY_FULL_READ_WRITE );
        __isb( FitSY_FULL_READ_WRITE );

        /* Disable interrupts again because the clock is about to be stopped
        and interrupts that execute while the clock is stopped will increase
        any slippage between the time maintained by the RTOS and calendar time. */
        __disable_irq();
        __dsb( FitSY_FULL_READ_WRITE );
        __isb( FitSY_FULL_READ_WRITE );
        
        /* Disable the SysTick clock without reading the FitNVIC_SYSTICK_CTRL_REG register to ensure the
        FitNVIC_SYSTICK_COUNT_FLAG_BIT is not cleared if it is set. */
        FitNVIC_SYSTICK_CTRL_REG = ( FitNVIC_SYSTICK_CLK_BIT | FitNVIC_SYSTICK_INT_BIT );

        /* Determine how long the microcontroller was actually in a low power state for, which will be less than uxLowPowerTicks 
        if the microcontroller was brought out of low power mode by an interrupt.*/
        /* Note that the scheduler is suspended. Therefore no other tasks will execute until this function completes. */        
        if( ( FitNVIC_SYSTICK_CTRL_REG & FitNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
        {
            /*The SysTick clock has already counted to zero and been set back to the current reload value*/
            
            uint32_t ulCalculatedLoadValue;

            /*Reset the FitNVIC_SYSTICK_LOAD_REG with whatever remains of this tick period. */
            ulCalculatedLoadValue = ( gulTimerCountsPerTick - 1UL ) - ( ulReloadValue - FitNVIC_SYSTICK_CURRENT_VALUE_REG );

            /* Don't allow a tiny value, or values that have somehow underflowed. */
            if( ( ulCalculatedLoadValue < gulTimerCountsCompensation ) || ( ulCalculatedLoadValue > gulTimerCountsPerTick ) )
            {
                ulCalculatedLoadValue = ( gulTimerCountsPerTick - 1UL );
            }

            FitNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

            /* As the pending tick will be processed as soon as this function exits, 
            the tick value maintained by the tick is stepped forward by one less than the time spent waiting. */
            ulCompleteLowPowerTicks = uxLowPowerTicks - 1UL;
        }
        else
        {
            /* Something other than the tick interrupt ended the sleep.*/
            /* Work out how long the sleep lasted rounded to complete tick periods. */
            ulCompleteLowPowerTimeCounts = ( uxLowPowerTicks * gulTimerCountsPerTick ) - FitNVIC_SYSTICK_CURRENT_VALUE_REG;
            ulCompleteLowPowerTicks = ulCompleteLowPowerTimeCounts / gulTimerCountsPerTick;

            /* The reload value is set to whatever fraction of a single tick period remains. */
            FitNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteLowPowerTicks + 1UL ) * gulTimerCountsPerTick ) - ulCompleteLowPowerTimeCounts;
        }

        /* Correct the kernels tick count to account for the time the microcontroller spent in its low power state. */
        OSFixTickCount( ulCompleteLowPowerTicks );
        
        /* Restart the timer that is generating the tick interrupt. */
        FitNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
        FitNVIC_SYSTICK_CTRL_REG |= FitNVIC_SYSTICK_ENABLE_BIT;
        
        /* Reset the reload register to the value required for normal tick periods. */
        FitNVIC_SYSTICK_LOAD_REG = gulTimerCountsPerTick - 1UL;

        /* Exit the critical section, Re-enable interrupts*/
        __enable_irq();
    }
}
#endif //OS_LOWPOWER_ON

/*
 * Setup the SysTick timer to generate the tick interrupts at the required
 * frequency.
 */
static void FitSetupTimerInterrupt( void )
{
    #if ( OS_LOWPOWER_ON!=0 )
    gulTimerCountsPerTick = ( OSSYSTICK_CLOCK_HZ / OSTICK_RATE_HZ );
    guxMaxLowPowerTicks = FitMAX_24_BIT_NUMBER / gulTimerCountsPerTick;
    gulTimerCountsCompensation = FitMISSED_COUNTS_FACTOR / ( OSCPU_CLOCK_HZ / OSSYSTICK_CLOCK_HZ );
    #endif //OS_LOWPOWER_ON

    /* Stop and clear the SysTick. */
    FitNVIC_SYSTICK_CTRL_REG = 0UL;
    FitNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
        
    /* Configure SysTick to interrupt at the requested rate. */
    FitNVIC_SYSTICK_LOAD_REG = ( OSSYSTICK_CLOCK_HZ / OSTICK_RATE_HZ ) - 1UL;
    FitNVIC_SYSTICK_CTRL_REG = ( FitNVIC_SYSTICK_CLK_BIT | FitNVIC_SYSTICK_INT_BIT | FitNVIC_SYSTICK_ENABLE_BIT );
}

__asm uOS32_t FitIntMask( void )
{
    PRESERVE8

    mrs r0, basepri
    mov r1, #OSMAX_HWINT_PRI
    msr basepri, r1
    bx r14
}
/*-----------------------------------------------------------*/

__asm void FitIntUnmask( uOS32_t ulNewMask )
{
    PRESERVE8

    msr basepri, r0
    bx r14
}

__asm uOS32_t FitGetIPSR( void )
{
    PRESERVE8

    mrs r0, ipsr
    bx r14
}

#ifdef __cplusplus
}
#endif
