#include "STM32_control.h"
#include "string.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "device.h"
#include  "esp_log.h"
#include "esp_err.h"

static const char *TAG = "STM32Gxxxx_master";

//see Protocol.docx file for I2C transaction bytes order and meaning

esp_err_t STM32_setPinConfiguration ( uint8_t STM32_I2C_Slave_Address, uint8_t STM32_port, uint8_t STM32_pin,STM32PinConfiguration STM32_Pin_Configuration)
{
    uint8_t message[3] = {0};
    esp_err_t ret;

    message[0] = 0x10;
    
    message[1] |= (STM32_pin & 0x0F);
    message[1] |= ((STM32_port << 4) & 0xF0);

    message[2] |=   (STM32_Pin_Configuration.in_out_analog & PIN_MODE_MASK) | 
                    ( (STM32_Pin_Configuration.pull_up_pull_down << 4 ) & PIN_PP_MASK ) | 
                    ( (STM32_Pin_Configuration.speed_mode << 6 ) & PIN_SPEED_MASK );

    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);

    
    return ret;
    
}
esp_err_t STM32_digitalWrite ( uint8_t STM32_I2C_Slave_Address, uint8_t STM32_port, uint8_t STM32_pin,uint8_t STM32_pin_state )
{
    uint8_t message[3] = {0};
    uint8_t ret;
    message[0] = 0x11;
    
    message[1] |= (STM32_pin & 0x0F) | ((STM32_port << 4) & 0xF0);
    message[2] = STM32_pin_state;

    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    return ret;
}
esp_err_t STM32_digitalRead ( uint8_t STM32_I2C_Slave_Address, uint8_t STM32_port, uint8_t STM32_pin, uint8_t *STM32_pin_state)
{
    uint8_t message[3] = {0};
    esp_err_t ret;

    message[0] = 0x12;
    message[1] |= (STM32_pin & 0x0F);
    message[1] |= ((STM32_port << 4) & 0xF0);
    message[2] = 0x00;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    //vTaskDelay(100 / portTICK_RATE_MS);
    ret = i2cMasterRead(STM32_I2C_Slave_Address, &message,3);

    STM32_pin_state[0]=message[0];
    STM32_pin_state[1]=message[1];
    STM32_pin_state[2]=message[2];
    return ret;
}

esp_err_t STM32_Configure_AnalogChannel (uint8_t STM32_I2C_Slave_Address, uint8_t ADC_Instance_Number, uint8_t ADC_Channel_Number)
{
   uint8_t message[3] = {0};
    esp_err_t ret;
      
    message[0] = 0x33;
    
    message[1] |= ( ((ADC_Instance_Number & 0x03)<<6 ) | (ADC_Channel_Number &0x3F) );
    message[2] = 0x00;

    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    return ret;
}
esp_err_t STM32_analogRead ( uint8_t STM32_I2C_Slave_Address, uint8_t ADC_instance, uint8_t ADC_channel, uint16_t *STM32_ADC_reading )
{
    uint8_t message[3] = {0};
    esp_err_t ret;
    message[0] = 0x30;
    message[1] |= ADC_instance;
    message[2] |= ADC_channel;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    ret = i2cMasterRead(STM32_I2C_Slave_Address, &message,3);
    STM32_ADC_reading[0] = message[0]<<8;
    STM32_ADC_reading[0] = STM32_ADC_reading[0] + message[1];
    return ret;
}

esp_err_t STM32_GPS_data_Read ( uint8_t STM32_I2C_Slave_Address, GPS_data *GPS_output_data )

{
    uint8_t message[31] = {0};
    esp_err_t ret;
    message[0] = 0x60;
    message[1] |= 0x00;
    message[2] |= 0x00;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);

    ret = i2cMasterRead(STM32_I2C_Slave_Address, &message,31);

    GPS_output_data->latitude_degrees = message[0];
    GPS_output_data->latitude_minutes = message[1];
    GPS_output_data->latitude_hundred_thouthandths_of_minutes = message[2]*10000+message[3]*100+message[4];
    
    GPS_output_data->longitude_degrees = message[5];
    GPS_output_data->longitude_minutes = message[6];
    GPS_output_data->longitude_hundred_thouthandths_of_minutes = message[7]*10000+message[8]*100+message[9];
    
    GPS_output_data->latitude_direction = (message[10] & 0x10)>>4;
    GPS_output_data->longitude_direction = (message[10] & 0x01);

    GPS_output_data->UTC_time_hours = message[11];
    GPS_output_data->UTC_time_mimutes = message[12];
    GPS_output_data->UTC_time_ten_thouthandths_of_seconds=message[14]*100 + message[15];

    GPS_output_data->UT_date_day = message[16];
    GPS_output_data->UT_date_month = message[17];
    GPS_output_data->UT_date_year = message[18];

    GPS_output_data->tenths_of_altitude = message[19]*10000 + message[20]*100 + message[21];
    GPS_output_data->tenths_of_geoidal_separation = message[22]*10000 + message[23]*100 + message[24];
    GPS_output_data->fix = message[25];
    GPS_output_data->fix_mode = message[26];
    GPS_output_data->tenths_of_PDOP = message[27];
    GPS_output_data->tenths_of_HDOP = message[28];
    GPS_output_data->tenths_of_VDOP = message[29];


    
    return ret;
}

esp_err_t STM32_RTC_Get (uint8_t STM32_I2C_Slave_Address,uint8_t *year, uint8_t *month, 
						uint8_t *day,uint8_t *hour, uint8_t *min, 
						uint8_t *sec, uint8_t *dow)
{
    uint8_t message[3] = {0};
    esp_err_t ret;

    message[0] = 0x40;
    message[1] = 0x00;
    message[2] = 0x00;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    ret = i2cMasterRead(STM32_I2C_Slave_Address, &message,7);
    day[0] =  message[0];
    month[0] = message[1];
    year[0] =  message[2];
    hour[0] =  message[3];
    min[0] =   message[4];
    sec[0] =   message[5];
    dow[0] =   message[6];

    return ret;

}
esp_err_t STM32_RTC_Set (uint8_t STM32_I2C_Slave_Address,uint8_t year, uint8_t month, 
						uint8_t day,uint8_t hour, uint8_t min, 
						uint8_t sec, uint8_t dow)
{
    uint8_t message[3] = {0};
    esp_err_t ret;
      
    message[0] = 0x41;
    message[1] = day;
    message[2] = 0x00;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    vTaskDelay(10 / portTICK_RATE_MS);

    message[0] = 0x42;
    message[1] = month;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    vTaskDelay(10 / portTICK_RATE_MS);

    message[0] = 0x43;
    message[1] = year;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    vTaskDelay(10 / portTICK_RATE_MS);

    message[0] = 0x44;
    message[1] = hour;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    vTaskDelay(10 / portTICK_RATE_MS);

    message[0] = 0x45;
    message[1] = min;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    vTaskDelay(10 / portTICK_RATE_MS);

    message[0] = 0x46;
    message[1] = sec;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    vTaskDelay(10 / portTICK_RATE_MS);

    message[0] = 0x47;
    message[1] = dow;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    vTaskDelay(10 / portTICK_RATE_MS);

    message[0] = 0x48;
    message[1] = 0x00;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);

    return ret;
}


esp_err_t STM32_Reset (uint8_t STM32_I2C_Slave_Address)
{
    uint8_t message[3] = {0};
    esp_err_t ret;

    message[0] = 0x99;
    message[1] = 0x00;
    message[2] = 0x00;
    ret = i2cMasterWrite(STM32_I2C_Slave_Address, message, 3);
    
    return ret;

}