/**
  ******************************************************************************
  * @file           ustdio.c
  * @author         杨翔湛
  * @brief          非标准化打印输出
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "ustdio.h"



fnFmtOutDef current_puts = NULL;
fnFmtOutDef default_puts = NULL;


const char none        []= "\033[0m";  
const char black       []= "\033[0;30m";  
const char dark_gray   []= "\033[1;30m";  
const char blue        []= "\033[0;34m";  
const char light_blue  []= "\033[1;34m";  
const char green       []= "\033[0;32m";  
const char light_green []= "\033[1;32m";  
const char cyan        []= "\033[0;36m";  
const char light_cyan  []= "\033[1;36m";  
const char red         []= "\033[0;31m";  
const char light_red   []= "\033[1;31m";  
const char purple      []= "\033[0;35m";  
const char light_purple[]= "\033[1;35m";  
const char brown       []= "\033[0;33m";  
const char yellow      []= "\033[1;33m";  
const char light_gray  []= "\033[0;37m";  
const char white       []= "\033[1;37m"; 

char * default_color = (char *)none;



/**
	* @author   古么宁
	* @brief    重定义 printf 函数。本身来说 printf 方法是比较慢的，
	*           因为 printf 要做更多的格式判断，输出的格式更多一些。
	*           所以为了效率，在后面写了 printk 函数。
	* @return   NULL
*/
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
}

//重定义fputc函数 
int fputc(int ch, FILE *f)
{
	char  cChar = (char)ch;
	if (current_puts)
		current_puts(&cChar,1);
	return ch;
}
#endif 




/**
	* @author   古么宁
	* @brief    i_itoa 
	*           整型转十进制字符串
	* @param    strbuf   转字符串所在内存
	* @param    value  值
	* @return   转换所得字符串长度
*/	
int i_itoa(char * strbuf,int value)		
{
	int len = 0;
	int value_fix = (value<0)?(0-value) : value; 
	
	do
	{
		strbuf[len++] = (char)(value_fix % 10 + '0'); 
		value_fix = value_fix/10;
	}
	while(value_fix);
	
	if (value < 0) 
		strbuf[len++] = '-'; 
	
	for (uint8_t index = 1 ; index <= len/2; ++index)
	{
		char reverse = strbuf[len  - index];  
		strbuf[len - index] = strbuf[index -1];   
		strbuf[index - 1] = reverse; 
	}
	
	return len;
}




/**
	* @author   古么宁
	* @brief    i_ftoa 
	*           浮点型转字符串，保留4位小数
	* @param    strbuf   转字符串所在内存
	* @param    value  值
	* @return   字符串长度
*/
int i_ftoa(char * strbuf,float value)		
{
	int len = 0;
	float value_fix = (value < 0.0f )? (0.0f - value) : value; 
	int int_part   = (int)value_fix;  
	int float_part =  (int)(value_fix * 10000) - int_part * 10000;

	for(uint32_t i = 0 ; i < 4 ; ++i)
	{		
		strbuf[len++] = (char)(float_part % 10 + '0');
		float_part = float_part / 10;
	}
	
	strbuf[len++] = '.';  

	do
	{
		strbuf[len++] = (char)(int_part % 10 + '0'); 
		int_part = int_part/10;
	}
	while(int_part);            
	
	
	if (value < 0.0f) 
		strbuf[len++] = '-'; 
	
	for (uint8_t index = 1 ; index <= len/2; ++index)
	{
		char reverse = strbuf[len  - index];  
		strbuf[len - index] = strbuf[index -1];   
		strbuf[index - 1] = reverse; 
	}
	
	return len;
}


/**
	* @author   古么宁
	* @brief    i_itoa 
	*           整型转十六进制字符串
	* @param    strbuf   转字符串所在内存
	* @param    value  值
	* @return   转换所得字符串长度
*/	
int i_xtoa(char * strbuf,uint32_t value)		
{
	int len = 0;
	
	do{
		char ascii = (char)((value & 0x0f) + '0');
		strbuf[len++] = (ascii > '9') ? (ascii + 7) : (ascii);
		value >>= 4;
	}
	while(value);
	
	for (uint8_t index = 1 ; index <= len/2; ++index)
	{
		char reverse = strbuf[len  - index];  
		strbuf[len - index] = strbuf[index -1];   
		strbuf[index - 1] = reverse; 
	}
	
	return len;
}



/**
	* @author   古么宁
	* @brief    printk 
	*           格式化输出，类似于 sprintf 和 printf ,可重入
	*           用标准库的 sprintf 和 printf 的方法太慢了，所以自己写了一个，重点要快
	* @param    fmt     要格式化的信息字符串指针
	* @param    ...     不定参
	* @return   void
*/
void printk(char* fmt, ...)
{
	char * buf_head = fmt;
	char * buf_tail = fmt;

	if (NULL == current_puts) 
		return ;
	
	va_list ap; 
	va_start(ap, fmt);

	while (*buf_tail) //需要防止发送缓存溢出
	{
		if ('%' == *buf_tail) //遇到格式化标志,为了效率仅支持 %d ,%f ,%s %x ,%c 
		{
			char  buf_malloc[64] = { 0 };//把数字转为字符串的缓冲区
			char *buf = buf_malloc;  //把数字转为字符串的缓冲区
			int   len = 0;   //最终转换长度
			
			if (buf_tail != buf_head)//把 % 前面的部分输出
				current_puts(buf_head,buf_tail - buf_head);
	
			buf_head = buf_tail++;
			switch (*buf_tail++) // 经过两次 ++, buf_tail 已越过 %? 
			{
				case 'd':
					len = i_itoa(buf,va_arg(ap, int));
					break;

				case 'f':
					len = i_ftoa(buf,(float)va_arg(ap, double));
					break;

				case 'x':
					len = i_xtoa(buf,va_arg(ap, int));
					break;
					
				case 'c' :
					buf[len++] = (char)va_arg(ap, int);
					break;
				
				case 's':
					buf = va_arg(ap, char*);
					len = strlen(buf);
					break;

				default:continue;
			}
			
			buf_head = buf_tail;
			current_puts(buf,len);
		}
		else
		{
			++buf_tail;
		}
	}

	va_end(ap);
	
	if (buf_tail != buf_head) 
		current_puts(buf_head,buf_tail - buf_head);
}



