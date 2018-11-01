/**
  ******************************************************************************
  * @file           serial_console.c
  * @author         杨翔湛
  * @brief          serial_console file
                    串口控制台文件。文件不直接操作硬件，依赖 serial_hal
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdint.h> //定义了很多数据类型

#include "cmsis_os.h" // 启用 freertos

#include "shell.h"
#include "serial_hal.h"
#include "serial_console.h"

//--------------------相关宏定义及结构体定义--------------------
osThreadId SerialConsoleTaskHandle;
osSemaphoreId osSerialRxSemHandle;
static char acTaskListBuf[512];
//------------------------------相关函数声明------------------------------



//------------------------------华丽的分割线------------------------------

/*
	* for freertos;
	configUSE_STATS_FORMATTING_FUNCTIONS    1
	portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  1
	configUSE_TRACE_FACILITY 1
	
	#ifndef portCONFIGURE_TIMER_FOR_RUN_TIME_STATS
		extern uint32_t iThreadRuntime;
		#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() (iThreadRuntime = 0)
		#define portGET_RUN_TIME_COUNTER_VALUE() (iThreadRuntime)
	#endif
*/
void vShell_GetTaskList(void * arg)
{
	static const char acTaskInfo[] = "\r\nthread\t\tstate\tPrior\tstack\tID\r\n----------------------------\r\n";
	static const char acTaskInfoDescri[] = "\r\n----------------------------\r\nB(block),R(ready),D(delete),S(suspended)\r\n";

	osThreadList((uint8_t *)acTaskListBuf);
	printk("%s%s%s",acTaskInfo,acTaskListBuf,acTaskInfoDescri);
}


void vShell_GetTaskRuntime(void * arg)
{
	static const char acTaskInfo[] = "\r\nthread\t\ttime\t\t%CPU\r\n----------------------------\r\n";
	static const char acTaskInfoDescri[] = "\r\n----------------------------\r\n";

	vTaskGetRunTimeStats(acTaskListBuf);
	printk("%s%s%s",acTaskInfo,acTaskListBuf,acTaskInfoDescri);
}




void task_SerialConsole(void const * argument)
{
	struct shell_buf stUsartShellBuf;
	char *info = (char *)argument;
	uint16_t len ;

	vUsartHal_Init(); //先初始化硬件层
	vShell_InitBuf(&stUsartShellBuf,vUsartHal_Output);
	
	vShell_RegisterCommand("top"   ,vShell_GetTaskList);
	vShell_RegisterCommand("ps"    ,vShell_GetTaskRuntime);
	vShell_RegisterCommand("reboot",vShell_RebootSystem);
	
	printk("\r\n");
	color_printk(purple,"%s",info);//打印开机信息或者控制台信息
	
	for(;;)
	{
		if (osOK == osSemaphoreWait(osSerialRxSemHandle,osWaitForever))
		{
			while(iUsartHal_RxPktOut(&info,&len))
				vShell_Input(&stUsartShellBuf,info,len);
		}
	}
}


void vSerialConsole_Init(char * info)
{
	osSemaphoreDef(osSerialRxSem);
	osSerialRxSemHandle = osSemaphoreCreate(osSemaphore(osSerialRxSem), 1); //创建中断信号量
  
	osThreadDef(SerialConsole, task_SerialConsole, osPriorityNormal, 0, 256);
	SerialConsoleTaskHandle = osThreadCreate(osThread(SerialConsole), info);
}






