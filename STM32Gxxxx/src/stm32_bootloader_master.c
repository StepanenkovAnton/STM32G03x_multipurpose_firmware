#include "stm32_bootloader_master.h"
#include "string.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "device.h"
#include  "esp_log.h"
#include "esp_err.h"

static const char *TAG = "STM32G03xx_i2c_bootloader";

/****************************************************************************************/
/*                                                                                      */
/*                          Static functions declaration                                */
/*                                                                                      */
/****************************************************************************************/
static uint8_t calculateChecksum(const uint8_t message[], uint16_t messageLen);
static uint8_t sendCommandFrame(uint8_t STMslaveAddress, uint8_t command);
static uint8_t sendStartingAddress(uint8_t STMslaveAddress, uint32_t address);


/****************************************************************************************/
/*                                                                                      */
/*                          Global functions implementations                            */
/*                                                                                      */
/****************************************************************************************/
uint8_t stmBlGetVersion(uint8_t STMslaveAddress)
{
    uint8_t acknowledgement;
    acknowledgement = sendCommandFrame(STMslaveAddress, BL_GET_VER_COMMAND);

    if (acknowledgement == BL_NACK_CODE)
    {
        return 0xff;
    }

    uint8_t version = 0;
    i2cMasterRead(STMslaveAddress, &version, 1);
    i2cMasterRead(STMslaveAddress, &acknowledgement, 1);

    if (acknowledgement == BL_NACK_CODE)
    {
        return 0xff;
    }
    return version;
}

uint16_t stmBlGetId(uint8_t STMslaveAddress)
{
    uint8_t acknowledgement;
    acknowledgement = sendCommandFrame(STMslaveAddress, BL_GET_ID_COMMAND);

    if (acknowledgement == BL_NACK_CODE)
    {
    return 0xFFFF;
    }

    uint8_t message[3] = {0, 0, 0};
    i2cMasterRead(STMslaveAddress, message, 3);
    i2cMasterRead(STMslaveAddress, &acknowledgement, 1);

    if (acknowledgement == BL_NACK_CODE)
    {
        return 0xFFFF;
    }
    return (message[1] << 8 | message[2]);
    
}

uint16_t stmBlReadMemory(uint8_t STMslaveAddress, uint32_t startingAddress, uint16_t numberOfBytes, uint8_t *data)
{
    if(startingAddress % 4 != 0)
    {
        return 1; // @todo add proper error code here.
    }
    if( numberOfBytes > BL_MAX_MESSAGE_LEN)
    {
        return 1;
    }

    uint8_t acknowledgement = 0;

    acknowledgement = sendCommandFrame(STMslaveAddress, BL_RMEM_COMMAND);
    if (acknowledgement == BL_NACK_CODE)
    {
        return 1; // @todo add proper error code here.
    }

    acknowledgement = sendStartingAddress(STMslaveAddress, startingAddress);
    if (acknowledgement == BL_NACK_CODE)
    {
        return 1; // @todo add proper error code here.
    }

    uint8_t message[2];
    message[0] = (numberOfBytes - 1U);
    message[1] = (numberOfBytes - 1U) ^ 0xFFU;
    i2cMasterWrite(STMslaveAddress, message, 2);
    i2cMasterRead(STMslaveAddress, &acknowledgement, 1);

    if (acknowledgement == BL_NACK_CODE)
    {
        return 1; // @todo add proper error code here.
    }

    i2cMasterRead(STMslaveAddress, data, numberOfBytes);
    return 0;
}

/*!
 * @brief  
 *  
 * @note Starting address must be multiple of 4 bytes. 
 * @note It is crucially important to understand, that data must be multiple of 4 words (16 bytes) to be written sequentially.
 *      It means, that if you will try to write 60 bytes, and then 60 more right after, the bootloader won't write the last 60 bytes.
 *      This happens because bootloader fills packages that are not multiple of 16 bytes with trailing zeros, and writes these zeros to the memory.
 *      So technically by writing 60 bytes the first time, the bootloader will write 64 (with 4 zeros at the end). Because of that, the next write access will
 *      try to write to the memory location, that was already written with 0 by the bootloader.
 *      This will lead to the bootloader aborting the writing operation.
 *      It is still possible to write data that is not multiple of 16 bytes (for example last portion of the bin file),
 *      but you should make sure, that you don't try to write data into the auto-filled address.
 */ 
uint16_t stmBlWriteMemory(uint8_t STMslaveAddress, uint32_t startingAddress, uint16_t numberOfBytes, uint8_t *data)
{
    if (startingAddress % 4 != 0) // You can't write to an address not multiple of 4
    {
        return 1; // @todo add proper error code here.
    }
    if (numberOfBytes > BL_MAX_MESSAGE_LEN)
    {
        return 1;
    }

    uint8_t acknowledgement = 0;

    //ESP_LOGI(TAG,"before send command frame");
    acknowledgement = sendCommandFrame(STMslaveAddress, BL_WMEM_COMMAND);
    //ESP_LOGI(TAG,"after send command frame");
    if (acknowledgement == BL_NACK_CODE)
    {
        return 1; // @todo add proper error code here.
    }

    //ESP_LOGI(TAG,"before send starting address");
    acknowledgement = sendStartingAddress(STMslaveAddress, startingAddress);
    //ESP_LOGI(TAG,"after send starting address");
    if (acknowledgement == BL_NACK_CODE)
    {
        return 1; // @todo add proper error code here.
    }
    uint8_t message[BL_MAX_MESSAGE_LEN + 2];
    message[0] = numberOfBytes - 1;
    memcpy(&message[1], data, numberOfBytes); // copy input array into new location
    message[numberOfBytes + 1] = calculateChecksum(message, numberOfBytes + 1);
    i2cMasterWrite(STMslaveAddress, message, numberOfBytes + 2);
    vTaskDelay(40 / portTICK_RATE_MS); 
    i2cMasterRead(STMslaveAddress, &acknowledgement, 1);
    if(acknowledgement == BL_NACK_CODE)
    {
        return 1; // @todo add proper error code here.
    }

    return 0;
}

/*!
 * @brief 
 *
 * @note You can't erase more than 128 sectors at once. This is not a documentation, but an implementation limitation.
 */
uint16_t stmBlEraseMemory(uint8_t STMslaveAddress, uint16_t numberOfSectors, uint16_t startingSector)
{
    if( numberOfSectors == 0)
    {
        return 1;
    }
    if(numberOfSectors > 128 && numberOfSectors < 0xFFF0U)
    {
        return 1;
    }
    
    uint8_t acknowledgement = 0;

    acknowledgement = sendCommandFrame(STMslaveAddress, BL_EMEM_COMMAND);
    if (acknowledgement == BL_NACK_CODE)
    {
        return 1; // @todo add proper error code here.
    }

    uint8_t message[3];
    if ((numberOfSectors & 0xFFF0) == 0xFFF0)
    {
        message[0] = (numberOfSectors >> 8) & 0xFF;
        message[1] = numberOfSectors & 0xFF;
    }
    else
    {
        message[0] = ((numberOfSectors - 1) >> 8) & 0xFF;
        message[1] = (numberOfSectors - 1) & 0xFF;
    }
    message[2] = message[0] ^ message[1];

    i2cMasterWrite(STMslaveAddress, message, 3);
    
    
    vTaskDelay(40 / portTICK_RATE_MS);                                       

    i2cMasterRead(STMslaveAddress, &acknowledgement, 1);
    if (acknowledgement == BL_NACK_CODE)
    {
        return 1;
    }

    if((numberOfSectors & 0xFFF0) == 0xFFF0)
    {
        return 0;
    }

    uint8_t sectors[257];
    for (int32_t i = 0; i < numberOfSectors; i += 1)
    {
        sectors[2 * i] = ((startingSector + i) >> 8) & 0xFF;
        sectors[2 * i + 1] = (startingSector + i) & 0xFF;
    }
    sectors[2 * numberOfSectors] = calculateChecksum(sectors, numberOfSectors * 2);

    i2cMasterWrite(STMslaveAddress, sectors, (numberOfSectors * 2) + 1);
   
    vTaskDelay(40*numberOfSectors / portTICK_RATE_MS);                                 
    
    i2cMasterRead(STMslaveAddress, &acknowledgement, 1);

    if (acknowledgement == BL_NACK_CODE)
    {
        return 1;
    }

    return 0;
}

/****************************************************************************************/
/*                                                                                      */
/*                          Static functions implementations                            */
/*                                                                                      */
/****************************************************************************************/
static uint8_t calculateChecksum(const uint8_t message[], uint16_t messageLen)
{
    uint8_t sum = 0;
    for (uint16_t i = 0; i < messageLen; i += 1)
    {
        sum ^= message[i];
    }
    return sum;
}

static uint8_t sendCommandFrame(uint8_t STMslaveAddress, uint8_t command)
{
    uint8_t acknowledgement = 0;
    uint8_t commandFrame[2];

    commandFrame[0] = command;
    commandFrame[1] = command ^ 0xFFU;
    i2cMasterWrite(STMslaveAddress, commandFrame, 2);
    acknowledgement = 0;
    i2cMasterRead(STMslaveAddress, &acknowledgement, 1);
    ESP_LOGI(TAG,"acknolegement from sendCommandFrame: %x",acknowledgement);

    return acknowledgement;
}

static uint8_t sendStartingAddress(uint8_t STMslaveAddress, uint32_t address)
{
    uint8_t message[5];
    uint8_t acknowledgement = 0;

    message[0] = (uint8_t)((address >> 24) & 0xFF);
    message[1] = (uint8_t)((address >> 16) & 0xFF);
    message[2] = (uint8_t)((address >> 8) & 0xFF);
    message[3] = (uint8_t)(address & 0xFF);
    message[4] = calculateChecksum(message, 4);

    i2cMasterWrite(STMslaveAddress, message, 5);
    i2cMasterRead(STMslaveAddress, &acknowledgement, 1);
    ESP_LOGI(TAG,"acknolegement from sendSatrtingAddress: %x",acknowledgement);
    return acknowledgement;
}

