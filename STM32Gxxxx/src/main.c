#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "device.h"
#include "STM32_control.h"
#include "stm32_bootloader_master.h"
static const char *TAG = "STM32Gxxxx_master";

//uncomment below if you want to enable an example STM32 reflashing procedure
//with the GPS readings to debug serial output
//note that the proper partitions structure must be set with SPIFFS present and the firmware file
///must be uploaded to SPIFFS filesystem
//#define reflashSTM32

//uncomment below if you want the ESP32 to request and output to debug serial the decoded GPS data
 #define readGPS

void app_main() {
    uint8_t err_code;
    //start and initialize I2C
    ESP_LOGI(TAG, "Before I2C init");
    vTaskDelay(500 / portTICK_RATE_MS);
    err_code = i2c_master_init();
    ESP_LOGI(TAG, "I2C initialized successfully");
    vTaskDelay(500 / portTICK_RATE_MS);

#ifdef reflashSTM32
    //hardware reset the stm32 before to start the boot process
    startBootloaderAccess();
    ESP_LOGI(TAG, "startBootloaderAccess();-done");
    vTaskDelay(1000 / portTICK_RATE_MS);//Just for debug. May be decreased to 100 mS.

    //Alternatively, you can call a software reset like this:
    if (STM32_Reset( 0x04) == ESP_OK )
        {
            ESP_LOGI(TAG,"Wrong address command SUCCESS");
        }
        else
        {
            ESP_LOGI(TAG,"Wrong address command ERROR");
        }


    uint8_t slaveAddressBuffer;
    slaveAddressBuffer = STM32G03X_SLAVE_ADDRESS;//STM32 address in boot mode buffer

    //get and display chip ID
    uint16_t id = stmBlGetId(slaveAddressBuffer);
    ESP_LOGI(TAG,"Device ID: %d",id);

    //get and display bootloader version
    uint8_t version = stmBlGetVersion(slaveAddressBuffer);
    ESP_LOGI(TAG,"Bootloader versionH: %d",version/10);
    ESP_LOGI(TAG,"Bootloader versionL: %d",version%10);

    int8_t buffer;
    buffer = readStm_nBOOT_SEL_Bit(slaveAddressBuffer);// @note Check if nBOOT_SEL_Bit is set to 0, and update if it is is not
    //nBOOT_SEL_Bit set to 0 will let the STM32 go to boot mode on the signal on boot pin otherwise
    //the STM32 will not go into boot mode on the boot pin once the flash memory is not empty
    if (buffer == -1)
    {
        ESP_LOGI(TAG,"An Error has occurred while reading the nBOOT_Bit");
    }

    if (buffer == 1)
    {
        ESP_LOGI(TAG,"Start writing nBOOT_Sel_Bit");
        writeStm_nBOOT_SEL_Bit(slaveAddressBuffer,0);
        vTaskDelay(100 / portTICK_RATE_MS);
        finishBootloaderAccess();
        vTaskDelay(100 / portTICK_RATE_MS);
        startBootloaderAccess();// @note Re-initialise chip as the bootloader communication stops after memory write command
        vTaskDelay(500 / portTICK_RATE_MS);
    }
    if (buffer == 0)
    {
        ESP_LOGI(TAG,"nBOOT_Sel_Bit is already 0");
    }

    flashStmFromFile(slaveAddressBuffer,"/spiffs/STM32I2CSlave.bin");
    finishBootloaderAccess();

    ESP_LOGI(TAG,"Firmware update is finished");
    configureBootloaderGPIOsAsHiZ();
    ESP_LOGI(TAG, "GPIOs HiZ configuration applied");
    vTaskDelay(10000 / portTICK_RATE_MS);//for debug. May be decreased to 100 mS
#endif


    //apply pins configuration
    STM32PinConfiguration PinConfuguration;
    uint8_t STM32_pin_state_buffer[3] = {0};
    uint16_t STM32_ADC_reading_buffer[30] = {0};

    //ping with wring I2C address. For testing puroses. May be omitted.
    for (uint8_t i = 0; i < 3; i++)
    {
        vTaskDelay(100 / portTICK_RATE_MS);
        PinConfuguration.in_out_analog =        STM32_GPIO_MODE_OUTPUT_OD;
        PinConfuguration.pull_up_pull_down =    STM32_GPIO_PULLUP;
        PinConfuguration.speed_mode =           STM32_GPIO_SPEED_FREQ_MEDIUM;
        if (STM32_setPinConfiguration( 0x06, 0, 0, PinConfuguration) == ESP_OK )
        {
            ESP_LOGI(TAG,"Wrong address command SUCCESS");
        }
        else
        {
            ESP_LOGI(TAG,"Wrong address command ERROR");
        }
    }
    
    uint8_t attemptsCounter = 0;//for debuging purposes, indicates the I2C link stability
    uint8_t toogleBuffer =0;//for switching pins on/off

    GPS_data GPS_data_buffer;

    while(1)
    {
        PinConfuguration.in_out_analog = STM32_GPIO_MODE_OUTPUT_PP;
        PinConfuguration.pull_up_pull_down = STM32_GPIO_NOPULL;
        PinConfuguration.speed_mode = STM32_GPIO_SPEED_FREQ_MEDIUM;
        if (STM32_setPinConfiguration( 0x04, 0, 4, PinConfuguration) == ESP_OK )
        {
            ESP_LOGI(TAG,"Pin configuration command SUCCESS");
        }
        else
        {
            ESP_LOGI(TAG,"Pin configuration command ERROR");
        }
        vTaskDelay(100 / portTICK_RATE_MS);
        //if you want to use pin for analog reading be sure to confugure it as analog first
        PinConfuguration.in_out_analog = STM32_GPIO_MODE_ANALOG;
        PinConfuguration.pull_up_pull_down = STM32_GPIO_NOPULL;
        PinConfuguration.speed_mode = STM32_GPIO_SPEED_FREQ_MEDIUM;
        if (STM32_setPinConfiguration( 0x04, 0, 11, PinConfuguration) == ESP_OK )
        {
            ESP_LOGI(TAG,"Pin configuration command SUCCESS");
        }
        else
        {
            ESP_LOGI(TAG,"Pin configuration command ERROR");
        }

        for (uint8_t i = 0; i < 24; i++)
        {
            vTaskDelay(100/ portTICK_RATE_MS);
            STM32_analogRead(0x04, 0, i, &STM32_ADC_reading_buffer[i]);
        }
        for (uint8_t i = 0; i < 24; i++)
        {
            ESP_LOGI(TAG,"ADC CH# %d Value: %d",i,STM32_ADC_reading_buffer[i]);
        }
        

        vTaskDelay(100 / portTICK_RATE_MS);
        if (STM32_digitalRead( 0x04, 0, 4, &STM32_pin_state_buffer) == ESP_OK )
        {
            attemptsCounter++;
            ESP_LOGI(TAG,"STM32_digitalRead command SUCCESS: %x", STM32_pin_state_buffer[0]);
        }
        else
        {
            ESP_LOGI(TAG,"STM32_digitalRead command ERROR");
        }
        
        
        vTaskDelay(100 / portTICK_RATE_MS);
        if (STM32_digitalWrite(0x04, 0, 4, toogleBuffer) == ESP_OK )
        {
            attemptsCounter++;
            ESP_LOGI(TAG,"STM32_digitalWrite2 command SUCCESS %x,%x,%x", STM32_pin_state_buffer[0], STM32_pin_state_buffer[1], STM32_pin_state_buffer[2]);
        }
        else
        {
            ESP_LOGI(TAG,"STM32_digitalWrite2 command ERROR");
        }

        vTaskDelay(100 / portTICK_RATE_MS);
#ifdef readGPS
        if (STM32_GPS_data_Read( 0x04, &GPS_data_buffer) == ESP_OK )
        {
            ESP_LOGI(TAG,"GPS packet receive SUCCESS");
            ESP_LOGI(TAG,"latitude_degrees: %d",GPS_data_buffer.latitude_degrees);
            ESP_LOGI(TAG,"latitude_minutes: %d",GPS_data_buffer.latitude_minutes);
            ESP_LOGI(TAG,"latitude_hundred_thouthandths_of_minutes: %d",GPS_data_buffer.latitude_hundred_thouthandths_of_minutes);
            ESP_LOGI(TAG,"longitude_degrees: %d",GPS_data_buffer.longitude_degrees);
            ESP_LOGI(TAG,"longitude_minutes: %d",GPS_data_buffer.longitude_minutes);
            ESP_LOGI(TAG,"latitude_direction: %d",GPS_data_buffer.latitude_direction);
            ESP_LOGI(TAG,"longitude_direction: %d",GPS_data_buffer.longitude_direction);
            ESP_LOGI(TAG,"UTC_time_hours: %d",GPS_data_buffer.UTC_time_hours);
            ESP_LOGI(TAG,"UTC_time_mimutes: %d",GPS_data_buffer.UTC_time_mimutes);
            ESP_LOGI(TAG,"UTC_time_ten_thouthandths_of_seconds: %d",GPS_data_buffer.UTC_time_ten_thouthandths_of_seconds);
            ESP_LOGI(TAG,"UT_date_day: %d",GPS_data_buffer.UT_date_day);
            ESP_LOGI(TAG,"UT_date_month: %d",GPS_data_buffer.UT_date_month);
            ESP_LOGI(TAG,"UT_date_year: %d",GPS_data_buffer.UT_date_year);
            ESP_LOGI(TAG,"tenths_of_altitude: %d",GPS_data_buffer.tenths_of_altitude);
            ESP_LOGI(TAG,"tenths_of_geoidal_separation: %d",GPS_data_buffer.tenths_of_geoidal_separation);
            ESP_LOGI(TAG,"fix: %d",GPS_data_buffer.fix);
            ESP_LOGI(TAG,"fix_mode: %d",GPS_data_buffer.fix_mode);
            ESP_LOGI(TAG,"tenths_of_PDOP: %d",GPS_data_buffer.tenths_of_PDOP);
            ESP_LOGI(TAG,"tenths_of_HDOP: %d",GPS_data_buffer.tenths_of_HDOP);
            ESP_LOGI(TAG,"tenths_of_VDOP: %d",GPS_data_buffer.tenths_of_VDOP);
        }
        else
        {
            ESP_LOGI(TAG,"GPS packet receive ERROR");
        }

#endif
        uint8_t year;uint8_t month;uint8_t day;uint8_t hour; uint8_t min;uint8_t sec; uint8_t dow;


        vTaskDelay(100 / portTICK_RATE_MS);
        if (STM32_RTC_Get( 0x04, &year,&month,&day,&hour,&min,&sec,&dow) == ESP_OK )
        {
            attemptsCounter++;
            ESP_LOGI(TAG,"STM32_RTC_Get command SUCCESS: year-%d, month-%d,day-%d,hour-%d,min-%d,sec-%d,dow-%d", year,month,day,hour,min,sec,dow);
        }
        else
        {
            ESP_LOGI(TAG,"STM32_RTC_Get command ERROR");
        }


        
        vTaskDelay(100 / portTICK_RATE_MS);
        year = 22,month = 11,day = 26,hour = 02,min = 03,sec = 04,dow = 3;
        if (STM32_RTC_Set( 0x04, year,month,day,hour,min,sec,dow) == ESP_OK )
        {
            attemptsCounter++;
            ESP_LOGI(TAG,"STM32_RTC_Set command SUCCESS: year-%d, month-%d,day-%d,hour-%d,min-%d,sec-%d,dow-%d", year,month,day,hour,min,sec,dow);
        //if everything is done right, 
        //the seconds read and set will differ by 10 
        //(delay at the end of loop)
        }
        else
        {
            ESP_LOGI(TAG,"STM32_RTC_Get command ERROR");
        }


        vTaskDelay(2000 / portTICK_RATE_MS);

        if (toogleBuffer == 1){toogleBuffer = 0;}else{toogleBuffer = 1;}

        ESP_LOGI(TAG,"attemptsCounter %d",attemptsCounter);

    }

}