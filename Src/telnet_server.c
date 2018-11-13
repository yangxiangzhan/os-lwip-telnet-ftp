/**
  ******************************************************************************
  * @file           telent_server.c
  * @author         古么宁
  * @brief          telent 服务器
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "avltree.h"
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h" 

#include "shell.h"
#include "ustdio.h"
#include "fatfs.h"


/* Private macro ------------------------------------------------------------*/

#define TELNET_WILL  251
#define TELNET_WONT  252
#define TELNET_DO    253
#define TELNET_DONT  254
#define TELNET_IAC   255


#define TELNET_NORMAL	   0
#define TELNET_BIN_TRAN    1
#define TELNET_BIN_ERROR   2



#define TELNET_FILE_ADDR 0x8060000
/*
* secureCRT telnet 发送文件大概流程：
* CRT   : will bin tran ; do bin tran
* server: do bin tran
* CRT   : will bin tran 
* CRT   : <file data>
* CRT   : won't bin tran ; don't bin tran
* server: won't bin tran ; don't bin tran
* CRT   : won't bin tran 
* server: won't bin tran 
* CRT   : <string mode>
*/
	
/* Private types ------------------------------------------------------------*/




/* Private variables ------------------------------------------------------------*/

static struct telnetbuf
{
	struct netconn * conn;  //对应的 netconn 指针
	volatile uint32_t tail; //telnet 数据缓冲区末尾
	char buf[TCP_MSS];    //telnet 数据缓冲区 __align(4) 
}
current_telnet, //当前正在处理的 telnet 连接
bind_telnet;    //绑定了 printf/printk 的 telnet 连接


static struct telnetfile
{
	uint16_t skip0xff;
	uint16_t remain ;
	uint32_t addr ;
	char buf[TCP_MSS+4];
}
telnet_file;

//static char telnet_state = TELNET_NORMAL; //



/*---------------------------------------------------------------------------*/

static void telnet_puts(char * buf,uint16_t len)
{
	struct telnetbuf * putsbuf;

	if ( current_telnet.conn )     //当前正在处理 telnet 连接
		putsbuf = &current_telnet;
	else
	if ( bind_telnet.conn )      //绑定 printf/printk 的 telnet 连接
		putsbuf = &bind_telnet;
	else
		return ;
	
	if ( putsbuf->tail + len < TCP_MSS) 
	{
		memcpy(&putsbuf->buf[putsbuf->tail],buf,len);
		putsbuf->tail += len;
	}
}




/**
	* @brief    telnet_option 
	*           telnet 配置
	* @param    arg 任务参数
	* @return   void
*/
static void telnet_option(uint8_t option, uint8_t value) //telnet_option(TELNET_DONT,1)
{
	volatile uint32_t new_tail = 3 + current_telnet.tail;
	
	if ( new_tail < TCP_MSS ) 
	{
		char * buf = &current_telnet.buf[current_telnet.tail];
		*buf = (char)TELNET_IAC;
		*++buf = (char)option;
		*++buf = (char)value;
		current_telnet.tail = new_tail;
	}
}



/**
	* @brief    telnet_check_option 
	*           telnet 连接时需要检测回复客户端的选项配置
	* @param    arg 任务参数
	* @return   void
*/
void telnet_check_option(char ** telnetbuf , uint16_t * buflen ,uint32_t * telnetstate)
{
	uint8_t iac = (uint8_t)((*telnetbuf)[0]);
	uint8_t opt = (uint8_t)((*telnetbuf)[1]);
	uint8_t val = (uint8_t)((*telnetbuf)[2]);

	if (TELNET_NORMAL == *telnetstate)
	{
		while(iac == TELNET_IAC && opt > 250 )
		{
			if (0 == val) //只回复二进制命令
			{
				if (TELNET_WILL == opt)
				{
					*telnetstate = TELNET_BIN_TRAN;
					telnet_file.addr = TELNET_FILE_ADDR;
					telnet_file.remain = 0;
					telnet_file.skip0xff = 0;
					HAL_FLASH_Unlock();
				}
				else
					telnet_option(opt, val);
			}
			
			*telnetbuf += 3;
			*buflen -= 3;
			iac = (uint8_t)((*telnetbuf)[0]);
			opt = (uint8_t)((*telnetbuf)[1]);
			val = (uint8_t)((*telnetbuf)[2]);
		}
	}
	else
	{
		while(iac == TELNET_IAC && val == 0  && opt > 250 )
		{
			if (TELNET_WONT == opt) //只回复二进制命令
			{
				iac = (uint8_t)((*telnetbuf)[3]);
				opt = (uint8_t)((*telnetbuf)[4]);
				val = (uint8_t)((*telnetbuf)[5]);

				if ( iac == TELNET_IAC  && opt == TELNET_DONT  && val == 0 )
				{
					HAL_FLASH_Lock();
					telnet_option(TELNET_WONT, 0);//退出二进制传输模式
					telnet_option(TELNET_DONT, 0);//退出二进制传输模式
					char * msg = & current_telnet.buf[current_telnet.tail];
					sprintf(msg,"\r\nGet file,size=%d bytes\r\n",telnet_file.addr-TELNET_FILE_ADDR);
					current_telnet.tail += strlen(msg);
					*telnetbuf += 3;
					*buflen -= 3;
					*telnetstate = TELNET_NORMAL;
				}
				else
					return ;
			}
			
			*telnetbuf += 3;
			*buflen -= 3;
			iac = (uint8_t)((*telnetbuf)[0]);
			opt = (uint8_t)((*telnetbuf)[1]);
			val = (uint8_t)((*telnetbuf)[2]);
		}
	}
}



/**
	* @brief    telnet_recv_file 
	*           telnet 接收文件，存于 flash 中
	* @param    arg 任务参数
	* @return   void
*/
void telnet_recv_file(char * data , uint16_t len)
{
	uint8_t  * copyfrom = (uint8_t*)data ;//+ telnet_file.skip0xff;//0xff 0xff 被分包的情况，跳过第一个 ff
	uint8_t	 * copyend = copyfrom + len ;
	uint8_t  * copyto = (uint8_t*)(&telnet_file.buf[telnet_file.remain]);
	uint32_t * value = (uint32_t*)(&telnet_file.buf[0]);
	uint32_t   size = 0;
	
	//telnet_file.skip0xff = ((uint8_t)data[len-1] == 0xff && (uint8_t)data[len-2] != 0xff);//0xff 0xff 被分包的情况

	//如果文件中存在 0xff ，在 SecureCRT 会发两个 0xff ，需要剔除一个
	while(copyfrom < copyend)
	{
		*copyto++ = *copyfrom++ ;
		if (*copyfrom == 0xff) 
			++copyfrom;
	}

	size = copyto - (uint8_t*)(&telnet_file.buf[0]);
	telnet_file.remain = size & 0x03 ;//stm32f429 的 flash 以4的整数倍写入，不足4字节留到下一包写入 
	size >>= 2; 	                  // 除于 4
	
	for(uint32_t i = 0;i < size ; ++i)
	{
		if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,telnet_file.addr,*value))
		{
			Errors("write data error\r\n");
		}
		else
		{
			++value;
			telnet_file.addr += 4;
		}
	}
	
	if (telnet_file.remain) //此次没写完的留到下次写
		memcpy(telnet_file.buf,&telnet_file.buf[size<<2],telnet_file.remain);
}



/**
	* @brief    telnet_recv_pro 
	*           telnet 接收数据任务
	* @param    arg 任务参数
	* @return   void
*/
static void telnet_recv_pro(void const * arg)
{
	uint32_t state = TELNET_NORMAL ;
	struct netconn	* this_conn = (struct netconn *)arg; //当前 telnet 连接句柄	
	struct netbuf	* recvbuf;
	struct shell_buf  telnet_shell;//新建 shell 交互 
	
	char shell_bufmem[COMMANDLINE_MAX_LEN] = {0};

	telnet_shell.bufmem = shell_bufmem;
	telnet_shell.index = 0;
	telnet_shell.puts = telnet_puts;//定义 telnet 的 shell 输入和输出
	
	telnet_option(TELNET_DO,1);   //客户端开启回显
	//telnet_option(TELNET_DO,34);//客户端关闭行模式
		
	while(ERR_OK == netconn_recv(this_conn, &recvbuf))	//阻塞直到收到数据
	{
		current_telnet.conn = this_conn;
		
		do
		{
			char *	  recvdata;
			uint16_t  datalen;
			
			netbuf_data(recvbuf, (void**)&recvdata, &datalen); //提取数据指针
			
			telnet_check_option(&recvdata,&datalen,&state);

			if (datalen)
			{
				if (TELNET_NORMAL == state)
					shell_input(&telnet_shell,recvdata,datalen);//把数据与 shell 交互
				else
					telnet_recv_file(recvdata,datalen);
			}
		}
		while(netbuf_next(recvbuf) > 0);
		
		netbuf_delete(recvbuf);
		
		current_telnet.conn = NULL;
		
		if ( current_telnet.tail ) // 如果 telnet 缓冲区有数据，发送
		{	
			netconn_write(this_conn,current_telnet.buf,current_telnet.tail, NETCONN_COPY);
			current_telnet.tail = 0;
		}
		
		if ((!bind_telnet.conn) && (default_puts == telnet_puts)) //输入了 debug-info 获取信息流后
		{
			bind_telnet.conn = this_conn;//绑定 telnet 数据流向
			bind_telnet.tail = 0;
		}
	}

	if (this_conn == bind_telnet.conn)// 关闭 telnet 时如果有数据流绑定，解绑
	{
		bind_telnet.conn = NULL;
		default_puts = NULL;
	}
	
	netconn_close(this_conn); //关闭链接
	netconn_delete(this_conn);//释放连接的内存
	
	vTaskDelete(NULL);//连接断开时删除自己
}




/**
	* @brief    telnet_server_listen 
	*           telnet 服务器监听任务
	* @param    void
	* @return   void
*/
static void telnet_server_listen(void const * arg)
{
	struct netconn *conn, *newconn;
	err_t err;

	conn = netconn_new(NETCONN_TCP); //创建一个 TCP 链接
	netconn_bind(conn,IP_ADDR_ANY,23); //绑定端口 23 号端口
	netconn_listen(conn); //进入监听模式

	for(;;)
	{
		err = netconn_accept(conn,&newconn); //接收连接请求
		
		if (err == ERR_OK) //新连接成功时，开辟新一个线程处理 telnet
		{
		  osThreadDef(telnet, telnet_recv_pro, osPriorityNormal, 0, 512);
		  osThreadCreate(osThread(telnet), newconn);
		}
	}
}



/**
	* @brief    telnet_idle_pro 
	*           telnet 空闲处理函数
	* @param    void
	* @return   void
*/
void telnet_idle_pro(void)
{
	if (bind_telnet.conn && bind_telnet.tail)
	{
		netconn_write(bind_telnet.conn,bind_telnet.buf,bind_telnet.tail, NETCONN_COPY);
		bind_telnet.tail = 0;
	}
}



/**
	* @brief    telnet_erase_file 
	*           telnet 清空接收到的文件
	* @param    arg 任务参数
	* @return   void
*/
void telnet_erase_file(void * arg)
{
	uint32_t SectorError;
    FLASH_EraseInitTypeDef FlashEraseInit;
	
	FlashEraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS; //擦除类型，扇区擦除 
	FlashEraseInit.Sector       = 7;                       //0x8060000 在 F429 扇区7，擦除
	FlashEraseInit.NbSectors    = 1;                       //一次只擦除一个扇区
	FlashEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;   //电压范围，VCC=2.7~3.6V之间!!
	
	HAL_FLASH_Unlock();
	HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError);
	HAL_FLASH_Lock();
	printk("done\r\n");
}



/**
	* @brief    telnet_server_init 
	*           telnet 服务器端初始化
	* @param    void
	* @return   void
*/
void telnet_server_init(void)
{
	current_telnet.tail = 0;
	bind_telnet.tail = 0;
	
	shell_register_command("telnet-erase",telnet_erase_file);	
	
	osThreadDef(TelnetServer, telnet_server_listen, osPriorityNormal, 0, 128);
	osThreadCreate(osThread(TelnetServer), NULL);//初始化完 lwip 后创建tcp服务器监听任务
}



