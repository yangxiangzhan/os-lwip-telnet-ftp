#ifndef __CONSOLE_HAL_H__
#define __CONSOLE_HAL_H__
#ifdef __cplusplus
 extern "C" {
#endif


void serial_puts(char * buf,uint16_t len);

int  serial_rxpkt_queue_out(char ** data,uint16_t * len);

void serial_rxpkt_max_len(uint16_t MaxLen);

int  serial_busy(void);

void hal_serial_init(void);

void hal_serial_deinit(void);

	 
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
