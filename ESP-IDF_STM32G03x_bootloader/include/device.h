#ifndef DEVICE_H_
#define DEVICE_H_


#define I2C_MASTER_SCL_IO           22                          /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           21                          /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          100000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       100

#define WRITE_BIT I2C_MASTER_WRITE                              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                                             /*!< I2C ack value */
#define NACK_VAL 0x1                                            /*!< I2C nack value */

#define STM32G03X_SLAVE_ADDRESS     0x56                        /*!< Slave address of the STM32G03X microcontroller in boot mode */

#define BOOT_PIN        12
#define NRST_PIN        14

/**

* @ brief Confugures the corresponding GPIOs as outputs 

* @ param None

* @ return None

*/
void configureBootloaderGPIOsAsOutputs(void);

/**

* @ brief Confugures the corresponding GPIOs as a Hi-Z state 

* @ param None

* @ return None

*/
void configureBootloaderGPIOsAsHiZ(void);
/**

* @ brief Issues reset pulse on the reset pin and BOOT0 pin high to get the stm32 chip into the boot mode

* @ param None

* @ return None

*/
void startBootloaderAccess(void);
/**

* @ brief Issues reset pulse and BOOT0 in low to get the stm32 chip into the FLASH boot mode

* @ param None

* @ return None

*/
void finishBootloaderAccess(void);
/**

* @ brief Initiates the I2C controller.

* @ param from defines - master/slave mode, clock pin number, data pin number, pullups enable/disable, I2C speed

* @ return Returns ESP_OK, ESP_ERR_INVALID_ARG, ESP_FAIL

*/


uint8_t i2c_master_init(void);
/**

* @ brief Issues consequtively: start bit, slave address+write bit, data array, stop bit

* @ param slaveAddress - a 7-bit slave device address
* data_wr - pointer to a byte array to transmit to I2C slave
* size - quantity of the first bytes from the byte array data_wr which will be transmitted to the I2C slave device

* @ return - ESP_OK Success 
*- ESP_ERR_INVALID_ARG Parameter error
*- ESP_FAIL Sending command error, slave doesn't ACK the transfer
*- ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode
*- ESP_ERR_TIMEOUT Operation timeout because the bus is busy
*/
uint8_t  i2cMasterWrite(uint8_t slaveAddress, uint8_t *data_wr, uint16_t size);


/**

* @ brief Issues consequtively: start bit, slave address+read bit, reads out from slave, stop bit

* @ param slaveAddress - a 7-bit slave device address
* data_wr - pointer to a byte array to reveive from I2C slave
* size - quantity of the first bytes of the byte array data_wr which will be written with the data from the I2C slave device

* @ return - ESP_OK Success 
*- ESP_ERR_INVALID_ARG Parameter error
*- ESP_FAIL Sending command error, slave doesn't ACK the transfer
*- ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode
*- ESP_ERR_TIMEOUT Operation timeout because the bus is busy
*/
uint8_t  i2cMasterRead(uint8_t slaveAddress, uint8_t *data_rd, uint16_t size);

/**

* @ brief Reads out 8 bytes starting from the address 0x1FFF7800. Checks for the complimentarcy of the nBOOT_sel bit

* @ param slaveAddress - a 7-bit slave device address

* @ return nBOOT_Sel boolean value
* -1 in case of nBOOT_Sel bit read error

*/
uint8_t readStm_nBOOT_SEL_Bit(uint8_t STMslaveAddress);

/**

* @ brief Reads out 8 bytes starting from the address 0x1FFF7800. Checks for the complimentarcy of the nBOOT_Sel bit.
* Modifies the nBOOT_Sel corresponding bits and writes back 8 bytes starting from the address 0x1FFF7800.

* @ param slaveAddress - a 7-bit slave device address

* @ return nBOOT_Sel boolean value
* -1 in case of nBOOT_Sel bit read error

*/
void writeStm_nBOOT_SEL_Bit(uint8_t STMslaveAddress, uint8_t data);
/**

* @ brief Initiates the write mode for the STM32 chip, writes array of bytes 

* @ param slaveAddress - a 7-bit slave device address
* data - byte array to write to the STM32 FLASH memory

* @ return None

*/
void flashStmFromFile(uint8_t STMslaveAddress, char *fileName);


#endif /* DEVICE_H_ */


