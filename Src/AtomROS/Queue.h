#ifndef  __QUEUE_H__
#define   __QUEUE_H__



#define    QueueBufSize   10
typedef struct 
{
	volatile int usQueueBuf[QueueBufSize];
	volatile unsigned char ucHead;
	volatile unsigned char ucTail;
}
stQueueDef;




#define   QUEUE_EMPTY       0
#define   QUEUE_NOT_EMPTY   1

#define   QueueNotFull(pStQueue)     ((((pStQueue)->ucTail+1)%QueueBufSize) != (pStQueue)->ucHead)
#define   QueueFull(pStQueue)        ((((pStQueue)->ucTail+1)%QueueBufSize) == (pStQueue)->ucHead)

#define   QueueNotEmpty(pStQueue)    ((pStQueue)->ucTail != (pStQueue)->ucHead)
#define   QueueEmpty(pStQueue)       ((pStQueue)->ucTail == (pStQueue)->ucHead)



static inline int  sQueueOut(stQueueDef * pStQueue)
{
	int iQueueOutValue;
	iQueueOutValue = pStQueue->usQueueBuf[pStQueue->ucHead];
	pStQueue->ucHead = (pStQueue->ucHead + 1) % QueueBufSize;

	return iQueueOutValue;
}

static inline  void vQueueIn(stQueueDef * pStQueue,int iQueueInValue)
{
	pStQueue->usQueueBuf[pStQueue->ucTail] = iQueueInValue ;
	pStQueue->ucTail = (pStQueue->ucTail + 1) % QueueBufSize;
}

static inline  void vQueueReset(stQueueDef * pStQueue)
{
	pStQueue->ucTail = 0;
	pStQueue->ucHead = 0;
}


#endif


