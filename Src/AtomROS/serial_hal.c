/**
  ******************************************************************************
  * @file           serial_hal.c
  * @author         goodmorning
  * @brief          串口控制台底层硬件实现。
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_dma.h"
#include "serial_hal.h"
#include "shell.h"



//---------------------HAL层相关--------------------------
// 如果要对硬件进行移植修改，修改下列宏，并提供引脚初始化程序

#define     UsartBaudRate           115200   //波特率
#define     USART_DMA_ClockInit()    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2)//跟 UsartDMAx 对应

#define     xUSART   1 //引用串口号

#if        (xUSART == 1) //串口1对应DMA

	#define xDMA     2  //对应DMA
	#define xDMATxCH 4  //发送对应 DMA 通道号
	#define xDMARxCH 4  //接收对应 DMA 通道号
	#define xDMATxStream 7
	#define xDMARxStream 2
	
#elif (xUSART == 3) //串口3对应DMA

//	#define     RemapPartial_USART
	#define xDMA     2
	#define xDMATxCH 2
	#define xDMARxCH 3
	#define xDMATxStream 7
	#define xDMARxStream 2

#endif

//---------------------HAL层宏替换--------------------------
#define USART_X(x)	                   USART##x
#define USART_x(x)	                   USART_X(x)
	 
#define USART_IRQn(x)                  USART##x##_IRQn
#define USARTx_IRQn(x)                 USART_IRQn(x)
	 
#define USART_IRQHandler(x)            USART##x##_IRQHandler
#define USARTx_IRQHandler(x)           USART_IRQHandler(x)

#define DMA_X(x)	                   DMA##x
#define DMA_x(x)	                   DMA_X(x)
	 
#define DMA_Channel(x)                 LL_DMA_CHANNEL_##x
#define DMA_Channelx(x)                DMA_Channel(x)	 

#define DMA_Stream(x)                  LL_DMA_STREAM_##x
#define DMA_Streamx(x)                 DMA_Stream(x)

#define DMA_Stream_IRQn(x,y)           DMA##x##_Stream##y##_IRQn
#define DMAx_Streamy_IRQn(x,y)         DMA_Stream_IRQn(x,y)

#define DMA_Stream_IRQHandler(x,y)     DMA##x##_Stream##y##_IRQHandler
#define DMAx_Streamy_IRQHandler(x,y)   DMA_Stream_IRQHandler(x,y)
	 
#define DMA_ClearFlag_TC(x,y)          LL_DMA_ClearFlag_TC##y(DMA##x)
#define DMAx_ClearFlag_TCy(x,y)        DMA_ClearFlag_TC(x,y)
	 
#define DMA_IsActiveFlag_TC(x,y)       LL_DMA_IsActiveFlag_TC##y(DMA##x)
#define DMAx_IsActiveFlag_TCy(x,y)     DMA_IsActiveFlag_TC(x,y)

#define UsartDMAx                  DMA_x(xDMA)     //串口所在 dma 总线
#define USARTx                     USART_x(xUSART)   //引用串口
#define UsartIRQn                  USARTx_IRQn(xUSART) //中断
#define USART_IRQ              USARTx_IRQHandler(xUSART) //中断函数名

#define UsartDmaTxCHx              DMA_Channelx(xDMATxCH)    //串口发送 dma 通道
#define UsartDmaTxStream           DMA_Streamx(xDMATxStream)
#define UsartDmaTxIRQn             DMAx_Streamy_IRQn(xDMA,xDMATxStream)
#define USART_DMA_TX_IRQ           DMAx_Streamy_IRQHandler(xDMA,xDMATxStream) //中断函数名
#define UsartDmaTxClearFlag()      DMAx_ClearFlag_TCy(xDMA,xDMATxStream)
#define UsartDmaTxCompleteFlag()   DMAx_IsActiveFlag_TCy(xDMA,xDMATxStream)

#define UsartDmaRxCHx              DMA_Channelx(xDMARxCH)
#define UsartDmaRxStream           DMA_Streamx(xDMARxStream)
#define UsartDmaRxIRQn             DMAx_Streamy_IRQn(xDMA,xDMARxStream)
#define USART_DMA_RX_IRQ          DMAx_Streamy_IRQHandler(xDMA,xDMARxStream) //中断函数名
#define UsartDmaRxClearFlag()      DMAx_ClearFlag_TCy(xDMA,xDMARxStream)
#define UsartDmaRxCompleteFlag()   DMAx_IsActiveFlag_TCy(xDMA,xDMARxStream)

//---------------------------------------------------------

#define HAL_RX_PACKET_SIZE 4     //硬件接收到的缓冲队列，以数据包为单位
#define HAL_RX_BUF_SIZE    (1024*2+1)  //硬件接收缓冲区
//#define HAL_RX_BUF_SIZE    (FLASH_PAGE_SIZE * 2 + 1)//硬件接收缓冲区
#define HAL_TX_BUF_SIZE    1024  //硬件发送缓冲区

static struct _serial_tx
{
	uint16_t pkttail ;
	uint16_t pktsize ;
	char buf[HAL_TX_BUF_SIZE];
}
serial_tx = {0};


static struct _serial_rx
{
	uint16_t pkttail;
	uint16_t pktmax;
	char buf[HAL_RX_BUF_SIZE];
}
serial_rx = {0};


static struct _serial_queue
{
	uint16_t tail ;
	uint16_t head ;
	
	uint16_t  pktlen[HAL_RX_PACKET_SIZE];
	char    * pktbuf[HAL_RX_PACKET_SIZE];
}
serial_rxpkt_queue  = {0};




#if   (xUSART == 1)
static void usart1_gpio_init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct;
  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
  
	#if 0
  /**USART1 GPIO Configuration  
  PA9   ------> USART1_TX
  PA10   ------> USART1_RX 
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	#else
  /**USART1 GPIO Configuration  
  PB6   ------> USART1_TX
  PB7   ------> USART1_RX 
  */
	
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	#endif

  /* USART1 interrupt Init 
  NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(USART1_IRQn);*/
}

#endif


/** 
	* @brief usart_dma_init 控制台 DMA 初始化
	* @param void
	* @return NULL
*/
static void usart_dma_init(void)
{
	USART_DMA_ClockInit();	 

	/* USART_RX Init */  /* USART_RX Init */
	LL_DMA_SetChannelSelection(UsartDMAx, UsartDmaRxStream, UsartDmaRxCHx);
	LL_DMA_SetDataTransferDirection(UsartDMAx, UsartDmaRxStream, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
	LL_DMA_SetStreamPriorityLevel(UsartDMAx, UsartDmaRxStream, LL_DMA_PRIORITY_MEDIUM);
	LL_DMA_SetMode(UsartDMAx, UsartDmaRxStream, LL_DMA_MODE_NORMAL);
	LL_DMA_SetPeriphIncMode(UsartDMAx, UsartDmaRxStream, LL_DMA_PERIPH_NOINCREMENT);
	LL_DMA_SetMemoryIncMode(UsartDMAx, UsartDmaRxStream, LL_DMA_MEMORY_INCREMENT);
	LL_DMA_SetPeriphSize(UsartDMAx, UsartDmaRxStream, LL_DMA_PDATAALIGN_BYTE);
	LL_DMA_SetMemorySize(UsartDMAx, UsartDmaRxStream, LL_DMA_MDATAALIGN_BYTE);
	LL_DMA_SetPeriphAddress(UsartDMAx,UsartDmaRxStream,LL_USART_DMA_GetRegAddr(USARTx)); 
	LL_DMA_DisableFifoMode(UsartDMAx, UsartDmaRxStream);

	/* USART_TX Init */
	LL_DMA_SetChannelSelection(UsartDMAx, UsartDmaTxStream, UsartDmaTxCHx);
	LL_DMA_SetDataTransferDirection(UsartDMAx, UsartDmaTxStream, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
	LL_DMA_SetStreamPriorityLevel(UsartDMAx, UsartDmaTxStream, LL_DMA_PRIORITY_MEDIUM);
	LL_DMA_SetMode(UsartDMAx, UsartDmaTxStream, LL_DMA_MODE_NORMAL);
	LL_DMA_SetPeriphIncMode(UsartDMAx, UsartDmaTxStream, LL_DMA_PERIPH_NOINCREMENT);
	LL_DMA_SetMemoryIncMode(UsartDMAx, UsartDmaTxStream, LL_DMA_MEMORY_INCREMENT);
	LL_DMA_SetPeriphSize(UsartDMAx, UsartDmaTxStream, LL_DMA_PDATAALIGN_BYTE);
	LL_DMA_SetMemorySize(UsartDMAx, UsartDmaTxStream, LL_DMA_MDATAALIGN_BYTE);
	LL_DMA_SetPeriphAddress(UsartDMAx,UsartDmaTxStream,LL_USART_DMA_GetRegAddr(USARTx));
	LL_DMA_DisableFifoMode(UsartDMAx, UsartDmaTxStream);	
  
	UsartDmaTxClearFlag();
	UsartDmaRxClearFlag();
	
	  /* DMA interrupt init 中断*/
	NVIC_SetPriority(UsartDmaTxIRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),6, 0));
	NVIC_EnableIRQ(UsartDmaTxIRQn);
	  
	NVIC_SetPriority(UsartDmaRxIRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),6, 0));
	NVIC_EnableIRQ(UsartDmaRxIRQn);
	
	LL_DMA_EnableIT_TC(UsartDMAx,UsartDmaTxStream);
	LL_DMA_EnableIT_TC(UsartDMAx,UsartDmaRxStream);
}




/** 
	* @brief usart_base_init 控制台串口参数初始化
	* @param void
	* @return NULL
*/
static void usart_base_init(void)
{
	LL_USART_InitTypeDef USART_InitStruct;

	USART_InitStruct.BaudRate = UsartBaudRate;
	USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;

	LL_USART_Init(USARTx, &USART_InitStruct);
	LL_USART_ConfigAsyncMode(USARTx);

	NVIC_SetPriority(UsartIRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),7, 0));
	NVIC_EnableIRQ(UsartIRQn);

	LL_USART_DisableIT_RXNE(USARTx);
	LL_USART_DisableIT_PE(USARTx);
	LL_USART_EnableIT_IDLE(USARTx);
	
	LL_USART_EnableDMAReq_RX(USARTx);
	LL_USART_EnableDMAReq_TX(USARTx);

	LL_USART_Enable(USARTx);
}

/**
	* @brief    设置 console 硬件发送缓存区，同时会清除接收标志位
	* @param    空
	* @return   

static inline void serial_dma_send( uint32_t memory_addr ,uint16_t buf_len)
{
//	LL_DMA_DisableIT_TC(UsartDMAx,UsartDmaTxStream);
	LL_DMA_DisableStream(UsartDMAx,UsartDmaTxStream);//发送暂不使能
	
	UsartDmaTxClearFlag();
	
	LL_DMA_SetMemoryAddress(UsartDMAx,UsartDmaTxStream,memory_addr);
	LL_DMA_SetDataLength(UsartDMAx,UsartDmaTxStream,buf_len);

	LL_DMA_EnableStream(UsartDMAx,UsartDmaTxStream);
//	LL_DMA_EnableIT_TC(UsartDMAx,UsartDmaTxStream);
}
*/


/**
	* @brief    设置 console 硬件接收缓存区，同时会清除接收标志位
	* @param    空
	* @return   
*/
static inline void serial_dma_recv( uint32_t memory_addr ,uint16_t dma_max_len)
{
	LL_DMA_DisableIT_TC(UsartDMAx,UsartDmaRxStream);
	LL_DMA_DisableStream(UsartDMAx,UsartDmaRxStream);//发送暂不使能
	
	UsartDmaRxClearFlag();
	
	LL_DMA_SetMemoryAddress(UsartDMAx,UsartDmaRxStream,memory_addr);
	LL_DMA_SetDataLength(UsartDMAx,UsartDmaRxStream,dma_max_len);

	LL_DMA_EnableStream(UsartDMAx,UsartDmaRxStream);
	LL_DMA_EnableIT_TC(UsartDMAx,UsartDmaRxStream);
}


/**
  * @brief    console 启动发送当前包
  * @param    空
  * @retval   空
  */
static inline void serial_send_pkt(void)
{
	uint32_t pkt_size = serial_tx.pktsize ;
	uint32_t pkt_head  = serial_tx.pkttail - pkt_size ;
	
	serial_tx.pktsize = 0;
	
//	serial_dma_send((uint32_t)(&serial_tx.buf[pkt_head]),pkt_size);
	LL_DMA_DisableStream(UsartDMAx,UsartDmaTxStream);//发送暂不使能
	
	UsartDmaTxClearFlag();
	
	LL_DMA_SetMemoryAddress(UsartDMAx,UsartDmaTxStream,(uint32_t)(&serial_tx.buf[pkt_head]));
	LL_DMA_SetDataLength(UsartDMAx,UsartDmaTxStream,pkt_size);

	LL_DMA_EnableStream(UsartDMAx,UsartDmaTxStream);

}



/**
  * @brief    serial_rxpkt_max_len 设置硬件接收最大包
  * @param    空
  * @retval   空
  */
void serial_rxpkt_max_len(uint16_t pktmax)
{
	serial_rx.pktmax = pktmax;
	serial_rx.pkttail = 0;
	
	serial_rxpkt_queue.tail = 0;
	serial_rxpkt_queue.head = 0;

	serial_dma_recv((uint32_t)(&serial_rx.buf[0]),pktmax);
}


int serial_busy(void)
{
	return (LL_DMA_IsEnabledStream(UsartDMAx,UsartDmaTxStream));
}



/**
	* @brief    serial_rxpkt_queue_in console 串口接收数据包队列入列
	* @param    
	* @return   空
*/
static inline void serial_rxpkt_queue_in(char * pkt ,uint16_t len)
{
	serial_rxpkt_queue.tail = (serial_rxpkt_queue.tail + 1) % HAL_RX_PACKET_SIZE;
	serial_rxpkt_queue.pktbuf[serial_rxpkt_queue.tail] = pkt;
	serial_rxpkt_queue.pktlen[serial_rxpkt_queue.tail] = len;
}


/**
	* @brief    serial_rxpkt_queue_out console 串口队列出队
	* @param    
	* @return   空
*/
int serial_rxpkt_queue_out(char ** data,uint16_t * len)
{
	if (serial_rxpkt_queue.tail != serial_rxpkt_queue.head)
	{
		serial_rxpkt_queue.head = (serial_rxpkt_queue.head + 1) % HAL_RX_PACKET_SIZE;
		*data = serial_rxpkt_queue.pktbuf[serial_rxpkt_queue.head];
		*len  = serial_rxpkt_queue.pktlen[serial_rxpkt_queue.head];
		return 1;
	}
	else
	{
		*len = 0;
		return 0;
	}
}


/**
	* @brief    hal_usart_puts console 硬件层输出
	* @param    空
	* @return   空
*/
void serial_puts(char * buf,uint16_t len)
{
	while(len)
	{
		uint32_t pkttail = serial_tx.pkttail;              //先获取当前尾部地址
		uint32_t remain  = HAL_TX_BUF_SIZE - pkttail - 1;
		uint32_t pktsize = (remain > len) ? len : remain;
		
		memcpy(&serial_tx.buf[pkttail] , buf , pktsize);//把数据包拷到缓存区中
		
		pkttail += pktsize;
		buf  += pktsize;
		len  -= pktsize; 
		
		serial_tx.pkttail = pkttail;	   //更新尾部
		serial_tx.pktsize += pktsize;//设置当前包大小

		//开始发送
		if (!LL_DMA_IsEnabledStream(UsartDMAx,UsartDmaTxStream))
			serial_send_pkt();

		if (len) 
			while(LL_DMA_IsEnabledStream(UsartDMAx,UsartDmaTxStream)) ;//未发送完等待
	}
}



//------------------------------以下为一些中断处理------------------------------
#include "cmsis_os.h"//用了freertos 打开

#ifdef _CMSIS_OS_H
	extern osSemaphoreId osSerialRxSemHandle;
#else
	#include "AtomRos.h"
	extern ros_semaphore_t rosSerialRxSem;
#endif

/**
	* @brief    USART_DMA_TX_IRQ console 串口发送一包数据完成中断
	* @param    空
	* @return   空
*/
void USART_DMA_TX_IRQ(void) 
{
	if (serial_tx.pktsize == 0) //发送完此包后无数据，复位缓冲区
	{
		serial_tx.pkttail = 0;
		LL_DMA_DisableStream(UsartDMAx,UsartDmaTxStream);
		UsartDmaTxClearFlag();
	}
	else
	{
		serial_send_pkt(); //还有数据则继续发送
	}
}


/**
	* @brief    USART_DMA_RX_IRQ console 串口接收满中断
	* @param    空
	* @return   空
*/
void USART_DMA_RX_IRQ(void) 
{
	serial_rxpkt_queue_in(&(serial_rx.buf[serial_rx.pkttail]),serial_rx.pktmax); //把当前包地址和大小送入缓冲队列
	
	serial_rx.pkttail += serial_rx.pktmax ; //更新缓冲地址
	
	if (serial_rx.pkttail + serial_rx.pktmax > HAL_RX_BUF_SIZE) //如果剩余空间不足以缓存最大包长度，从 0 开始
		serial_rx.pkttail = 0;
	
	UsartDmaRxClearFlag();
	serial_dma_recv((uint32_t)&(serial_rx.buf[serial_rx.pkttail]),serial_rx.pktmax);//设置缓冲地址和最大包长度

	#ifdef _CMSIS_OS_H	
		osSemaphoreRelease(osSerialRxSemHandle);// 释放信号量
	#else
		task_semaphore_release(&rosSerialRxSem);
	#endif
}



/**
	* @brief    USART_IRQ 串口中断函数，只有空闲中断
	* @param    空
	* @return   空
*/
void USART_IRQ(void) 
{
	uint16_t pkt_len ;
	
	LL_USART_ClearFlag_IDLE(USARTx); //清除空闲中断
	
	pkt_len = serial_rx.pktmax - LL_DMA_GetDataLength(UsartDMAx,UsartDmaRxStream);//得到当前包的长度
	
	if (pkt_len)
	{
		serial_rxpkt_queue_in(&(serial_rx.buf[serial_rx.pkttail]),pkt_len); //把当前包送入缓冲队列，交由应用层处理
	
		serial_rx.pkttail += pkt_len ;	 //更新缓冲地址
		if (serial_rx.pkttail + serial_rx.pktmax > HAL_RX_BUF_SIZE)//如果剩余空间不足以缓存最大包长度，从 0 开始
			serial_rx.pkttail = 0;

		serial_dma_recv((uint32_t)&(serial_rx.buf[serial_rx.pkttail]),serial_rx.pktmax);//设置缓冲地址和最大包长度

		#ifdef _CMSIS_OS_H
			osSemaphoreRelease(osSerialRxSemHandle);// 释放信号量	
		#else
			task_semaphore_release(&rosSerialRxSem);
		#endif
	}
}


//------------------------------华丽的分割线------------------------------
/**
	* @brief    hal_serial_init console 硬件层初始化
	* @param    空
	* @return   空
*/
void hal_serial_init(void)
{
	//引脚初始化
	#if   (xUSART == 1) 
		usart1_gpio_init();
	#elif (xUSART == 3) 
		vUsartHal_USART3_GPIO_Init();	
	#endif

	usart_base_init();
	usart_dma_init();
	
	serial_tx.pkttail = 0;
	serial_tx.pktsize = 0;
	
	serial_rxpkt_max_len(COMMANDLINE_MAX_LEN);
}


void hal_serial_deinit(void)
{
	NVIC_DisableIRQ(UsartDmaTxIRQn);
	NVIC_DisableIRQ(UsartDmaRxIRQn);
	NVIC_DisableIRQ(UsartIRQn);
	
	LL_DMA_DisableIT_TC(UsartDMAx,UsartDmaTxStream);
	LL_DMA_DisableIT_TC(UsartDMAx,UsartDmaRxStream);	
	
	LL_USART_DisableDMAReq_RX(USARTx);
	LL_USART_DisableDMAReq_TX(USARTx);

	LL_USART_Disable(USARTx);	
	
}
