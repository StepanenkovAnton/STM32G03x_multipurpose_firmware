/**
 * @file i2c_manager.c
 * @author Gabriel Ballesteros (coratron)
 * @brief 
 * @version 0.1
 * @date 2021-09-12
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "I2C_manager.h"
#include "libNMEA.h"

extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
//extern UART_HandleTypeDef huart2;
extern RTC_HandleTypeDef hrtc;

extern NMEA_data nmea_data;

#define BUFFER_SIZE 3

enum SlaveMode {
	WAITING_COMMAND_BYTE,
	PARSING_COMMAND_BYTE,
	WAITING_COMMAND,
	TRANSMITTING
} slaveMode = WAITING_COMMAND_BYTE;


//Local variables
uint8_t receivingBuffer[BUFFER_SIZE] = { 0 };
uint8_t commandByte[1] = { 0 };
uint8_t payloadBytes[20] = { 0 };
uint8_t receivingBufferDataSize = 0;

uint8_t transmittingBuffer[BUFFER_SIZE] = { 0xFA, 0x01, 0xC1 };
uint8_t transmittingBufferDataSize = 0;



uint16_t commandQueue[3][3] = {};
uint8_t commandIndex = 0;
uint8_t commandLoad = 0;
uint8_t I2C_SlaveIsWaitingForData = 0;

volatile uint16_t adcDMABuffer[MAX_ADC_HW_INSTANCES][8]; //where the temporary values from DMA are stored
volatile uint16_t adcReadings[MAX_ADC_HW_INSTANCES][MAX_ADC_CHANNELS];  //where the values that are accessible over I2C are stored

volatile uint32_t adcUsedChannels[MAX_ADC_HW_INSTANCES];     //bitwise with 1 for channel used, 0 for channel not used
uint8_t adcTotalInstancesUsed;
volatile uint32_t adcTotalUsedChannels[MAX_ADC_HW_INSTANCES]; //just total numbers of channel (integer) per hardware adc
uint8_t adcUsedInstances; //bitwise 1 for used, 0 for unused
uint8_t adcRestarting = 0;

uint8_t ADC_Scan_Number = 0;

RTC_TimeTypeDef time_buffer;;
RTC_DateTypeDef date_buffer;

//uint16_t ADCDMABuffer[16] = { 0 };
//uint16_t ADCDMAReadings[16] = { 0 };

// void transmit(const uint8_t *data, uint8_t dataSize) {
//     if (dataSize > BUFFER_SIZE) {
//         dataSize = BUFFER_SIZE;
//     }

//     for (int i = 0; i < dataSize; ++i) {
//         transmittingBuffer[i] = data[i];
//     }

//     transmittingBufferDataSize = dataSize;
// }


//this callback never happens. Looks like specific chip HAL low level bug
void HAL_I2C_SlaveAddrCallback(I2C_HandleTypeDef *hi2c, uint8_t transferDirection, uint16_t addressMatchCode) 
{
   
#ifdef UART_DEBUG
	//static char msg_buf[64];
	//extern UART_HandleTypeDef huart2;
	//snprintf(msg_buf, sizeof(msg_buf), "I2C slave address detected");
	//HAL_UART_Transmit(&huart2, (uint8_t*)msg_buf, strlen(msg_buf), HAL_MAX_DELAY);
	//HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
#endif
	if (transferDirection == I2C_DIRECTION_RECEIVE) {
		transmittingBuffer[0]++;
		transmittingBuffer[1]++;
		transmittingBuffer[2]++;
		//HAL_I2C_Slave_Seq_Transmit_DMA(hi2c, transmittingBuffer, transmittingBufferDataSize, I2C_LAST_FRAME);
		if (HAL_I2C_Slave_Seq_Transmit_IT(hi2c, transmittingBuffer, BUFFER_SIZE, I2C_FIRST_AND_LAST_FRAME) != HAL_OK)
		{
			// printf("%d", receivingBuffer[0]);
			/* Transfer error in reception process */
			Error_Handler();        
		}
	} 
	else 
	{
		switch (slaveMode)
		{
		case WAITING_COMMAND_BYTE:
			if (HAL_I2C_Slave_Seq_Receive_IT(hi2c, commandByte, 1, I2C_FIRST_AND_LAST_FRAME) != HAL_OK)
			{
				// printf("%d", receivingBuffer[0]);
				/* Transfer error in reception process */
				Error_Handler();        
			}
			slaveMode = PARSING_COMMAND_BYTE;
			break;
			// case WAITING_COMMAND:
			//     if(HAL_I2C_Slave_Seq_Receive_IT(hi2c, receivingBuffer, 2, I2C_FIRST_AND_LAST_FRAME) != HAL_OK)
			//     {
			//         // printf("%d", receivingBuffer[0]);
			//         /* Transfer error in reception process */
			//         Error_Handler();        
			//     }
			//     break;
		}
      
		//HAL_I2C_Slave_Seq_Receive_DMA(hi2c, receivingBuffer + receivingBufferDataSize, 1, I2C_NEXT_FRAME);
	}
}

//this callback never happens. Looks like specific chip HAL low level bug
void HAL_I2C_SlaveListenCpltCallback(I2C_HandleTypeDef *hi2c) 
{
	//static char msg_buf[64];
	//extern UART_HandleTypeDef huart2;
	//snprintf(msg_buf, sizeof(msg_buf), "HAL_I2C_SlaveListenCpltCallback");
	//HAL_UART_Transmit(&huart2, (uint8_t*)msg_buf, strlen(msg_buf), HAL_MAX_DELAY);
	//HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
   
	receivingBufferDataSize = 0;

	for (int i = 0; i < 3; i++)
	{
		transmittingBuffer[i] = 0x01;
	}

	HAL_I2C_EnableListen_IT(hi2c);
}



void Fix_Date(RTC_DateTypeDef* date, uint8_t set_max) {
	if (date->Date == 0) {
		date->Date = 1;
		return; // no need to re-check
	}

	if (date->Date <= 28) {
		return; // always OK
	}

	// since Year is 0..99 there is no need to
	// check (Year % 100) and (Year % 400) cases.
	uint8_t is_leap = (date->Year % 4) == 0;
	uint8_t days_in_month = 31;
	// There are 30 days in April, June, September and November
	if ((date->Month == 4) || (date->Month == 6) || (date->Month == 9) || (date->Month == 11)) {
		days_in_month = 30;
	}
	else if (date->Month == 2) {
		// February
		days_in_month = is_leap ? 29 : 28;
	}

	if (date->Date > days_in_month) {
		date->Date = set_max ? days_in_month : 1;
	}
}



uint8_t RTC_Set(uint8_t year,uint8_t month,	uint8_t day,uint8_t hour,uint8_t min,uint8_t sec,uint8_t dow) {
	HAL_StatusTypeDef res;
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;

	//year = 02;
	//month = 02;
	//day = 02;
	//hour = 03;
	//min = 04;
	//sec = 11;
	//dow = 1;
	
	memset(&time, 0, sizeof(time));
	memset(&date, 0, sizeof(date));

	

	time.Hours = hour;
	time.Minutes = min;
	time.Seconds = sec;

	res = HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
	if (res != HAL_OK) {
		return HAL_ERROR;
	}
	
	
	date.WeekDay = dow;
	date.Year = year;
	date.Month = month;
	date.Date = day;

	res = HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
	if (res != HAL_OK) {
		return HAL_ERROR;
	}
	return HAL_OK;
}




//Here all the I2C requests response logic is done. See the Protocol.docx for details
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{		
	HAL_ADC_Stop_DMA(&hadc1);
	I2C_SlaveIsWaitingForData = 0;
	//char msg_buf[64];
	uint8_t transmittingBuffer[30];
	if (hi2c == &hi2c1)
	{
		switch (payloadBytes[0])
		{
		case 0x10:
			{
				pinMode(payloadBytes[1], payloadBytes[2]);
				break;
			}
		case 0x11:
			{
				digitalWrite(payloadBytes[1], payloadBytes[2]);
				break;
			}
		case 0x12:
			{
				transmittingBuffer[0] = digitalRead(payloadBytes[1]);
				transmittingBuffer[1] = 0x00;
				transmittingBuffer[2] = 0x00;
				//snprintf(msg_buf, sizeof(msg_buf), "Digital read command detected");
				//HAL_UART_Transmit(&huart2, (uint8_t*)msg_buf, strlen(msg_buf), HAL_MAX_DELAY);
				//HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
				HAL_I2C_Slave_Transmit(hi2c, transmittingBuffer, 3, 900);
				break;
			}
		case 0x13:
			{
				break;
			}
		case 0x14:
			{
				break;
			}
		case 0x00:
			{
				break;
			}
		case 0x01:
			{
				break;
			}
		case 0x02:
			{
				break;
			}
		case 0x03:
			{
				break;
			}
		case 0x04:
			{
				break;
			}
		case 0x30:
			{
				
				//if you start ADC_DMA right after ADC has finished convertion, so I2C hangs sometimes
				//I guess it is in case of too frequent HAL_ADC_Start_DMA functions calls
				//You can call HAL_ADC_Start_DMA() let say once in a second
				//HAL_ADC_Start_DMA() is placed here just as an example
				HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adcDMABuffer[0], 8);
				uint16_t ADC_buffer = 0;
				uint8_t ADC_HW_INSTANCE_buffer = 0;
				uint8_t ADC_Channel_Buf = 0;
            
				ADC_HW_INSTANCE_buffer = payloadBytes[1];
				ADC_Channel_Buf = payloadBytes[2];
				ADC_buffer = adcReadings[ADC_HW_INSTANCE_buffer][ADC_Channel_Buf];
            
				transmittingBuffer[0] = (uint8_t)(ADC_buffer >> 8);
				transmittingBuffer[1] = (uint8_t)(ADC_buffer & 0xFF);
				transmittingBuffer[2] = 0x00;

				HAL_I2C_Slave_Transmit(hi2c, transmittingBuffer, 3, 900);
				//for (uint8_t i = 0; i < 8; i++)
				//{
				//   snprintf(msg_buf, sizeof(msg_buf), "ADC #: %d - %d", i, ADCDMAReadings[i]);
				//   HAL_UART_Transmit(&huart2, (uint8_t*)msg_buf, strlen(msg_buf), HAL_MAX_DELAY);
				//   HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
				//}
            
            
				break;
			}
		case 0x31:
			{
				break;
			}
		case 0x32:
			{
				break;
			}
		case 0x33:///not finished, unused
			{
				uint16_t ADC_buffer = 0;
				uint8_t ADC_HW_INSTANCE_buffer = 0;
				uint8_t ADC_Channel_Buf = 0;
            
				ADC_HW_INSTANCE_buffer = (payloadBytes[1] & 0xC0) >> 6;
				ADC_Channel_Buf = (payloadBytes[2] & 0x3F);   
				adcConfigure1(ADC_HW_INSTANCE_buffer, ADC_Channel_Buf);
				break;
			}
		case 0x40:
			{
				HAL_RTC_GetTime(&hrtc, &time_buffer, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &date_buffer, RTC_FORMAT_BIN);
				transmittingBuffer[0] = date_buffer.Date;
				transmittingBuffer[1] = date_buffer.Month;
				transmittingBuffer[2] = date_buffer.Year;
				transmittingBuffer[3] = time_buffer.Hours;
				transmittingBuffer[4] = time_buffer.Minutes;
				transmittingBuffer[5] = time_buffer.Seconds;
				transmittingBuffer[6] = date_buffer.WeekDay;	
				HAL_I2C_Slave_Transmit(hi2c, transmittingBuffer, 7, 900);
				break;
				
			}
		case 0x41:
			{
				date_buffer.Date = payloadBytes[1];
				break;
			}
		case 0x42:
			{
				date_buffer.Month = payloadBytes[1];
				break;
			}
		case 0x43:
			{
				date_buffer.Year = payloadBytes[1];
				break;
			}
		case 0x44:
			{
				time_buffer.Hours = payloadBytes[1];
				break;
			}
		case 0x45:
			{
				time_buffer.Minutes = payloadBytes[1];
				break;
			}
		case 0x46:
			{
				time_buffer.Seconds = payloadBytes[1];
				break;
			}
		case 0x47:
			{
				date_buffer.WeekDay = payloadBytes[1];
				break;
			}
		case 0x48:
			{
				Fix_Date(&date_buffer, 1);
				//date_buffer.Year = 23;//////////////
				RTC_Set(date_buffer.Year,
					date_buffer.Month,
					date_buffer.Date,
					time_buffer.Hours,
					time_buffer.Minutes,
					time_buffer.Seconds,
					date_buffer.WeekDay);
				break;
			}
		case 0x50:
			{
				break;
			}
		case 0x51:
			{
				break;
			}
		case 0x52:
			{
				break;
			}
		case 0x53:
			{
				break;
			}
		case 0x60:
			{		
				////just for test purposes
				//nmea_data.altitude = 1234;
				//nmea_data.fix = 0;
				//nmea_data.fix_mode = 1;
				//nmea_data.geoidal_separation = 123;
				//nmea_data.HDOP = 12;
				//nmea_data.PDOP = 123;
				//nmea_data.VDOP = 123;
				//nmea_data.latitude_00000_xxxxx = 12345;
				//nmea_data.latitude_xxxxx_00000 = 123;
				//nmea_data.longitude_00000_xxxxx = 12345;
				//nmea_data.longitude_xxxxx_00000 = 123;
				//nmea_data.latitude_direction = 0x53;
				//nmea_data.longitude_direction = 0x57;
				//nmea_data.sat_in_use = 12;
				//nmea_data.sat_in_view = 22;
				////end just for test purposes
				
				//the GPS data is being received using UART DMA function
				//the GPS data from UART is being copied into buffer on the USART2_IRQHandler (fires when uart gets idle after data reception)
				//Before operating with GPS data you have to call the NMEA_process_task() function. In this example it is called every one
				//second in the Main loop.
				//And before sending to I2C call the convertNMEAtoI2C for proper GPS data formatting
				convertNMEAtoI2C(nmea_data,&transmittingBuffer);///placed right here as it takes very few time to process (the delay not even
				//visible on oscilloscope when cheching I2C communication)
				HAL_I2C_Slave_Transmit(hi2c, transmittingBuffer, 31,900);//the blocking function applied as DMA is not tested yet
				
				break;
			}
		case 0x99:
			{
				
				HAL_NVIC_SystemReset();
				break;
			}
		}
		//__HAL_I2C_GENERATE_NACK(&hi2c1);
		//HAL_I2C_Slave_Receive_IT(&hi2c1, payloadBytes, 3);
		HAL_I2C_Slave_Receive_DMA(&hi2c1, payloadBytes, 3);
		
	}
	//snprintf(msg_buf, sizeof(msg_buf), "HAL_I2C_SlaveRxCpltCallback%x,%x,%x", payloadBytes[0], payloadBytes[1], payloadBytes[2]);
	//HAL_UART_Transmit(&huart2, (uint8_t*)msg_buf, strlen(msg_buf), HAL_MAX_DELAY);
	//HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);   
	//HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adcDMABuffer[0], 8);
}

uint32_t getADCChannelFromInteger(uint8_t channelNumber)
{
	uint32_t ret = ADC_CHANNEL_1;
	
#ifdef ADC_CHANNEL_1
	if (channelNumber == 0) ret =  ADC_CHANNEL_1;
#endif 
#ifdef ADC_CHANNEL_2
	if (channelNumber == 1) ret =  ADC_CHANNEL_2;
#endif 
#ifdef ADC_CHANNEL_3
	if (channelNumber == 2) ret =  ADC_CHANNEL_3;
#endif 
#ifdef ADC_CHANNEL_4
	if (channelNumber == 3) ret =  ADC_CHANNEL_4;
#endif 
#ifdef ADC_CHANNEL_5
	if (channelNumber == 4) ret =  ADC_CHANNEL_5;
#endif 
#ifdef ADC_CHANNEL_6
	if (channelNumber == 5) ret =  ADC_CHANNEL_6;
#endif 
#ifdef ADC_CHANNEL_7
	if (channelNumber == 6) ret =  ADC_CHANNEL_7;
#endif 
#ifdef ADC_CHANNEL_8
	if (channelNumber == 7) ret =  ADC_CHANNEL_8;
#endif 
#ifdef ADC_CHANNEL_9
	if (channelNumber == 8) ret =  ADC_CHANNEL_9;
#endif 
#ifdef ADC_CHANNEL_10
	if (channelNumber == 9) ret =  ADC_CHANNEL_10;
#endif 
#ifdef ADC_CHANNEL_11
	if (channelNumber == 10) ret =  ADC_CHANNEL_11;
#endif 
#ifdef ADC_CHANNEL_12
	if (channelNumber == 11) ret = ADC_CHANNEL_12;
#endif 
#ifdef ADC_CHANNEL_13
	if (channelNumber == 12) ret =  ADC_CHANNEL_13;
#endif 
#ifdef ADC_CHANNEL_14
	if (channelNumber == 13) ret =  ADC_CHANNEL_14;
#endif 
#ifdef ADC_CHANNEL_15
	if (channelNumber == 14) ret =  ADC_CHANNEL_15;
#endif 
#ifdef ADC_CHANNEL_16
	if (channelNumber == 15) ret =  ADC_CHANNEL_16;
#endif 
#ifdef ADC_CHANNEL_17
	if (channelNumber == 16) ret =  ADC_CHANNEL_17;
#endif 
#ifdef ADC_CHANNEL_18
   
	if (channelNumber == 17) ret =  ADC_CHANNEL_18;
#endif 
#ifdef ADC_CHANNEL_19
	if (channelNumber == 18) ret =  ADC_CHANNEL_19;
#endif 
#ifdef ADC_CHANNEL_20
	if (channelNumber == 19) ret =  ADC_CHANNEL_20;
#endif 
#ifdef ADC_CHANNEL_21
	if (channelNumber == 20) ret =  ADC_CHANNEL_21;
#endif 
#ifdef ADC_CHANNEL_22
	if (channelNumber == 21) ret =  ADC_CHANNEL_22;
#endif 
#ifdef ADC_CHANNEL_23
	if (channelNumber == 22) ret =  ADC_CHANNEL_23;
#endif 
#ifdef ADC_CHANNEL_24
	if (channelNumber == 23) ret =  ADC_CHANNEL_24;
#endif 
	return ret;
}
uint32_t getADCRankFromInteger(uint8_t rankNumber)
{
	uint32_t ret = ADC_REGULAR_RANK_1;
#ifdef ADC_REGULAR_RANK_1
	if (rankNumber == 0) ret = ADC_REGULAR_RANK_1;
#endif    
#ifdef ADC_REGULAR_RANK_2
	if (rankNumber == 1) ret = ADC_REGULAR_RANK_2;
#endif 
#ifdef ADC_REGULAR_RANK_3
	if (rankNumber == 2) ret = ADC_REGULAR_RANK_3;
#endif 
#ifdef ADC_REGULAR_RANK_4
	if (rankNumber == 3) ret = ADC_REGULAR_RANK_4;
#endif 
#ifdef ADC_REGULAR_RANK_5
	if (rankNumber == 4) ret = ADC_REGULAR_RANK_5;
#endif 
#ifdef ADC_REGULAR_RANK_6
	if (rankNumber == 5) ret = ADC_REGULAR_RANK_6;
#endif 
#ifdef ADC_REGULAR_RANK_7
	if (rankNumber == 6) ret = ADC_REGULAR_RANK_7;
#endif 
#ifdef ADC_REGULAR_RANK_8
	if (rankNumber == 7) ret = ADC_REGULAR_RANK_8;
#endif 
#ifdef ADC_REGULAR_RANK_9
	if (rankNumber == 8) ret = ADC_REGULAR_RANK_9;
#endif 
#ifdef ADC_REGULAR_RANK_10
	if (rankNumber == 9) ret = ADC_REGULAR_RANK_10;
#endif 
#ifdef ADC_REGULAR_RANK_11
	if (rankNumber == 10) ret = ADC_REGULAR_RANK_11;
#endif 
#ifdef ADC_REGULAR_RANK_12
	if (rankNumber == 11) ret = ADC_REGULAR_RANK_12;
#endif 
#ifdef ADC_REGULAR_RANK_13
	if (rankNumber == 12) ret = ADC_REGULAR_RANK_13;
#endif 
#ifdef ADC_REGULAR_RANK_14
	if (rankNumber == 13) ret = ADC_REGULAR_RANK_14;
#endif 
#ifdef ADC_REGULAR_RANK_15
	if (rankNumber == 14) ret = ADC_REGULAR_RANK_15;
#endif 
#ifdef ADC_REGULAR_RANK_16
	if (rankNumber == 15) ret = ADC_REGULAR_RANK_16;
#endif 
#ifdef ADC_REGULAR_RANK_17
	if (rankNumber == 16) ret = ADC_REGULAR_RANK_17;
#endif 
#ifdef ADC_REGULAR_RANK_18
	if (rankNumber == 17) ret = ADC_REGULAR_RANK_18;
#endif 
#ifdef ADC_REGULAR_RANK_19
	if (rankNumber == 18) ret = ADC_REGULAR_RANK_19;
#endif 
#ifdef ADC_REGULAR_RANK_20
	if (rankNumber == 19) ret = ADC_REGULAR_RANK_20;
#endif 
#ifdef ADC_REGULAR_RANK_21
	if (rankNumber == 20) ret = ADC_REGULAR_RANK_21;
#endif 
#ifdef ADC_REGULAR_RANK_22
	if (rankNumber == 21) ret = ADC_REGULAR_RANK_22;
#endif 
#ifdef ADC_REGULAR_RANK_23
	if (rankNumber == 22) ret = ADC_REGULAR_RANK_23;
#endif 
#ifdef ADC_REGULAR_RANK_24
	if (rankNumber == 23) ret = ADC_REGULAR_RANK_24;
#endif 
	return ret;
}

void Remap_ADC_Ranks(uint8_t adc, uint8_t scan_number)
{
	ADC_HandleTypeDef *hadc;
	ADC_ChannelConfTypeDef sConfig = { 0 };
	uint8_t ranksCounter = 0;
   
	hadc = adcCheckValidity(adc);
   
	for (uint8_t i = 0; i < 8; i++)
	{
		sConfig.Channel = getADCChannelFromInteger(i + (scan_number * 8));
		sConfig.Rank = getADCRankFromInteger(i);
		sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;

	}
	
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
      
	}
}


//assigns ranks 1-8 to first 8 channels, scans them, then reassigns ranks 1-8 to next 8 channels and so on.
//after has passed 24 channels the HAL_ADC_DMA stops. Need to be started manually
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
   
	if (hadc->Instance == ADC1)
	{
		//HAL_ADC_Stop_DMA(&hadc1); // not necessary
		
		//for (uint8_t i = 0; i < 8; i++)
		//{
		//	adcDMABuffer[0][i] = i;
		//}
		
		
	
		for (uint8_t i = 0; i < 8; i++)
		{
			adcReadings[0][i + (ADC_Scan_Number * 8)] = adcDMABuffer[0][i];
		}
		
		ADC_Scan_Number = ADC_Scan_Number + 1;
		
		if (ADC_Scan_Number > (MAX_ADC_CHANNELS / 8))
		{
			//adcReadings[0][8] = ADC_Scan_Number;//////
			ADC_Scan_Number = 0;
		}
		else
		{
			Remap_ADC_Ranks(0, ADC_Scan_Number);
			HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adcDMABuffer[0], 8);
		}
		//HAL_Delay(100);
		     
		//HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adcDMABuffer[0], 8);
	}
   
}







uint8_t pinStatus[MAX_PORTS][MAX_PINS] = { 0 };
void pinMode(uint8_t pinID, uint8_t pinConf)
{
   

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	//load structure with requested settings
	GPIO_InitStruct.Pin = pinGetNumber(pinID);
	GPIO_InitStruct.Mode = pinConf & PIN_MODE_MASK;
	GPIO_InitStruct.Pull = (pinConf & PIN_PP_MASK) >> 4;
	GPIO_InitStruct.Speed = (pinConf & PIN_SPEED_MASK) >> 6;

	//activate clock for port if it is used
    
	//apply configuration
	HAL_GPIO_Init(pinGetPort(pinID), &GPIO_InitStruct);
}



GPIO_TypeDef* pinGetPort(uint8_t pinID)
{
	GPIO_TypeDef *GPIOx;

	switch ((uint8_t)(pinID >> NIBBLE))
	{
	case PORT_GPIOA:
		GPIOx = GPIOA;
		break;
	case PORT_GPIOB:
		GPIOx = GPIOB;
		break;
	case PORT_GPIOC:
		GPIOx = GPIOC;
		break;
	case PORT_GPIOD:
		GPIOx = GPIOD;
		break;    
		//FIXME: PORT E does not exist on stm32G07x
	case PORT_GPIOE:
		break;
	case PORT_GPIOF:
		GPIOx = GPIOF;
		break;    
	default:
		//add some kind of invalid command handling
		break;
	}

	return GPIOx;
}

uint32_t pinGetNumber(uint8_t pinID)
{
	return (1 << ((uint32_t)pinID & 0x000F));
}

void digitalWrite(uint8_t pinID, uint8_t value)
{
	HAL_GPIO_WritePin(pinGetPort(pinID), pinGetNumber(pinID), value);
}

uint8_t digitalRead(uint8_t pinID)
{
	return HAL_GPIO_ReadPin(pinGetPort(pinID), pinGetNumber(pinID));
}

void adcConfigure1(uint8_t adc, uint8_t channel_number)
{
	ADC_HandleTypeDef *hadc;
	ADC_ChannelConfTypeDef sConfig = { 0 };
	uint32_t channel = 0;

	channel = 0x01 << channel_number;

	hadc = adcCheckValidity(adc);

	//only proceed if the configuration can be done

	if (hadc != NULL)
	{
		adcUsedChannels[adc] |= channel;
	}
}

void adcConfigure(uint8_t adc, uint32_t channel)
{
	ADC_HandleTypeDef *hadc;
	ADC_ChannelConfTypeDef sConfig = { 0 };




	hadc = adcCheckValidity(adc);

	//only proceed if the configuration can be done
	if (hadc != NULL)
	{
		//assign rank to channel with lower index for scan mode so the array has a consistent order
		//independently from total number of channels used
		for (int i = 0; i < MAX_ADC_HW_INSTANCES; i++)
		{
			if (channel < adcUsedChannels[i]) sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
		}

		//pass channel so it is configured
		sConfig.Channel = channel;

		//disable  ADC first
		// while(HAL_ADC_Stop(hadc) == HAL_BUSY)
		// {
		//     HAL_Delay(10);
		// }

		// hadc->Init.NbrOfConversion++;

		HAL_ADC_Stop(hadc);
		HAL_ADC_Stop_DMA(hadc);

		//configure channel
		if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) Error_Handler();
        
		//enable adc again
		// while(HAL_ADC_Start(hadc) == HAL_BUSY)
		// {
		//     HAL_Delay(10);
		// }


		//keep track of which channels have been activated
		adcUsedChannels[adc] |= channel;

		// adcStart();
	}
}

ADC_HandleTypeDef * adcCheckValidity(uint8_t adc)
{
	ADC_HandleTypeDef *hadc = NULL;
   

	//not all families have the same numbers of ADC - filter
#ifdef ADC1
	if (adc == ADC1_INSTANCE) 
	{
		hadc = &hadc1;
		hadc->Instance = ADC1;
		return hadc;
	}
#endif

#ifdef ADC2
	if (adc == ADC2_INSTANCE) 
	{
		hadc = &hadc2;
		hadc->Instance = ADC2;
		return hadc;
	}
#endif

#ifdef ADC3
	if (adc == ADC3_INSTANCE) 
	{
		hadc = &hadc3;
		hadc->Instance = ADC3;
		return hadc;
	}
#endif

#ifdef ADC4
	if (adc == ADC1_INSTANCE) 
	{
		hadc = &hadc4;
		hadc->Instance = ADC4;
		return hadc;
	}
#endif
	return hadc;
}






