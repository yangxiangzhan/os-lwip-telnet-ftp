#ifndef _ROS_TASK_H_
#define _ROS_TASK_H_

#ifndef NULL
	#define NULL ((void*)0)
#endif

#define OS_USE_ID_AND_NAME //是否使用 id 号和名字，其实除非搭载了 shell ，否则使用这个是没有意义的


//--------------------------------------

enum TASK_STATUS_VALUE
{
	TASK_WAITING= 0 ,
	TASK_EXITED ,
};



typedef struct protothread  //基于 Protothread 机制拓展的控制块
{
	volatile unsigned short lc;     //Local Continuation ,Protothread 协程的核心
	volatile unsigned short dly;    //delay/sleep
	volatile unsigned int time;     //调度时间点，用于实现 sleep 和超时 yield

	unsigned char post;             //post 事件标志
	unsigned char init;             //init 标志
	#define TASK_IS_INITIALIZED 0x9A //init 值
	
	#ifdef OS_USE_ID_AND_NAME
		unsigned short ID;          // id 号
		const char *name;           // 任务名
	#endif

	void * arg;
	int(*func)(void *);//任务函数入口的标准形式
	
	struct list_head list_node; //调度链表块
}
ros_task_t;


//-------------------TASK 定义-------------------


// 任务开始，放在任务函数开头。大写，表示一定要有 
#define TASK_BEGIN()    do{\
							int yield = 1;               \
							ros_task_t * task = task_self(); \
							task->time = OS_current_time;\
                            if (yield)                   \
							switch(task->lc)             \
							{ case 0: 
							
#define TASK_END()      	}\
							task_exit();\
						}while(0)
// 任务结束，放在任务函数结尾处。大写，表示一定要有


//-----------------------------------------------------------------
/* TASK 内部可用操作,必须放在 TASK_BEGIN() 和 TASK_END() 之间
 *  因为需要用到 TASK_BEGIN() 定义的 task 和 yield 变量
 * 
 * 注意事项：不能在任务内的 switch 实现以下阻塞挂起功能，即
 * char task(void * arg)
 * {
	 TASK_BEGIN();
	 task_sleep(100); //常规操作
	 switch(xxx)
	 {
		case 2:task_yield(); //不允许，因为任务的恢复挂起本身要靠case来实现 
	 }
	 TASK_END();
 * }
*/
// 任务等待某条件成立，如果不成立则阻塞。抄袭linux 的 pthred_cond_wait(),不过有点不一样	
#define task_cond_wait(cond) \
	do{(task)->lc = __LINE__;case __LINE__:if(!(cond)) return TASK_WAITING;}while(0)

						
// 任务等待某条件失效，如果仍成立则阻塞，自己加的
#define task_cond_while(cond) \
	do{(task)->lc = __LINE__;case __LINE__:if((cond)) return TASK_WAITING;}while(0)


// 任务等待其他线程任务结束，如果未结束则阻塞。抄袭linux 的 pthred_join()					
#define task_join(thread)  task_cond_wait(task_is_exited(thread))

// 任务阻塞一段时间，单位毫秒 ms 
#define task_sleep(x_ms)   do{(task)->dly = x_ms;task_yield();}while(0)


// 任务让出 cpu
#define task_yield()  \
    do {\
      yield = 0;\
      (task)->lc = __LINE__;case __LINE__:if(!yield){return TASK_WAITING;}\
    }while(0)
	

// 用在时间很长的循环或计算中 , 超时(1ms)让出 cpu ，并 post 一个事件
#define task_timeout_yield() if (OS_current_time != task->time) do{OS_task_post(task);task_yield();}while(0)


// 立即退出任务，并不再执行此任务
#define task_exit()        do{task_cancel(task);return TASK_EXITED;}while(0)


	

//-------------------TASK 外部可用操作-------------------
// 创建任务并开始运行 , 抄袭 linux 的 pthread_create(),第二个参数先留着备用
#ifdef OS_USE_ID_AND_NAME
	#define task_create(tidp,x,func,arg) OS_task_create((tidp),#func,func,arg)
#else
	#define task_create(tidp,x,func,arg) OS_task_create((tidp),NULL,func,arg)
#endif

// 删除一个任务,不直接从列表删除，置位 lc 由调度器删除。有可能产生互斥死锁，注意使用
#define task_cancel(task)     do{(task)->lc = 1;(task)->dly = 0;}while(0)


// 获取任务是否在运行 ， 这个是 linux 线程库没有的
#define task_is_running(task)   ((task)->init == TASK_IS_INITIALIZED && (task)->lc != 1)
#define task_is_exited(task)    (!task_is_running(task))



#if 0
	#define OS_Start() OS_PSP();} void OS_Start_func(void) {
#else
	#define OS_Start() 
#endif



//------------------系统对外声明--------------------

extern struct list_head       OS_scheduler_list;
extern struct protothread *   OS_current_task;
#define task_self()           OS_current_task

extern volatile unsigned long OS_current_time;
#define OS_heartbeat()        do{++OS_current_time;}while(0)
	

void    OS_task_create(ros_task_t *task,const char * name, int (*start_rtn)(void*), void *arg);
void    OS_task_post  (ros_task_t * task);
void    OS_scheduler(void);

#endif

