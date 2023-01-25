#ifndef STM_BOOTLOADER_MASTER_H_
#define STM_BOOTLOADER_MASTER_H_
#include "stdint.h"
#include "stm32_bootloader_registers.h"


/**

* @ brief Reads out the bootloader version

* @ param slaveAddress - a 7-bit slave device address

* @ return 0xFF if the error while reading has accured
* the bootloader version

*/
uint8_t stmBlGetVersion(uint8_t STMslaveAddress);

/**

* @ brief Reads out the chip ID

* @ param slaveAddress - a 7-bit slave device address

* @ return 0xFFFF if the error while reading has accured or
* the chip ID

*/
uint16_t stmBlGetId(uint8_t STMslaveAddress);

/**

* @ brief Reads out the chip flash memory

* @ param slaveAddress - a 7-bit slave device address
* startingAddress - address to start to read from
* numberOfBytes - number of bytes to read
* data - pointer to the byte array to read chip memory to

* @ return 0 in case of success,
* 1 in case of error

*/
uint16_t stmBlReadMemory(uint8_t STMslaveAddress, uint32_t startingAddress, uint16_t numberOfBytes, uint8_t *data);

/**

* @ brief Writes to the chip flash memory in portions of 112 bytes maximum

* @ param slaveAddress - a 7-bit slave device address
* startingAddress - address to start to write from
* numberOfBytes - number of bytes to write
* data - pointer to the byte array to write chip memory from

* @ return 0 in case of success,
* 1 in case of error

*/
uint16_t stmBlWriteMemory(uint8_t STMslaveAddress, uint32_t startingAddress, uint16_t numberOfBytes, uint8_t *data);

/**

* @ brief Writes the 0xFF value to the chip flash memory in portions of 112 bytes maximum

* @ param slaveAddress - a 7-bit slave device address
* startingAddress - address to start to write from
* numberOfBytes - number of bytes to write
* data - pointer to the byte array to write chip memory from

* @ return 0 in case of success,
* 1 in case of error

*/
uint16_t stmBlEraseMemory(uint8_t STMslaveAddress, uint16_t numberOfSectors, uint16_t startingSector);

#endif /* STM_BOOTLOADER_MASTER_H_ */