#ifndef INC_DS18B20_H_
#define INC_DS18B20_H_

#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "tim.h"

#define DS18B20_Port DS18B20_DQ_GPIO_Port
#define DS18B20_Pin DS18B20_DQ_Pin

/*********ROM COMMANDS ***********/
#define SEARCH_ROM		0xF0
#define READ_ROM		0x33
#define MATCH_ROM		0x55
#define SKIP_ROM		0xCC
#define ALARM_SEARCH	0xEC

/*********FUNCTION COMMANDS*******/
#define CONVERT_T		0x44
#define WRITE_SCRATCHPAD 0x4E
#define READ_SCRATCHPAD	0xBE
#define COPY_SCRATCHPAD	0x48
#define RECALL_E2		0xB8
#define READ_POWER		0xB4

/*********FUNCTIONS**************/

int DS18B20_Initialize();			//returns 0 if successful, 1 if not
float DS18B20_Read_Temperature();	//return temperature if successful, 0xffff if not
float DS18B20_Read_Temperature_NB();


void DS18B20_Write_Byte(uint8_t byte);
uint8_t DS18B20_Read_Byte();

#endif /* INC_DS18B20_H_ */
