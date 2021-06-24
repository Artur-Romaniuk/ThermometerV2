/*
 * DS18B20.c
 *
 *  Created on: Jun 22, 2021
 *      Author: artur
 */

#include "DS18B20.h"

//static variables////////////////////
static int DS18B20_state = 0;
static uint16_t temperature = 0;

//static functions declarations//////////////
static int DS18B20_Initialize();
static void micro_delay(uint16_t delay);
static void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
static void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
static void Set_Pin_Output_Default();
static void Set_Pin_Input_Default();
static void Write_Byte(uint8_t byte);
static uint8_t Read_Byte();

//State machine for NB version
static int Read_Temperature_State_Machine_Controller();
static int Read_Temperature_Stage_0();
static void Read_Temperature_Stage_1();
static int Read_Temperature_Stage_2();
static void Read_Temperature_Stage_3();

//functions/////////////////////////////

float DS18B20_Read_Temperature()
{
	if (DS18B20_Initialize())
	{
		return 0xffff; //error, no response from Thermometer
	}
	else
	{
		HAL_Delay(1);				   //BLOCKING!
		DS18B20_Write_Byte(SKIP_ROM);  //Single device connected
		DS18B20_Write_Byte(CONVERT_T); //Start temperature conversion

		HAL_Delay(800); //BLOCKING! Gives time for temp conversion

		if (DS18B20_Initialize())
		{
			return 0xffff; //error, device didn't respond
		}
		else
		{
			HAL_Delay(1);						 //BLOCKING! Waits for voltage to normalize
			DS18B20_Write_Byte(SKIP_ROM);		 //Single device connected
			DS18B20_Write_Byte(READ_SCRATCHPAD); //Get temperature from the device
			uint8_t tmp1 = DS18B20_Read_Byte();
			uint8_t tmp2 = DS18B20_Read_Byte();
			uint16_t temperature = tmp1 | (tmp2 << 8);
			return (float)temperature / 16; //return float
		}
	}
}

float DS18B20_Read_Temperature_NB()
{
	if (Read_Temperature_State_Machine_Controller())
	{
		return 0xffff; //ERROR
	}
	return (float)temperature / 16;
}

//static functions///////////////////////////

static int DS18B20_Initialize()
{
	uint8_t response;
	Set_Pin_Output_Default();
	HAL_GPIO_WritePin(DS18B20_Port, DS18B20_Pin, GPIO_PIN_RESET);
	micro_delay(480); //datasheet
	Set_Pin_Input_Default();
	micro_delay(100); //datasheet
	if (!HAL_GPIO_ReadPin(DS18B20_Port, DS18B20_Pin))
	{
		response = 0; //correct
	}
	else
	{
		response = 1; //empty line, error
	}
	micro_delay(380);
	return response;
}

static void micro_delay(uint16_t delay)
{
	__HAL_TIM_SET_COUNTER(MICRO_DELAY_TIM_HANDLE, 0);
	while ((__HAL_TIM_GET_COUNTER(MICRO_DELAY_TIM_HANDLE)) < delay)
		;
}

static void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

static void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

static void Set_Pin_Output_Default()
{
	Set_Pin_Output(DS18B20_Port, DS18B20_Pin);
}

static void Set_Pin_Input_Default()
{
	Set_Pin_Input(DS18B20_Port, DS18B20_Pin);
}

static void Write_Byte(uint8_t byte)
{
	for (int i = 0; i < 8; i++)
	{
		if ((byte & (1 << i)) != 0)
		{
			Set_Pin_Output_Default();
			HAL_GPIO_WritePin(DS18B20_Port, DS18B20_Pin, GPIO_PIN_RESET);
			micro_delay(1);
			Set_Pin_Input_Default();
			micro_delay(60);
		}
		else
		{
			Set_Pin_Output_Default();
			HAL_GPIO_WritePin(DS18B20_Port, DS18B20_Pin, GPIO_PIN_RESET);
			micro_delay(60);
			Set_Pin_Input_Default();
		}
	}
}

static uint8_t Read_Byte()
{
	uint8_t byte = 0;
	Set_Pin_Input_Default();
	for (int i = 0; i < 8; i++)
	{
		Set_Pin_Output_Default();
		HAL_GPIO_WritePin(DS18B20_Port, DS18B20_Pin, GPIO_PIN_RESET);
		micro_delay(2);
		Set_Pin_Input_Default();
		micro_delay(10);
		if (HAL_GPIO_ReadPin(DS18B20_Port, DS18B20_Pin))
		{
			byte |= 1 << i;
		}
		micro_delay(50);
	}
	return byte;
}

//state machine static funtions///////
static int Read_Temperature_State_Machine_Controller()
{
	switch (DS18B20_state)
	{
	case 0:
		if (Read_Temperature_Stage_0())
		{
			return 1; //ERROR
		}
		__HAL_TIM_SetCounter(STATE_MACHINE_TIM_HANDLE, 0);
		DS18B20_state++;
		break;
	case 1:
		if (__HAL_TIM_GetCounter(STATE_MACHINE_TIM_HANDLE) < 1000)
		{
			break;
		}
		Read_Temperature_Stage_1();
		__HAL_TIM_SetCounter(STATE_MACHINE_TIM_HANDLE, 0);
		DS18B20_state++;
		break;
	case 2:
		if (__HAL_TIM_GetCounter(STATE_MACHINE_TIM_HANDLE) < 8000)
		{
			break;
		}
		if (Read_Temperature_Stage_2())
		{
			return 1; //ERROR
		}
		__HAL_TIM_SetCounter(STATE_MACHINE_TIM_HANDLE, 0);
		DS18B20_state++;
		break;
	case 3:
		if (__HAL_TIM_GetCounter(STATE_MACHINE_TIM_HANDLE) < 1000)
		{
			break;
		}
		Read_Temperature_Stage_3();
		DS18B20_state = 0;
		break;
	}
	return 0;
}
static int Read_Temperature_Stage_0()
{
	if (DS18B20_Initialize())
	{
		return 1; //error, no response from Thermometer
	}
	return 0;
}
static void Read_Temperature_Stage_1()
{
	DS18B20_Write_Byte(SKIP_ROM);  //Single device connected
	DS18B20_Write_Byte(CONVERT_T); //Start temperature conversion
}
static int Read_Temperature_Stage_2()
{
	if (DS18B20_Initialize())
	{
		return 1; //error, device didn't respond
	}
	return 0;
}
static void Read_Temperature_Stage_3()
{
	uint8_t tmp1;
	uint8_t tmp2;
	DS18B20_Write_Byte(SKIP_ROM);		 //Single device connected
	DS18B20_Write_Byte(READ_SCRATCHPAD); //Get temperature from the device
	tmp1 = DS18B20_Read_Byte();
	tmp2 = DS18B20_Read_Byte();
	temperature = (tmp1 | (tmp2 << 8));
}
