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

// !!!注：应用程序可以根据需要调整Tiniux系统API接口函数及相关功能模块的开关 !!!

#ifndef __OS_PRESET_H_
#define __OS_PRESET_H_

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 *----------------------------------------------------------*/

/* Ensure stdint is only used by the compiler, and not the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
 #include <stdint.h>
 extern uint32_t SystemCoreClock;
#endif

#define SETOS_CPU_CLOCK_HZ                      ( SystemCoreClock ) //定义CPU运行主频 (如72000000)
#define SETOS_TICK_RATE_HZ                      ( 1000 )            //定义TINIUX系统中ticks频率

#define SETOS_MINIMAL_STACK_SIZE                ( 48 )          //定义任务占用的最小Stack空间
#define SETOS_TOTAL_HEAP_SIZE                   ( 1024*2 )      //定义系统占用的Heap空间
#define SETOS_ENABLE_MEMFREE                    ( 0 )           //是否允许释放内存，允许后可以在系统运行时删除Task MsgQ Semaphone Mutex Timer等
#define SETOS_LOWPOWER_MODE                     ( 0 )           //是否开启低功耗模式
#define SETOS_MAX_NAME_LEN                      ( 4 )           //定义任务、信号量、消息队列等变量中的名称长度
#define SETOS_MAX_PRIORITIES                    ( 4 )           //定义任务最大优先级
#define SETOS_TASK_SIGNAL_ON                    ( 1 )           //是否启动轻量级的任务同步信号，功能类似Semaphore MsgQ，内存占用要小于Semaphore MsgQ
#define SETOS_USE_SEMAPHORE                     ( 1 )           //是否启用系统信号量功能 0关闭 1启用
#define SETOS_USE_MUTEX                         ( 1 )           //是否启用互斥信号量功能 0关闭 1启用
#define SETOS_USE_MSGQ                          ( 1 )           //是否启用系统消息队列功能 0关闭 1启用
#define SETOS_MSGQ_MAX_MSGNUM                   ( 2 )           //定义消息队列中消息的门限值
#define SETOS_USE_TIMER                         ( 1 )           //是否使用系统软件定时器
#define SETOS_USE_QUICK_SCHEDULE                ( 1 )           //是否启动快速调度算法
#define SETOS_TIME_SLICE_ON                     ( 1 )           //是否启用时间片轮转（time slice）调度机制
#define SETOS_PEND_FOREVER_VALUE                ( 0xFFFFFFFFUL )//定义信号量及消息队列中永久等待的数值

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
    /* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
    #define SETHW_PRIO_BITS             __NVIC_PRIO_BITS
#else
    #define SETHW_PRIO_BITS             ( 4 )        /* 15 priority levels */
#endif

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe TINIUX API functions.  DO NOT CALL
INTERRUPT SAFE TINIUX API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
/* !!!! OSMAX_HWINT_PRI must not be set to zero !!!!*/
#define OSMAX_HWINT_PRI                 ( 0x5 << (8 - SETHW_PRIO_BITS) ) /* equivalent to 0x50, or priority 5. */
/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the OSMIN_HWINT_PRI 
setting.  Here 15 corresponds to the lowest NVIC value of 255. */
#define OSMIN_HWINT_PRI                 ( 0xF << (8 - SETHW_PRIO_BITS) ) /* equivalent to 0xF0, or priority 15. */

#define FitSVCHandler                   SVC_Handler
#define FitPendSVHandler                PendSV_Handler
#define FitOSTickISR                    SysTick_Handler

#endif /* __OS_PRESET_H_ */

