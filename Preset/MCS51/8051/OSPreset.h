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

#include <mcs51/8051.h>
/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 *----------------------------------------------------------*/

#define SETOS_CPU_CLOCK_HZ                      ( ( unsigned long ) 24000000 ) //定义CPU运行主频 (如72000000)
#define SETOS_TICK_RATE_HZ                      ( 1000 )             //定义TINIUX系统中ticks频率

#define SETOS_MINIMAL_STACK_SIZE                ( ( unsigned short ) 100 )     //任务运行时的Stack空间，51架构下系统中所有任务运行时均使用此大小的stack空间
#define SETOS_TOTAL_HEAP_SIZE                   ( 1024*2 )   //定义系统占用的Heap空间
#define SETOS_ENABLE_MEMFREE                    ( 0 )        //是否允许释放内存，允许后可以在系统运行时删除Task MsgQ Semaphone Mutex Timer等
#define SETOS_LOWPOWER_MODE                     ( 0 )        //是否开启低功耗模式
#define SETOS_MAX_NAME_LEN                      ( 8 )        //定义任务、信号量、消息队列等变量中的名称长度
#define SETOS_MAX_PRIORITIES                    ( 4 )        //定义任务最大优先级
#define SETOS_TASK_SIGNAL_ON                    ( 1 )        //是否启动轻量级的任务同步信号，功能类似Semaphore MsgQ，内存占用要小于Semaphore MsgQ
#define SETOS_USE_SEMAPHORE                     ( 0 )        //是否启用系统信号量功能 0关闭 1启用
#define SETOS_USE_MUTEX                         ( 0 )        //是否启用互斥信号量功能 0关闭 1启用
#define SETOS_USE_MSGQ                          ( 0 )        //是否启用系统消息队列功能 0关闭 1启用
#define SETOS_MSGQ_MAX_MSGNUM                   ( 5 )        //定义消息队列中消息的门限值
#define SETOS_USE_TIMER                         ( 0 )        //是否使用系统软件定时器
#define SETOS_USE_QUICK_SCHEDULE                ( 0 )        //是否启动快速调度算法
#define SETOS_TIME_SLICE_ON                     ( 1 )        //是否启用时间片轮转（time slice）调度机制
#define SETOS_PEND_FOREVER_VALUE                ( 0xFFFFU )  //定义信号量及消息队列中永久等待的数值

#endif /* __OS_PRESET_H_ */

