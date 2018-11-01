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
	* @param    pcBuf   转字符串所在内存
	* @param    iValue  值
	* @return   转换所得字符串长度
*/	
int i_itoa(char * pcBuf,int iValue)		
{
	int iLen = 0;
	int iVal = (iValue<0)?(0-iValue) : iValue; 
	
	do
	{
		pcBuf[iLen++] = (char)(iVal % 10 + '0'); 
		iVal = iVal/10;
	}
	while(iVal);
	
	if (iValue < 0) 
		pcBuf[iLen++] = '-'; 
	
	for (uint8_t index = 1 ; index <= iLen/2; ++index)
	{
		char reverse = pcBuf[iLen  - index];  
		pcBuf[iLen - index] = pcBuf[index -1];   
		pcBuf[index - 1] = reverse; 
	}
	
	return iLen;
}




/**
	* @author   古么宁
	* @brief    i_ftoa 
	*           浮点型转字符串，保留4位小数
	* @param    pcBuf   转字符串所在内存
	* @param    fValue  值
	* @return   字符串长度
*/
int i_ftoa(char * pcBuf,float fValue)		
{
	int iLen = 0;
	float fVal = (fValue < 0.0f )? (0.0f - fValue) : fValue; 
	int iIntVal   = (int)fVal;  
	int iFloatVal =  (int)(fVal * 10000) - iIntVal * 10000;

	for(uint32_t cnt = 0 ; cnt < 4 ; ++ cnt)
	{		
		pcBuf[iLen++] = (char)(iFloatVal % 10 + '0');
		iFloatVal = iFloatVal / 10;
	}
	
	pcBuf[iLen++] = '.';  

	do
	{
		pcBuf[iLen++] = (char)(iIntVal % 10 + '0'); 
		iIntVal = iIntVal/10;
	}
	while(iIntVal);            
	
	
	if (fValue < 0.0f) 
		pcBuf[iLen++] = '-'; 
	
	for (uint8_t index = 1 ; index <= iLen/2; ++index)
	{
		char reverse = pcBuf[iLen  - index];  
		pcBuf[iLen - index] = pcBuf[index -1];   
		pcBuf[index - 1] = reverse; 
	}
	
	return iLen;
}


/**
	* @author   古么宁
	* @brief    i_itoa 
	*           整型转十六进制字符串
	* @param    pcBuf   转字符串所在内存
	* @param    iValue  值
	* @return   转换所得字符串长度
*/	
int i_xtoa(char * strbuf,uint32_t iValue)		
{
	int iLen = 0;
	
	do{
		char cChar = (char)((iValue & 0x0f) + '0');
		strbuf[iLen++] = (cChar > '9') ? (cChar + 7) : (cChar);
		iValue >>= 4;
	}
	while(iValue);
	
	for (uint8_t index = 1 ; index <= iLen/2; ++index)
	{
		char reverse = strbuf[iLen  - index];  
		strbuf[iLen - index] = strbuf[index -1];   
		strbuf[index - 1] = reverse; 
	}
	
	return iLen;
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
	char * pcInput = fmt;
	char * pcOutput = fmt;

	if (NULL == current_puts) return ;
	
	va_list ap; 
	va_start(ap, fmt);

	while (*pcOutput) //需要防止发送缓存溢出
	{
		if ('%' == *pcOutput) //遇到格式化标志,为了效率仅支持 %d ,%f ,%s %x ,%c 
		{
			char  buf[64] = { 0 };//把数字转为字符串的缓冲区
			char *pStrbuf = buf;  //把数字转为字符串的缓冲区
			int   iStrlen = 0;   //最终转换长度
			
			if (pcOutput != pcInput)//把 % 前面的部分输出
				current_puts(pcInput,pcOutput - pcInput);
	
			pcInput = pcOutput++;
			switch (*pcOutput++) // 经过两次 ++, output 已越过 %? 
			{
				case 'd':
					iStrlen = i_itoa(pStrbuf,va_arg(ap, int));
					break;

				case 'f':
					iStrlen = i_ftoa(pStrbuf,(float)va_arg(ap, double));
					break;

				case 'x':
					iStrlen = i_xtoa(pStrbuf,va_arg(ap, int));
					break;
					
				case 'c' :
					pStrbuf[iStrlen++] = (char)va_arg(ap, int);
					break;
				
				case 's':
					pStrbuf = va_arg(ap, char*);
					iStrlen = strlen(pStrbuf);
					break;

				default:continue;
			}
			
			pcInput = pcOutput;
			current_puts(pStrbuf,iStrlen);
		}
		else
		{
			++pcOutput;
		}
	}

	va_end(ap);
	
	if (pcOutput != pcInput) 
		current_puts(pcInput,pcOutput - pcInput);
}



