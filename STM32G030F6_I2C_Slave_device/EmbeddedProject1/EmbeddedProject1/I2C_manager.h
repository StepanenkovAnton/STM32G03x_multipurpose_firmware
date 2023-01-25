
#pragma once
/**
 * @file i2c_manager.h
 * @author Gabriel Ballesteros (coratron)
 * @brief 
 * @version 0.1
 * @date 2021-09-12
 * 
 * @copyright Copyright (c) 2021
 * 
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2C_MANAGER_H__
#define __I2C_MANAGER_H__
#include "stm32g0xx_hal.h"
//#include <stdlib.h>
//#include <string.h>

#define I2C_ADDRESS 0x04
#define MAX_PORTS 10
#define MAX_PINS  16

#define NIBBLE 4

#define UART_DEBUG


//I2C command :  START ADRESS ACK DATA ACK DATA ACK STOP
//DATA for pin selection: 

//Registers
/*
IOSET 0x00

PIN pinID byte 0xXY, X is the port, Y is the pin number.
X values: port A -> X = 0
Y values: 0 = pin 0
Example: 0x00 is pin A0, 0x13 is pin B3


PIN MODE byte
@see GPIO_mode

0xB7 B6 B5 B4 B3 B2 B1 B0

B3-B0 determine the mode @see GPIO_mode
B5-B4 determine the PP resistor @see GPIO_pull
B7-B6 determine the speed @see GPIO_speed

*/

#define MAX_ADC_CHANNELS 24
#define MAX_ADC_HW_INSTANCES 4

#define PIN_MODE_MASK  0x0F
#define PIN_PP_MASK    0x30
#define PIN_SPEED_MASK 0xC0





/// Enum for ports
enum pinPorts
{
	PORT_GPIOA,
	PORT_GPIOB,
	PORT_GPIOC,
	PORT_GPIOD,
	PORT_GPIOE,
	PORT_GPIOF
};

enum adcInstances
{
	ADC1_INSTANCE,
	ADC2_INSTANCE,
	ADC3_INSTANCE,
	ADC4_INSTANCE
};






GPIO_TypeDef* pinGetPort(uint8_t pinID);
uint32_t pinGetNumber(uint8_t pinID);
void pinMode(uint8_t pinID, uint8_t pinConf);

void digitalWrite(uint8_t pinID, uint8_t value);
uint8_t digitalRead(uint8_t pinID);


void adcConfigure1(uint8_t adc, uint8_t channel_number);
ADC_HandleTypeDef * adcCheckValidity(uint8_t adc);
void Remap_ADC_Ranks(uint8_t adc, uint8_t scan_number);





#endif /* __I2C_MANAGER_H__ */