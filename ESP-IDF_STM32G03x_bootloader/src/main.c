#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "device.h"
#include "stm32_bootloader_master.h"

static const char *TAG = "STM32G03xx_i2c_bootloader";


void app_main() 
{
    uint8_t err_code;
    ESP_LOGI(TAG, "Before I2C init");
    vTaskDelay(500 / portTICK_RATE_MS);
    err_code = i2c_master_init();
    ESP_LOGI(TAG, "I2C initialized successfully");
    vTaskDelay(500 / portTICK_RATE_MS);
    configureBootloaderGPIOsAsOutputs();
    ESP_LOGI(TAG, "GPIOs output configuration applied");
    vTaskDelay(500 / portTICK_RATE_MS);

    

    startBootloaderAccess();
    ESP_LOGI(TAG, "startBootloaderAccess();-done");
    vTaskDelay(5000 / portTICK_RATE_MS);

    uint8_t slaveAddressBuffer;
    //uint16_t id;
    //uint8_t version;
    //for (uint8_t i = 86; i < 87; i++)
    //{
    //    slaveAddressBuffer =i;
    //    startBootloaderAccess();
    //    id = stmBlGetId(slaveAddressBuffer);
    //    ESP_LOGI(TAG, "Device ADDRESS:%x",i);/////////////////////
    //    ESP_LOGI(TAG, "Device ID: %x",id);/////////////////////////////////

    //    version = stmBlGetVersion(slaveAddressBuffer);


    //    if (!(version == 255))
    //    {
    //        ESP_LOGI(TAG,"Bootloader versionH: %d",version/10);
    //        ESP_LOGI(TAG,"Bootloader versionL: %d",version%10);
    //        if (i == 0x56)
    //        {
    //            uint8_t buffer;
    //            buffer = readStm_nBOOT_SEL_Bit(slaveAddressBuffer);
    //            ESP_LOGI(TAG,"nBOOT_SEL_Bit is: %d", buffer);
    //            finishBootloaderAccess();
    //            vTaskDelay(1000 / portTICK_RATE_MS);
    //            startBootloaderAccess();
    //            vTaskDelay(1000 / portTICK_RATE_MS);
    //            ESP_LOGI(TAG,"Start writing");
    //            writeStm_nBOOT_SEL_Bit(slaveAddressBuffer,0);
    //            ESP_LOGI(TAG,"End writing");
    //            finishBootloaderAccess();
    //            vTaskDelay(1000 / portTICK_RATE_MS);
    //            startBootloaderAccess();
    //            vTaskDelay(1000 / portTICK_RATE_MS);
    //            buffer =readStm_nBOOT_SEL_Bit(slaveAddressBuffer);
    //            ESP_LOGI(TAG,"nBOOT_SEL_Bit_is: %d",buffer);
    //        }

    //    }
    //    finishBootloaderAccess();
    //    vTaskDelay(100 / portTICK_RATE_MS);
    //}
//////////////////////////////////////

  //slaveAddressBuffer = 0x56;
  slaveAddressBuffer = STM32G03X_SLAVE_ADDRESS;
  uint16_t id = stmBlGetId(slaveAddressBuffer);
  ESP_LOGI(TAG,"Device ID: %d",id);

  uint8_t version = stmBlGetVersion(slaveAddressBuffer);
  ESP_LOGI(TAG,"Bootloader versionH: %d",version/10);
  ESP_LOGI(TAG,"Bootloader versionL: %d",version%10);

  uint8_t buffer;
  buffer = readStm_nBOOT_SEL_Bit(slaveAddressBuffer);// @note Check if nBOOT_SEL_Bit is set to 0, and update if it is is not

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
  

  flashStmFromFile(slaveAddressBuffer,"/spiffs/STM32G030_BLINK.bin");
  finishBootloaderAccess();
  vTaskDelay(10000 / portTICK_RATE_MS);


  ESP_LOGI(TAG,"Firmware update is finished");

  configureBootloaderGPIOsAsHiZ();

  ESP_LOGI(TAG, "GPIOs HiZ configuration applied");

while(1)
{
    vTaskDelay(30 / portTICK_RATE_MS);
}
    
}