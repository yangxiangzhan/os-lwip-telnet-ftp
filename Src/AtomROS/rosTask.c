/**
  ******************************************************************************
  * @file            OS.c
  * @author         杨翔湛
  * @brief           OS file .用协程模拟一个简单的操作系统
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <stdio.h>

#include "liblist.h"  //from linux kernel
#include "rosTask.h"


#define OS_CONF_NUMEVENTS 8 //事件队列深度


// 系统运行一个任务
#define OS_call(task) \
		do{if(TASK_EXITED == (task)->func((task)->arg)) list_del_init(&((task)->list_node));}while(0)
	

struct event_data
{
	struct protothread * task;
};


struct list_head       OS_scheduler_list = {&OS_scheduler_list,&OS_scheduler_list};//系统调度链表入口
struct protothread *   OS_current_task = NULL;//系统当前运行的任务
volatile unsigned long OS_current_time = 0;     //系统时间，ms

static unsigned char nevents = 0; //未处理事件个数
static unsigned char fevent = 0;  //事件队列下标
static struct event_data events[OS_CONF_NUMEVENTS];//事件队列


/** 
	* @brief OS_create_task :注册一个  task
	* @param TaskFunc      : Task 对应的执行函数指针
	* @param task          : Task 对应的任务控制结构体
	* @param pTaskName     : Task 名
	*
	* @return NULL
*/
void OS_task_create(ros_task_t *task,const char * name, int (*start_rtn)(void*), void *arg)
{
	static unsigned short IDcnt = 1;
	
	if ( task->init != TASK_IS_INITIALIZED)
	{
		INIT_LIST_HEAD(&task->list_node);
		
		task->init = TASK_IS_INITIALIZED;
		task->func = start_rtn;//任务执行函数
		task->arg  = arg;
		
		#ifdef  OS_USE_ID_AND_NAME
			task->name = name;	 //任务名
			task->ID = IDcnt;
		#endif
		
		++IDcnt;
	}

	if (list_empty(&task->list_node))//如果任务已在运行，不重复注册
	{
		task->lc = 0;
		task->dly = 0;
		task->post = 0;
		list_add_tail(&task->list_node, &OS_scheduler_list);//加入调度链表末端
	}
}




/** 
	* @brief OS_task_post : post 一个任务事件；
	*                     post 的事件优先级比在链表内的任务优先级高
	* @param task
	*
	* @return NULL
*/
void OS_task_post(struct protothread * task)
{
	unsigned char index;
	
	if (nevents == OS_CONF_NUMEVENTS || task->post || 
		task->init != TASK_IS_INITIALIZED) //队列满或者已在队列或者未初始化的
	{
		return ;
	}

	index = (fevent + nevents) % OS_CONF_NUMEVENTS;
	++nevents;

	task->post = 1;
	events[index].task = task;
}	





/** 
	* @brief run task
	* @param NULL
	*
	* @return NULL
*/
void OS_scheduler(void)
{
	struct list_head * SchedulerListThisNode;
	struct list_head * SchedulerListNextNode;
	
	list_for_each_safe(SchedulerListThisNode,SchedulerListNextNode,&OS_scheduler_list)
	{
		//获取当前链表节点对应的任务控制块指针
		OS_current_task = list_entry(SchedulerListThisNode,struct protothread,list_node); 
		
		if (OS_current_task->dly)//休眠期内的任务不执行，所以其任务时间被冻结，直到到达时间点
		{
			if (OS_current_time - OS_current_task->time < OS_current_task->dly) 
				continue; //当然这个 continue 也会跳过 post 事件处理，这么处理要考虑一下
			else
				OS_current_task->dly = 0;
		}
			
		OS_call(OS_current_task);//执行链表中的任务

		if (nevents) //处理 post 事件
		{
			OS_current_task = events[fevent].task;
			
			--nevents;
			fevent = (fevent + 1) % OS_CONF_NUMEVENTS;
			
			OS_current_task->post = 0;//清除此事件的 post 标志，如此才可以反复post
			OS_call(OS_current_task);
		}
	}
}


