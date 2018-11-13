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

#include "AtomROS.h"
#include "iap_hal.h"

#include "shell.h"
#include "serial_hal.h"
#include "serial_console.h"

#include "stm32f429xx.h" //for SCB->VTOR

//--------------------相关宏定义及结构体定义--------------------


#define UASRT_IAP_BUF_SIZE  1024

const static char iap_logo[]=
"\r\n\
 ____   ___   ____\r\n\
|_  _| / _ \\ |  _ \\\r\n\
 _||_ |  _  ||  __/don't press any key now\r\n\
|____||_| |_||_|   ";



static struct st_console_iap
{
	uint32_t flashaddr;
	uint32_t timestamp;
}
serial_iap;

ros_task_t stSerialConsoleTask;
ros_task_t stUsartIapTask;
ros_task_t stUsartIapTimeoutTask;
ros_semaphore_t rosSerialRxSem;



static struct shell_buf serial_shellbuf;
//------------------------------相关函数声明------------------------------




//------------------------------华丽的分割线------------------------------



/**
	* @brief task_UsartIapCompletePro iap 升级超时判断任务
	* @param void
	* @return NULL
*/
int task_UsartIapCompletePro(void * arg)
{
	TASK_BEGIN();//任务开始
	
	printk("loading");
	
	task_cond_wait(OS_current_time - serial_iap.timestamp > 2000) ;//超时 2.5 s
	
	task_cancel(&stUsartIapTask); // 删除 iap 任务
	
	iap_lock_flash();   //由于要写完最后一包数据才能上锁，所以上锁放在 task_UsartIapCompletePro 中
	
	serial_rxpkt_max_len(COMMANDLINE_MAX_LEN);
	
	uint32_t filesize = (SCB->VTOR == FLASH_BASE) ? (serial_iap.flashaddr-APP_ADDR):(serial_iap.flashaddr-IAP_ADDR);
	printk("\r\nupdate completed!\r\nupdate package size:%d byte\r\n",filesize);

	TASK_END();
}


/** 
	* @brief task_UsartIapPro  iap 升级任务
	* @param void
	* @return NULL
*/
int task_UsartIapPro(void * arg)
{
	uint16_t pktsize;
	char   * pktdata;

	uint32_t * value;
	
	TASK_BEGIN();//任务开始
	
	iap_unlock_flash();//由于要写完最后一包数据才能上锁，所以上锁放在 task_UsartIapCompletePro 中
	
	if (SCB->VTOR == FLASH_BASE)//如果是 iap 模式，擦除 app 区域
	{
		serial_iap.flashaddr = APP_ADDR;
		if(iap_erase_flash(5)) //app 地址在 0x8020000,删除扇区5数据
		{
			Error_Here();//发生错误了	
			task_exit();
		}
	}	
 	else
 	{
		serial_iap.flashaddr = IAP_ADDR; //iap 地址在 0x8000000,删除扇区0数据
		for (uint32_t  sector = 0 ; sector < 3 ; ++sector)
		{
			if(iap_erase_flash(sector))
			{
				Error_Here();//发生错误了	
				task_exit();
			}
		}
 	}
	
	color_printk(light_green,"\033[2J\033[%d;%dH%s",0,0,iap_logo);//清屏
	
	while(1)
	{
		//task_cond_wait(iUsartHal_RxPktOut(&pktdata,&pktsize));
		task_semaphore_wait(&rosSerialRxSem);//等待接收到一包数据
		
		while (serial_rxpkt_queue_out(&pktdata,&pktsize))
		{
			value = (uint32_t*)pktdata;
			
			for (uint32_t i = 0;i < pktsize ; i += 4) // f4 可以以 word 写入
			{
				iap_write_flash(serial_iap.flashaddr,*value);
				serial_iap.flashaddr += 4;
				++value;
			}
			
			serial_iap.timestamp = OS_current_time;//更新时间戳
			
			if (task_is_exited(&stUsartIapTimeoutTask))
				task_create(&stUsartIapTimeoutTask,NULL,task_UsartIapCompletePro,NULL);
			else
				printk(".");
		}
	}
	
	TASK_END();
}




/** 
	* @brief task_SerialConsole  串口控制台处理
	* @param void
	* @return NULL
*/
int task_SerialConsole(void * arg)
{
	char  *  packet;
	uint16_t pktlen ;

	TASK_BEGIN();//任务开始

	task_semaphore_init(&rosSerialRxSem);
	
	while(1)
	{
		task_semaphore_wait(&rosSerialRxSem);
		//task_cond_wait(iUsartHal_RxPktOut(&packet,&pktlen));

		while (serial_rxpkt_queue_out(&packet,&pktlen))
			shell_input(&serial_shellbuf,packet,pktlen);//数据帧传入应用层
		
		task_join(&stUsartIapTask); //在线升级时数据流往 iap 任务走
	}
	
	TASK_END();
}




/** 
	* @brief shell_iap_command  iap 升级命令
	* @param void
	* @return NULL
*/
void shell_iap_command(void * arg)
{
	task_create(&stUsartIapTask,NULL,task_UsartIapPro,NULL);
	serial_rxpkt_max_len(UASRT_IAP_BUF_SIZE);
}





void serial_console_init(char * info)
{
	hal_serial_init(); //先初始化硬件层
	
	SHELL_MALLOC(&serial_shellbuf,serial_puts);

	if (SCB->VTOR != FLASH_BASE)
	{
		shell_register_command("update-iap",shell_iap_command);	
	}
	else
	{
		shell_register_command("update-app",shell_iap_command);	
		shell_register_command("jump-app",shell_jump_command);
	}

	shell_register_command("reboot"  ,shell_reboot_command);
	
	task_create(&stSerialConsoleTask,NULL,task_SerialConsole,NULL);
	
	color_printk(purple,"%s",info);//打印开机信息或者控制台信息
	
	while(serial_busy()); //等待打印结束
}






