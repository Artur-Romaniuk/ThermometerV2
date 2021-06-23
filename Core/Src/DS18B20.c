/*
 * DS18B20.c
 *
 *  Created on: Jun 22, 2021
 *      Author: artur
 */

#include "DS18B20.h"

volatile int DS18B20_state = 0;

//static functions declarations//////////////
static void micro_delay(uint16_t delay);
static void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
static void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
static void Set_Pin_Output_Defualt();
static void Set_Pin_Input_Defualt();

//functions/////////////////////////////

int DS18B20_Initialize() {
	uint8_t response;
	Set_Pin_Output_Defualt();
	HAL_GPIO_WritePin(DS18B20_Port, DS18B20_Pin, GPIO_PIN_RESET);
	micro_delay(480); //datasheet
	Set_Pin_Input_Defualt();
	micro_delay(100); //datasheet
	if (!HAL_GPIO_ReadPin(DS18B20_Port, DS18B20_Pin)) {
		response = 0; //correct
	} else {
		response = 1; //empty line, error
	}
	micro_delay(380);
	return response;
}

float DS18B20_Read_Temperature() {
	if (DS18B20_Initialize()) {
		return 0xffff; //error, no response from Thermometer
	} else {
		HAL_Delay(1);				//BLOCKING!
		DS18B20_Write_Byte(SKIP_ROM);	//Single device connected
		DS18B20_Write_Byte(CONVERT_T);	//Start temperature conversion

		HAL_Delay(800);				//BLOCKING! Gives time for temp conversion

		if (DS18B20_Initialize()) {
			return 0xffff; //error, device didn't respond
		} else {
			HAL_Delay(1);			//BLOCKING! Waits for voltage to normalize
			DS18B20_Write_Byte(SKIP_ROM); //Single device connected
			DS18B20_Write_Byte(READ_SCRATCHPAD); //Get temperature from the device
			uint8_t tmp1 = DS18B20_Read_Byte();
			uint8_t tmp2 = DS18B20_Read_Byte();
			uint16_t temperature = tmp1 | (tmp2 << 8);
			return (float) temperature / 16;	//return float
		}

	}
}

float DS18B20_Read_Temperature_NB() {
	uint8_t tmp1 ;
	uint8_t tmp2 ;
	uint16_t temperature;


	switch (DS18B20_state) {
	case 0:
		if (DS18B20_Initialize()) {
			return 0xffff; //error, no response from Thermometer
		} else {
			__HAL_TIM_SET_COUNTER(&htim7, 0xffff - 1000);
		}
		break;
	case 1:
		DS18B20_Write_Byte(SKIP_ROM);	//Single device connected
		DS18B20_Write_Byte(CONVERT_T);	//Start temperature conversion
		__HAL_TIM_SET_COUNTER(&htim7, 0xffff - 8000);
		break;
	case 2:
		if (DS18B20_Initialize()) {
			return 0xffff; //error, device didn't respond
		} else {
			__HAL_TIM_SET_COUNTER(&htim7, 0xffff - 1000);
		}
		break;
	case 3:
		DS18B20_Write_Byte(SKIP_ROM); //Single device connected
		DS18B20_Write_Byte(READ_SCRATCHPAD); //Get temperature from the device
		tmp1 = DS18B20_Read_Byte();
		tmp2 = DS18B20_Read_Byte();
		temperature = tmp1 | (tmp2 << 8);
		return (float) temperature / 16;	//return float
	default:
		DS18B20_state=0;
		break;
	}

}

void DS18B20_Write_Byte(uint8_t byte) {
	for (int i = 0; i < 8; i++) {
		if ((byte & (1 << i)) != 0) {
			Set_Pin_Output_Defualt();
			HAL_GPIO_WritePin(DS18B20_Port, DS18B20_Pin, GPIO_PIN_RESET);
			micro_delay(1);
			Set_Pin_Input_Defualt();
			micro_delay(60);
		} else {
			Set_Pin_Output_Defualt();
			HAL_GPIO_WritePin(DS18B20_Port, DS18B20_Pin, GPIO_PIN_RESET);
			micro_delay(60);
			Set_Pin_Input_Defualt();
		}
	}
}

uint8_t DS18B20_Read_Byte() {
	uint8_t byte = 0;
	Set_Pin_Input_Defualt();
	for (int i = 0; i < 8; i++) {
		Set_Pin_Output_Defualt();
		HAL_GPIO_WritePin(DS18B20_Port, DS18B20_Pin, GPIO_PIN_RESET);
		micro_delay(2);
		Set_Pin_Input_Defualt();
		micro_delay(10);
		if (HAL_GPIO_ReadPin(DS18B20_Port, DS18B20_Pin)) {
			byte |= 1 << i;
		}
		micro_delay(50);
	}
	return byte;
}

//static functions///////////////////////////

static void micro_delay(uint16_t delay) {
	__HAL_TIM_SET_COUNTER(&htim6, 0);
	while ((__HAL_TIM_GET_COUNTER(&htim6)) < delay)
		;
}

static void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

static void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

static void Set_Pin_Output_Defualt() {
	Set_Pin_Output(DS18B20_Port, DS18B20_Pin);
}

static void Set_Pin_Input_Defualt() {
	Set_Pin_Input(DS18B20_Port, DS18B20_Pin);
}
