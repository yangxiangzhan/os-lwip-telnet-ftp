


#ifndef __pseudo_mutex_h__
#define __pseudo_mutex_h__


//------------------简单互斥量实现--------------------
typedef struct{
	volatile struct protothread * lock;
}ros_mutex_t; 


// 初始化一个 mutex 
#define task_mutex_init(mx)       (mx)->lock = NULL

// 上锁 mutex ，失败则挂起，直至上锁成功 
#define task_mutex_lock(mx)       task_cond_while((mx)->lock);(mx)->lock = task

// 解锁 mutex ,不能解锁另一个任务上的锁
#define task_mutex_unlock(mx)     if ((mx)->lock == task)  (mx)->lock = NULL

#define task_mutex_is_locked(mx)  ((mx)->lock)
	

#endif
