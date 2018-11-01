/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#include "fatfs.h"

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
FIL SDFile;       /* File object for SD */

/* USER CODE BEGIN Variables */

#include "shell.h"
#include <string.h>

//static uint8_t buffer[_MAX_SS]; /* a work buffer for the f_mkfs() */

char g_acCurrentPath[128] = {0};
DIR  g_stCurrentDir = {0};
uint8_t g_cPathTail = 0;

const char * const MonthList[] =
{"Jan","Feb","Mar","Apr","May","June","July","Aug","Sept","Oct","Nov","Dec"};




void vFatfs_CD(void * arg)
{
    DIR dir;
	char acAbsolutePath[128];
	char * path = (char*)arg;
	
	while(* path == ' ') ++path; //跳过空格

	if (*path == 0) //空路径
	{
		if (g_cPathTail > 1) //如果当前目录不是根目录
		{
			g_acCurrentPath[0] = '/';//清空当前路径
			g_acCurrentPath[1] = 0;//清空当前路径
			g_cPathTail = 1;
			sprintf(shell_input_sign,DEFAULT_INPUTSIGN); //设置输入标志
			printk("%s",shell_input_sign);
		}
		return ;
	}

	if (*path != '/')  // 是不是相对路径
	{
		sprintf(acAbsolutePath,"%s/%s",g_acCurrentPath,path);//合成绝对路径
	}
	else //绝对路径
	{
		int iPathLen = strlen(path);
		memcpy(acAbsolutePath,path,iPathLen);
		acAbsolutePath[iPathLen] = 0;
	}
	
    if (FR_OK == f_opendir(&dir, acAbsolutePath)) //路径存在，打开成功
    {
		f_closedir(&dir); //把路径关闭
		
		g_cPathTail = strlen(acAbsolutePath);
		memcpy(g_acCurrentPath,acAbsolutePath,g_cPathTail);//更新当前路径
		
		sprintf(shell_input_sign,"%s # ",g_acCurrentPath);//更新当前输入标志
		printk("%s",shell_input_sign);
    }
	else
	{
		Errors("\tillegal path!\r\n");
	}
}




/* Start node to be scanned (***also used as work area***) */
void  vFatfs_ScanDir ( void * arg )
{
    FRESULT res;
    DIR dir;
	FILINFO fno;

	f_opendir(&dir, g_acCurrentPath);

    for (;;) {
        res = f_readdir(&dir, &fno); /* Read a directory item */
		
		if (res != FR_OK || fno.fname[0] == 0)
			break;  /* Break on error or end of dir */

		struct FileDate * pStDate = (struct FileDate *)(&fno.fdate);
		struct FileTime * pStTime = (struct FileTime *)(&fno.ftime);
		
		#if 0
        if (fno.fattrib & AM_DIR) {  /* It is a directory */
            i = strlen(path);
            sprintf(&path[i], "/%s", fno.fname);
            res = scan_files(path);                    /* Enter the directory */
            if (res != FR_OK) break;
            path[i] = 0;
        } 
		else                                        /* It is a file. */
        #endif
		if (fno.fattrib & AM_DIR) /* It is a directory */
			color_printk(light_purple,"%s-%s-%d\r\n", fno.fname,MonthList[pStDate->Month],pStTime->Min);
		else
			color_printk(green,"%s-%s-%d\r\n", fno.fname,MonthList[pStDate->Month],pStTime->Min);
    }
	
	f_closedir(&dir); //把路径关闭

}





/* Start node to be scanned (***also used as work area***) */
FRESULT scan_files ( char* path )
{
    FRESULT res;
    DIR dir;
	FILINFO fno;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno); /* Read a directory item */
            
			if (res != FR_OK || fno.fname[0] == 0)
				break;  /* Break on error or end of dir */
			#if 0
            if (fno.fattrib & AM_DIR) {  /* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                res = scan_files(path);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } 
			else                                        /* It is a file. */
            #endif
			if (fno.fattrib & AM_DIR) /* It is a directory */
				color_printk(light_purple,"%s\r\n", fno.fname);
			else
				color_printk(green,"%s\r\n", fno.fname);
            
        }
        f_closedir(&dir);
    }

    return res;
}



void vShell_FatfsLS(void * arg)
{
	char ScanBuf[128] = {0};
//	memset(ScanBuf,0,128);
	scan_files(ScanBuf);
}



void vFatfs_TouchFile(void * arg)
{
	
	FIL File; 		 /* File object */
	FRESULT res;
	uint32_t byteswritten;
	
	res = f_open(&File, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK) 
	{
		return;
	}
	
//	res = f_write(&File, stm32_text, 512, &byteswritten);
	if((byteswritten == 0) || (res != FR_OK))
	{
		Errors("write file error\r\n");
	}	
	f_close(&File);
}



/* USER CODE END Variables */    

void MX_FATFS_Init(void) 
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */   
	if (FR_OK != f_mount(&SDFatFS, (TCHAR const*)SDPath, 0))
	{
		Errors("Fatfs Mount failed!\r\n");
	}
	else
	{
		printk("fatfs Mount success\r\n");
		if (f_opendir(&g_stCurrentDir, g_acCurrentPath) == FR_OK)
		{
			vShell_RegisterCommand("ls",vFatfs_ScanDir);
			vShell_RegisterCommand("cd",vFatfs_CD);
			//vShell_RegisterCommand("touch",vFatfs_TouchFile);
			f_closedir(&g_stCurrentDir);
		}
	}  
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC 
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  return 0;
  /* USER CODE END get_fattime */  
}

/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
