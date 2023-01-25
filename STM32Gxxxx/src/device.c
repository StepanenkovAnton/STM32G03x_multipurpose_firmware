#ifndef DEVICE_C_
#define DEVICE_C_

#include <stdio.h>
#include "driver/i2c.h"
#include  "esp_log.h"
#include "esp_err.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "stm32_bootloader_registers.h"
#include "stm32_bootloader_master.h"
#include "string.h"
#include "device.h"
#include "esp_spiffs.h"

static const char *TAG = "STM32Gxxxx_master";

void configureBootloaderGPIOsAsOutputs(){
gpio_config_t io_conf;
    //GPIO CONFIGURE
   io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << NRST_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
   io_conf.pin_bit_mask = (1ULL << BOOT_PIN);
   gpio_config(&io_conf);
   //END GPIO CONFIGURE

    //set initial gpio output levels    
   gpio_set_level( BOOT_PIN,0);
   gpio_set_level( NRST_PIN,1);
   //END set initial gpio output levels
}

void configureBootloaderGPIOsAsHiZ(){
gpio_config_t io_conf;
    //GPIO CONFIGURE
   io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_DISABLE;
    io_conf.pin_bit_mask = (1ULL << NRST_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
   io_conf.pin_bit_mask = (1ULL << BOOT_PIN);
   gpio_config(&io_conf);
   //END GPIO CONFIGURE
}

void startBootloaderAccess(void)
{
    gpio_set_level( NRST_PIN, 0 );
    gpio_set_level( BOOT_PIN, 1 );
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level( NRST_PIN, 1 );
    vTaskDelay(10 / portTICK_RATE_MS );
}

void finishBootloaderAccess(void)
{
    gpio_set_level( NRST_PIN, 0 );
    gpio_set_level( BOOT_PIN, 0 );
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level( NRST_PIN, 1 );
    vTaskDelay(10 / portTICK_RATE_MS);
}


uint8_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

uint8_t  i2cMasterWrite(uint8_t slaveAddress, uint8_t *data_wr, uint16_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddress << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

uint8_t  i2cMasterRead(uint8_t slaveAddress, uint8_t *data_rd, uint16_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddress << 1) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

uint8_t readStm_nBOOT_SEL_Bit(uint8_t STMslaveAddress)
{
    
    uint32_t index = 8;
    uint8_t readMemoryBuffer[index]; // There is a reason for such a buffer (explain here).
    int8_t result;

    result = -1;
    stmBlReadMemory(STMslaveAddress, 0x1FFF7800, index, readMemoryBuffer);
    //stmBlReadMemory(STMslaveAddress, 0x08000000, index, readMemoryBuffer);
    ESP_LOGI(TAG," readMemoryBuffer[0] is: %x", readMemoryBuffer[0]);
    ESP_LOGI(TAG," readMemoryBuffer[1] is: %x", readMemoryBuffer[1]);
    ESP_LOGI(TAG," readMemoryBuffer[2] is: %x", readMemoryBuffer[2]);
    ESP_LOGI(TAG," readMemoryBuffer[3] is: %x", readMemoryBuffer[3]);
    ESP_LOGI(TAG," readMemoryBuffer[4] is: %x", readMemoryBuffer[4]);
    ESP_LOGI(TAG," readMemoryBuffer[5] is: %x", readMemoryBuffer[5]);
    ESP_LOGI(TAG," readMemoryBuffer[6] is: %x", readMemoryBuffer[6]);
    ESP_LOGI(TAG," readMemoryBuffer[7] is: %x", readMemoryBuffer[7]);
    if (((readMemoryBuffer[3]&0b00000001) == 0b00000001) && ((readMemoryBuffer[7]&0b00000001) == 0b00000000) )
    {
        result = 1;
    }
    if (((readMemoryBuffer[3]&0b00000001) == 0b00000000) && ((readMemoryBuffer[7]&0b00000001) == 0b00000001) )
    {
        result = 0;
    }

    return result;
}

void writeStm_nBOOT_SEL_Bit(uint8_t STMslaveAddress, uint8_t data)

{
    
    uint32_t index = 8;
    uint8_t readMemoryBuffer[index];
    uint8_t dataBuffer[index];

    stmBlReadMemory(STMslaveAddress, 0x1FFF7800, index, readMemoryBuffer);
    //stmBlReadMemory(STMslaveAddress, 0x08000000, index, readMemoryBuffer);

    dataBuffer[0] = readMemoryBuffer[0];
    dataBuffer[1] = readMemoryBuffer[1];
    dataBuffer[2] = readMemoryBuffer[2];

    if (data == 0)
    {
        dataBuffer[3] = readMemoryBuffer[3] & 0b11111110;
    }
    
    if (data == 1)
    {
        dataBuffer[3] = readMemoryBuffer[3] | 0b00000001;
    }
    dataBuffer[4] = readMemoryBuffer[4];
    dataBuffer[5] = readMemoryBuffer[5];
    dataBuffer[6] = readMemoryBuffer[6];
     if (data == 1)
    {
        dataBuffer[7] = readMemoryBuffer[7] & 0b11111110;
    }
    
    if (data == 0)
    {
        dataBuffer[7] = readMemoryBuffer[7] | 0b00000001;
    }

    
    /////this is the initial "blank" chip configuration with only nBOOT_Set_Bit == 0
    //dataBuffer[0] = 0xAA;
    //dataBuffer[1] = 0xE1;
    //dataBuffer[2] = 0xFF;
    //dataBuffer[3] = 0xDE;
    //dataBuffer[4] = 0x55;
    //dataBuffer[5] = 0x1E;
    //dataBuffer[6] = 0x00;
    //dataBuffer[7] = 0x21;
    /////end of the initial "blank" chip configuration with only nBOOT_Set_Bit == 0

    


    ESP_LOGI(TAG," dataBuffer[0] is: %x", dataBuffer[0]);
    ESP_LOGI(TAG," dataBuffer[1] is: %x", dataBuffer[1]);
    ESP_LOGI(TAG," dataBuffer[2] is: %x", dataBuffer[2]);
    ESP_LOGI(TAG," dataBuffer[3] is: %x", dataBuffer[3]);
    ESP_LOGI(TAG," dataBuffer[4] is: %x", dataBuffer[4]);
    ESP_LOGI(TAG," dataBuffer[5] is: %x", dataBuffer[5]);
    ESP_LOGI(TAG," dataBuffer[6] is: %x", dataBuffer[6]);
    ESP_LOGI(TAG," dataBuffer[7] is: %x", dataBuffer[7]);       

    stmBlWriteMemory(STMslaveAddress, 0x1FFF7800, index, dataBuffer);
    //stmBlWriteMemory(STMslaveAddress, 0x08000000, index, dataBuffer);
}

void flashStmFromFile(uint8_t STMslaveAddress, char *fileName)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    ESP_LOGI(TAG, "Opening file for reading....");
    //FILE* f = fopen("/spiffs/STM32G030_BLINK.bin", "r");
    ESP_LOGE(TAG, "FileName-%s",fileName);
    FILE* f = fopen(fileName, "r");
    if (f == NULL) 
    {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    //ESP_LOGI(TAG, "Starting firmware update from /spiffs/STM32G030_BLINK.bin");

    ESP_LOGI(TAG, "Erasing Flash memory!");
    stmBlEraseMemory(STMslaveAddress, 0xFFFF, 0); // Full erase.
    ESP_LOGI(TAG, "Memory was erased!");

    ESP_LOGI(TAG, "Flashing the firmware!");
    uint32_t writeNumber = 0;
    uint32_t index = 0;
    uint8_t bootloaderBuffer[112]; // There is a reason for such a buffer (explain here).
    char buf[4];
    while (!feof(f))
    {
         while ((index != 112) && (!feof(f)))
        {
            fgets(buf,1+1,f);
            if (!feof(f)) 
            {
                bootloaderBuffer[index] = buf [0];
                //ESP_LOGI(TAG, "byte# %x",index+writeNumber*112); 
                //ESP_LOGI(TAG, "byte Val# %x",bootloaderBuffer[index+writeNumber*112]);
            }
        index = index + 1;
        }
        stmBlWriteMemory(STMslaveAddress, (0x08000000 + (writeNumber * 112)), index, bootloaderBuffer);
        writeNumber += 1;
        index = 0;
        
    }
    fclose(f);
    ESP_LOGI(TAG, "The firmware update is finished!");
}





#endif /* DEVICE_C_ */