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

#include <OSPreset.h>

    RSEG    CODE:CODE(2)
    thumb

    EXTERN FitYieldFromISR
    EXTERN gptCurrentTCB
    EXTERN OSTaskSwitchContext

    PUBLIC vSetMSP
    PUBLIC FitPendSVHandler
    PUBLIC FitSVCHandler
    PUBLIC FitStartFirstTask
    PUBLIC FitIntMask
    PUBLIC FitIntUnmask
    PUBLIC FitGetIPSR
    
/*-----------------------------------------------------------*/

vSetMSP:
    msr msp, r0
    bx lr

/*-----------------------------------------------------------*/

FitPendSVHandler:
    mrs r0, psp

    ldr    r3, =gptCurrentTCB   /* Get the location of the current TCB. */
    ldr    r2, [r3]

    subs r0, r0, #32            /* Make space for the remaining low registers. */
    str r0, [r2]                /* Save the new top of stack. */
    stmia r0!, {r4-r7}          /* Store the low registers that are not saved automatically. */
    mov r4, r8                  /* Store the high registers. */
    mov r5, r9
    mov r6, r10
    mov r7, r11
    stmia r0!, {r4-r7}

    push {r3, r14}
    cpsid i
    bl OSTaskSwitchContext
    cpsie i
    pop {r2, r3}                /* lr goes in r3. r2 now holds tcb pointer. */

    ldr r1, [r2]
    ldr r0, [r1]                /* The first item in gptCurrentTCB is the task top of stack. */
    adds r0, r0, #16            /* Move to the high registers. */
    ldmia r0!, {r4-r7}          /* Pop the high registers. */
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7

    msr psp, r0                 /* Remember the new top of stack for the task. */

    subs r0, r0, #32            /* Go back for the low registers that are not automatically restored. */
    ldmia r0!, {r4-r7}          /* Pop low registers.  */

    bx r3

/*-----------------------------------------------------------*/

FitSVCHandler;
    /* This function is no longer used, but retained for backward
    compatibility. */
    bx lr

/*-----------------------------------------------------------*/

FitStartFirstTask:
    /* The MSP stack is not reset as, unlike on M3/4 parts, there is no vector
    table offset register that can be used to locate the initial stack value.
    Not all M0 parts have the application vector table at address 0. */

    ldr    r3, =gptCurrentTCB    /* Obtain location of gptCurrentTCB. */
    ldr r1, [r3]
    ldr r0, [r1]                 /* The first item in gptCurrentTCB is the task top of stack. */
    adds r0, #32                 /* Discard everything up to r0. */
    msr psp, r0                  /* This is now the new top of stack to use in the task. */
    movs r0, #2                  /* Switch to the psp stack. */
    msr CONTROL, r0
    isb
    pop {r0-r5}                  /* Pop the registers that are saved automatically. */
    mov lr, r5                   /* lr is now in r5. */
    cpsie i                      /* The first task has its context and interrupts can be enabled. */
    pop {pc}                     /* Finally, pop the PC to jump to the user defined task code. */

/*-----------------------------------------------------------*/

FitIntMask:
    mrs r0, PRIMASK
    cpsid i
    bx lr

/*-----------------------------------------------------------*/

FitIntUnmask:
    msr PRIMASK, r0
    bx lr

FitGetIPSR:
    /* Get IPSR value. */
    mrs r0, IPSR
    bx    lr

    END
