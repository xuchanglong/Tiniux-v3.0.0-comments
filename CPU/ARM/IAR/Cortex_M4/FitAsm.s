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

    EXTERN gptCurrentTCB
    EXTERN OSTaskSwitchContext

    PUBLIC FitPendSVHandler
    PUBLIC FitSVCHandler
    PUBLIC FitStartFirstTask
    PUBLIC FitEnableVFP
    PUBLIC FitGetIPSR

/*-----------------------------------------------------------*/

FitPendSVHandler:
    mrs r0, psp
    isb
    /* Get the location of the current TCB. */
    ldr    r3, =gptCurrentTCB
    ldr    r2, [r3]

    /* Is the task using the FPU context?  If so, push high vfp registers. */
    tst r14, #0x10
    it eq
    vstmdbeq r0!, {s16-s31}

    /* Save the core registers. */
    stmdb r0!, {r4-r11, r14}

    /* Save the new top of stack into the first member of the TCB. */
    str r0, [r2]

    stmdb sp!, {r3}
    mov r0, #OSMAX_HWINT_PRI
    msr basepri, r0
    dsb
    isb
    bl OSTaskSwitchContext
    mov r0, #0
    msr basepri, r0
    ldmia sp!, {r3}

    /* The first item in gptCurrentTCB is the task top of stack. */
    ldr r1, [r3]
    ldr r0, [r1]

    /* Pop the core registers. */
    ldmia r0!, {r4-r11, r14}

    /* Is the task using the FPU context?  If so, pop the high vfp registers
    too. */
    tst r14, #0x10
    it eq
    vldmiaeq r0!, {s16-s31}

    msr psp, r0
    isb
    #ifdef WORKAROUND_PMU_CM001 /* XMC4000 specific errata */
        #if WORKAROUND_PMU_CM001 == 1
            push { r14 }
            pop { pc }
        #endif
    #endif

    bx r14


/*-----------------------------------------------------------*/

FitSVCHandler:
    /* Get the location of the current TCB. */
    ldr    r3, =gptCurrentTCB
    ldr r1, [r3]
    ldr r0, [r1]
    /* Pop the core registers. */
    ldmia r0!, {r4-r11, r14}
    msr psp, r0
    isb
    mov r0, #0
    msr    basepri, r0
    bx r14

/*-----------------------------------------------------------*/

FitStartFirstTask:
    /* Use the NVIC offset register to locate the stack. */
    ldr r0, =0xE000ED08
    ldr r0, [r0]
    ldr r0, [r0]
    /* Set the msp back to the start of the stack. */
    msr msp, r0
    /* Call SVC to start the first task. */
    cpsie i
    cpsie f
    dsb
    isb
    svc 0

/*-----------------------------------------------------------*/

FitEnableVFP:
    /* The FPU enable bits are in the CPACR. */
    ldr.w r0, =0xE000ED88
    ldr    r1, [r0]

    /* Enable CP10 and CP11 coprocessors, then save back. */
    orr    r1, r1, #( 0xf << 20 )
    str r1, [r0]
    bx    r14

FitGetIPSR:
    /* Get IPSR value. */
    mrs r0, IPSR
    bx    r14
    

    END

