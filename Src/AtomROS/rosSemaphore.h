
#ifndef _ros_Semaphore_h_
#define _ros_Semaphore_h_


//------------------简单信号量实现--------------------
typedef struct{
	volatile int signal;
	volatile struct protothread * wait;
}ros_semaphore_t;

// 初始化一个信号量
#define task_semaphore_init(sph)  do{(sph)->signal = 0;(sph)->wait = NULL;}while(0)

// 释放一个信号量，比如一个中断.会 post 一个事件，加速响应
#define task_semaphore_release(sph)\
do{\
	(sph)->signal = 1;  \
	if((sph)->wait)     \
		OS_task_post((sph)->wait);\
}while(0)

// 等待一个二值信号量
#define task_semaphore_task(sph)\
do{\
	(sph)->wait = task ;          \
	task_cond_wait((sph)->signal);\
	task_semaphore_init(sph);     \
}while(0)
	

#endif


