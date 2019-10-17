 /*
 * FreeRTOS Kernel V10.0.0
 * Copyright (C) 2010-2018 Xilinx, Inc. All Rights Reserved.
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef _FREERTOSCONFIG_H
#define _FREERTOSCONFIG_H

#include "xparameters.h" 

#define configUSE_PREEMPTION 1

#define configUSE_MUTEXES 1

#define INCLUDE_xSemaphoreGetMutexHolder 1

#define configUSE_RECURSIVE_MUTEXES 1

#define configUSE_COUNTING_SEMAPHORES 1

#define configUSE_TIMERS 1

#define configUSE_IDLE_HOOK 0

#define configUSE_TICK_HOOK 0

#define configUSE_DAEMON_TASK_STARTUP_HOOK 0

#define configUSE_MALLOC_FAILED_HOOK 1

#define configUSE_TRACE_FACILITY 1

#define configUSE_NEWLIB_REENTRANT 0

#define configSTREAM_BUFFER 0

#define configMESSAGE_BUFFER 0

#define configSUPPORT_STATIC_ALLOCATION 0

#define configUSE_16_BIT_TICKS 0

#define configUSE_APPLICATION_TASK_TAG 0

#define configUSE_CO_ROUTINES 0

#define configTICK_RATE_HZ (100)

#define configMAX_PRIORITIES (8)

#define configMAX_CO_ROUTINE_PRIORITIES 2

#define configMINIMAL_STACK_SIZE ( ( unsigned short ) 200)

#define configTOTAL_HEAP_SIZE ( ( size_t ) ( 50331648 ) )

#define configMAX_TASK_NAME_LEN 10

#define configIDLE_SHOULD_YIELD 1

#define configUSE_TIME_SLICING 1

#define configTIMER_TASK_PRIORITY (configMAX_PRIORITIES - 1)

#define configTIMER_QUEUE_LENGTH 10

#define configTIMER_TASK_STACK_DEPTH ((configMINIMAL_STACK_SIZE) * 2)

#define configASSERT( x ) if( ( x ) == 0 ) vApplicationAssert( __FILE__, __LINE__ )

#define configUSE_QUEUE_SETS 1

#define configUSE_TASK_NOTIFICATIONS 1

#define configCHECK_FOR_STACK_OVERFLOW 2

#define configUSE_TASK_FPU_SUPPORT 1

#define configQUEUE_REGISTRY_SIZE 10

#define configUSE_STATS_FORMATTING_FUNCTIONS 1

#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0

#define configGENERATE_RUN_TIME_STATS 0

#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()

#define portGET_RUN_TIME_COUNTER_VALUE()

#define configUSE_TICKLESS_IDLE	0
#define configTASK_RETURN_ADDRESS    NULL
#define INCLUDE_vTaskPrioritySet             1
#define INCLUDE_uxTaskPriorityGet            1
#define INCLUDE_vTaskDelete                  1
#define INCLUDE_vTaskCleanUpResources        1
#define INCLUDE_vTaskSuspend                 1
#define INCLUDE_vTaskDelayUntil              1
#define INCLUDE_vTaskDelay                   1
#define INCLUDE_eTaskGetState                1
#define INCLUDE_xTimerPendFunctionCall       1
#define INCLUDE_pcTaskGetTaskName            1
#define portPOINTER_SIZE_TYPE	uint32_t
#define portTICK_TYPE_IS_ATOMIC 0
#define configMESSAGE_BUFFER_LENGTH_TYPE uint32_t
#define configSTACK_DEPTH_TYPE uint32_t
#define configTIMER_ID XPAR_XTTCPS_0_DEVICE_ID

#define configTIMER_BASEADDR XPAR_XTTCPS_0_BASEADDR

#define configTIMER_INTERRUPT_ID XPAR_XTTCPS_0_INTR

#define configUNIQUE_INTERRUPT_PRIORITIES 32

#define configINTERRUPT_CONTROLLER_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID

#define configINTERRUPT_CONTROLLER_BASE_ADDRESS XPAR_SCUGIC_0_DIST_BASEADDR

#define configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET 0x1000

void vApplicationAssert( const char *pcFile, uint32_t ulLine );
void FreeRTOS_SetupTickInterrupt( void );
#define configSETUP_TICK_INTERRUPT() FreeRTOS_SetupTickInterrupt()

void FreeRTOS_ClearTickInterrupt( void );
#define configCLEAR_TICK_INTERRUPT()	FreeRTOS_ClearTickInterrupt()

#define configCOMMAND_INT_MAX_OUTPUT_SIZE 2096

#define recmuCONTROLLING_TASK_PRIORITY ( configMAX_PRIORITIES - 2 )

#define portSET_INTERRUPT_MASK_FROM_ISR()	ulPortSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	vPortClearInterruptMask(x)
#define configMAX_API_CALL_INTERRUPT_PRIORITY (18)

#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

#ifdef FREERTOS_ENABLE_TRACE
#include "FreeRTOSSTMTrace.h"
#endif /* FREERTOS_ENABLE_TRACE */

#endif
