/**
  ******************************************************************************
  * @file           iap_hal.c
  * @author         古么宁
  * @brief          serial_console file
                    在线升级功能硬件抽象部分
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/


#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "iap_hal.h"


//------------------------------串口 IAP 相关------------------------------


/**
	* @brief    iap_erase_flash console 擦除 flash 某个扇区
	* @param    空
	* @return   空
*/
int iap_erase_flash(uint32_t SECTOR)
{
	uint32_t SectorError;
    FLASH_EraseInitTypeDef FlashEraseInit;
	HAL_StatusTypeDef HAL_Status;
	
	FlashEraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;       //擦除类型，扇区擦除 
	FlashEraseInit.Sector       = SECTOR;                        //扇区
	FlashEraseInit.NbSectors    = 1;                             //一次只擦除一个扇区
	FlashEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;      //电压范围，VCC=2.7~3.6V之间!!
	
	HAL_Status = HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError);
	
	return HAL_Status;
}



/**
	* @brief    vUsartHal_IAP_Write console 写 flash
	* @param    空
	* @return   空
*/
void iap_write_flash(uint32_t FlashAddr,uint32_t FlashData)
{
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,FlashAddr,FlashData);
}



/**
	* @brief    iap_lock_flash console 上锁 flash
	* @param    空
	* @return   空
*/
void iap_lock_flash(void)
{
	HAL_FLASH_Lock();
}



/**
	* @brief    iap_unlock_flash console 解锁 flash
	* @param    空
	* @return   空
*/
void iap_unlock_flash(void)
{
	HAL_FLASH_Unlock();
}



/**
	* @brief    vSystemReboot 硬件重启
	* @param    空
	* @return  
*/
void shell_reboot_command(void * arg)
{
	NVIC_SystemReset();
}



/**
	* @brief    vShell_JumpToAppCmd console 串口发送一包数据完成中断
	* @param    空
	* @return   空
*/
void shell_jump_command(void * arg)
{
	uint32_t UPDATE_ADDR = (SCB->VTOR == FLASH_BASE) ? (APP_ADDR):(IAP_ADDR);
	uint32_t SpInitVal = *(uint32_t *)(UPDATE_ADDR);    
	uint32_t JumpAddr = *(uint32_t *)(UPDATE_ADDR + 4); 
	void (*pAppFun)(void) = (void (*)(void))JumpAddr;    
	__set_BASEPRI(0); 	      
	__set_FAULTMASK(0);       
	__disable_irq();          
	__set_MSP(SpInitVal);     
	__set_PSP(SpInitVal);     
	__set_CONTROL(0);         
	(*pAppFun) ();            
}




