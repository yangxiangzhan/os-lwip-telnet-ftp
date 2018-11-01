#ifndef __CONSOLE_HAL_H__
#define __CONSOLE_HAL_H__
#ifdef __cplusplus
 extern "C" {
#endif


#define UPDATE_PACKAGE_AT_W25X16_ADRR 0x10000

#define IAP_RX_BUF_SIZE 256  //0x100//  W25X16 一页大小


	 
#define IAP_ADDR 0x08000000
#define APP_ADDR 0x08020000

//在 iap 代码区可更新跳转 app
//在 app 代码区可更新跳转 iap
#define UPDATE_ADDR  IAP_ADDR 

	 
// stm32f103vet6 ram起始地址为 0x20000000 ，大小 0x10000
// 直接用 ram 区高字段的地址作为进 iap 模式的标志位
// 在 app 中，先对标志位进行置位提示，然后跳转到 iap 
#define IAP_FLAG_ADDR       0x2000FF00 

#define IAP_UPDATE_CMD_FLAG 0x1234ABCD

#define IAP_SUCCESS_FLAG      0x54329876

#define iIAP_GetUpdateCmd() (*(__IO uint32_t *) IAP_FLAG_ADDR == IAP_UPDATE_CMD_FLAG)

#define vIAP_SetUpdateCmd() (*(__IO uint32_t *) IAP_FLAG_ADDR = IAP_UPDATE_CMD_FLAG)

#define iIAP_GetSuccessCmd() (*(__IO uint32_t *) IAP_FLAG_ADDR == IAP_SUCCESS_FLAG)

#define vIAP_SetSuccessCmd() (*(__IO uint32_t *) IAP_FLAG_ADDR = IAP_SUCCESS_FLAG)

#define vIAP_ClearFlag()       (*(__IO uint32_t *) IAP_FLAG_ADDR = 0)




void vUsartHal_Output(char * buf,uint16_t len);

int  iUsartHal_RxPktOut(char ** data,uint16_t * len);

void vUsartHal_RxPktMaxLen(uint16_t MaxLen);

int  iUsartHal_TxBusy(void);

void vUsartHal_Init(void);
//------------------------------串口 IAP 相关------------------------------
int  iUsartHal_IAP_Erase(uint32_t SECTOR);	 
	 
void vUsartHal_IAP_Write(uint32_t FlashAddr,uint32_t FlashData);

void vUsartHal_UnlockFlash(void);

void vUsartHal_LockFlash(void);
//------------------------------控制台命令------------------------------
void vShell_JumpCmd(void * arg);
void vShell_RebootSystem(void * arg);

	 
#ifdef __cplusplus
}
#endif
#endif /* __CONSOLE_HAL_H__ */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
