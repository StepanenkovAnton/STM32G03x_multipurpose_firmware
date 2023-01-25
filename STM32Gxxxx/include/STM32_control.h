#ifndef STM32_CONTROL_H_
#define STM32_CONTROL_H_

#include "stdint.h"
#include "esp_log.h"
#include "esp_err.h"

#define PIN_MODE_MASK  0x0F
#define PIN_PP_MASK    0x30
#define PIN_SPEED_MASK 0xC0

#define STM32_GPIO_MODE_INPUT       (0x00000000u)   /*!< Input Floating Mode                   */
#define STM32_GPIO_MODE_OUTPUT_PP   (0x00000001u)   /*!< Output Push Pull Mode                 */
#define STM32_GPIO_MODE_OUTPUT_OD   (0x00000011u)   /*!< Output Open Drain Mode                */
#define STM32_GPIO_MODE_AF_PP       (0x00000002u)   /*!< Alternate Function Push Pull Mode     */
#define STM32_GPIO_MODE_AF_OD       (0x00000012u)   /*!< Alternate Function Open Drain Mode    */
#define STM32_GPIO_MODE_ANALOG      (0x00000003u)   /*!< Analog Mode  */

#define STM32_GPIO_NOPULL           (0x00000000u)   /*!< No Pull-up or Pull-down activation  */
#define STM32_GPIO_PULLUP           (0x00000001u)   /*!< Pull-up activation                  */
#define STM32_GPIO_PULLDOWN         (0x00000002u)   /*!< Pull-down activation                */

#define STM32_GPIO_SPEED_FREQ_LOW     (0x00000000u)  /*!< Low speed       */
#define STM32_GPIO_SPEED_FREQ_MEDIUM  (0x00000001u)  /*!< Medium speed    */
#define STM32_GPIO_SPEED_FREQ_HIGH    (0x00000002u)  /*!< High speed      */
#define STM32_PIO_SPEED_FREQ_VERY_HIGH (0x00000003u)  /*!< Very high speed */


typedef struct {
	/*!
	 * <pre>	
	 * time of fix
	 * format: 
	 * hhmmss		for fix rate = 1Hz or less
	 * hhmmssss		for fix rate > 1Hz 
	 * </pre>
	 */
	uint8_t	UTC_time_hours;
	uint8_t	UTC_time_mimutes;
	uint8_t	UTC_time_ten_thouthandths_of_seconds;
	/*!	 
	 * <pre>
	 * date of fix
	 * </pre>
	 */
	uint8_t			UT_date_month;
	uint8_t			UT_date_day;
	uint8_t			UT_date_year;

	/*!	 
	 * <pre>
	 * latitude of position
	 * </pre>
	 */
	
	uint8_t	latitude_degrees;
	uint8_t	latitude_minutes;
	uint32_t latitude_hundred_thouthandths_of_minutes;

	
	
	/*!	 
	 * <pre>
	 * latitude direction
	 * N(0x00) or S(0x01)
	 * </pre>
	 */
	uint8_t 		latitude_direction;

	/*!	 
	 * <pre>
	 * longitude of position
	 * </pre>
	 */
	uint8_t	longitude_degrees;
	uint8_t	longitude_minutes;
	uint32_t longitude_hundred_thouthandths_of_minutes;


	
	
	
	/*!	 
	 * <pre>
	 * longitude direction
	 * E(0x00) or W(0x01)
	 * </pre>
	 */
	uint8_t 		longitude_direction;
	/*!	 
	 * <pre>
	 * Antenna altitude above mean-sea-level
	 * units of antenna altitude:	meters/10
	 * </pre>
	 */
	uint32_t	tenths_of_altitude;
	/*!	 
	 * <pre>
	 * Geoidal separation
	 * units of geoidal separation:	meters/10
	 * </pre>
	 */
	uint32_t	tenths_of_geoidal_separation;

	uint8_t		sat_in_view;

	uint8_t		sat_in_use;
	
	/*!	 
	 * <pre>
	 * Fix flag 
	 * 0 - Invalid
	 * 1 - GPS fix
	 * </pre>
	 */
	uint8_t		fix;
	/*!	 
	 * <pre>
	 * Fix mode 
	 * 1 - Fix not available
	 * 2 - 2D mode
	 * 3 - 3D mode
	 * </pre>
	 */
	uint8_t		fix_mode;
	/*!	 
	 * <pre>
	 * Position dilution of precision (3D)
	 * units:	meters/10
	 * </pre>
	 */
	uint32_t		tenths_of_PDOP;
	/*!	 
	 * <pre>
	 * Horizontal dilution of precision
	 * units:	meters/10
	 * </pre>
	 */
	uint32_t		tenths_of_HDOP;
	/*!	 
	 * <pre>
	 * Vertical dilution of precision 
	 * units:	meters/10
	 * </pre>
	 */
	uint32_t		tenths_of_VDOP;

}GPS_data;





typedef struct {
    //GPIO_MODE_INPUT       (0x00000000u)   /*!< Input Floating Mode                   */
    //GPIO_MODE_OUTPUT_PP   (0x00000001u)   /*!< Output Push Pull Mode                 */
    //GPIO_MODE_OUTPUT_OD   (0x00000011u)   /*!< Output Open Drain Mode                */
    //GPIO_MODE_AF_PP       (0x00000002u)   /*!< Alternate Function Push Pull Mode     */
    //GPIO_MODE_AF_OD       (0x00000012u)   /*!< Alternate Function Open Drain Mode    */
    //GPIO_MODE_ANALOG      (0x00000003u)   /*!< Analog Mode  */
	uint8_t in_out_analog;

    //GPIO_NOPULL           (0x00000000u)   /*!< No Pull-up or Pull-down activation  */
    //GPIO_PULLUP           (0x00000001u)   /*!< Pull-up activation                  */
    //GPIO_PULLDOWN         (0x00000002u)   /*!< Pull-down activation                */
    uint8_t pull_up_pull_down;

    //GPIO_SPEED_FREQ_LOW     (0x00000000u)  /*!< Low speed       */
    //GPIO_SPEED_FREQ_MEDIUM  (0x00000001u)  /*!< Medium speed    */
    //GPIO_SPEED_FREQ_HIGH    (0x00000002u)  /*!< High speed      */
    //PIO_SPEED_FREQ_VERY_HIGH(0x00000003u)  /*!< Very high speed */
    uint8_t speed_mode;
} STM32PinConfiguration;

/**
 * @brief the slave I2C STM32xxx device's pin mode\n 
 * @param STM32_I2C_Slave_Address I2C device address. Don't use even numbers! STM32 HAL library bug!
 * I know how to fix it but the code will become less portable (corrections to be made are inside HAL 
 * system files).
 * Once ther're enough addresses, so it's ok.
 * @param STM32_port e.g. 0=PORT A, 1=PORT B, etc
 * @param STM32_pin adjustable pin number
 * @param STM32_Pin_Configuration a structured set of STM32 pin configuration variables
 * @return ESP-IDF I2C communication library error code. Correcponds to I2C transaction status.
 * Does not report about pin configuration status inside the STM32 chip.
 */
esp_err_t STM32_setPinConfiguration ( uint8_t STM32_I2C_Slave_Address, uint8_t STM32_port, uint8_t STM32_pin,STM32PinConfiguration STM32_Pin_Configuration);

/**
 * @brief Obsolete! No need to use it. Will be deleted.\n 
 * @param STM32_I2C_Slave_Address I2C device address. Don't use even numbers! STM32 HAL library bug!
 * I know how to fix it but the code will become less portable (corrections to be made are inside HAL 
 * system files).
 * Once ther're enough addresses, so it's ok.
 * @return ESP-IDF I2C communication library error code. Correcponds to I2C transaction status.
 * Does not report about pin configuration status inside the STM32 chip.
 */
esp_err_t STM32_Configure_AnalogChannel (uint8_t STM32_I2C_Slave_Address, uint8_t ADC_Instance_Number, uint8_t ADC_Channel_Number);
/**
 * @brief the slave I2C STM32xxx device's pin output state\n 
 * @param STM32_I2C_Slave_Address I2C device address. Don't use even numbers! STM32 HAL library bug!
 * I know how to fix it but the code will become less portable (corrections to be made are inside HAL 
 * system files).
 * Once ther're enough addresses, so it's ok.
 * @param STM32_port e.g. 0=PORT A, 1=PORT B, etc
 * @param STM32_pin adjustable pin number
 * @param STM32_pin_state High/Low command
 * @return ESP-IDF I2C communication library error code. Correcponds to I2C transaction status.
 * Does not report about pin configuration status inside the STM32 chip.
 */
esp_err_t STM32_digitalWrite ( uint8_t STM32_I2C_Slave_Address, uint8_t STM32_port, uint8_t STM32_pin, uint8_t STM32_pin_state );
/**
 * @brief the slave I2C STM32xxx device's input pin state\n 
 * @param STM32_I2C_Slave_Address I2C device address. Don't use even numbers! STM32 HAL library bug!
 * I know how to fix it but the code will become less portable (corrections to be made are inside HAL 
 * system files).
 * Once ther're enough addresses, so it's ok.
 * @param STM32_port e.g. 0=PORT A, 1=PORT B, etc
 * @param STM32_pin adjustable pin number
 * @return STM32_pin_state High/Low report
 * @return ESP-IDF I2C communication library error code. Correcponds to I2C transaction status.
 * Does not report about pin configuration status inside the STM32 chip.
 */
esp_err_t STM32_digitalRead ( uint8_t STM32_I2C_Slave_Address, uint8_t STM32_port, uint8_t STM32_pin, uint8_t *STM32_pin_state);
/**
 * @brief the slave I2C STM32xxx analog channel reading. You can request any of 24 channels.
 * If the corresponding pin is not configured as analog input, the function will return analog value 
 * anyway. Ex. digital output pin at HIGH state may return 4095 or a bit less; digital input pin at a low signal applied
 * will return something a bit above 0 (some DC offset).\n 
 * @param STM32_I2C_Slave_Address I2C device address. Don't use even numbers! STM32 HAL library bug!
 * I know how to fix it but the code will become less portable (corrections to be made are inside HAL 
 * system files).
 * Once ther're enough addresses, so it's ok.
 * @param ADC_instance 0-ADC#1, 1-ADC#2, etc.
 * @param ADC_channel 0-channel#0, 1-channel#1, etc. See STM32 datasheet for channels-to-pins match.
 * If the channel number is not supported by the chip so the channel#0 value will be returned.
 * @return STM32_ADC_reading a pointer to the ADC value variable.
 * @return ESP-IDF I2C communication library error code. Correcponds to I2C transaction status.
 * Does not report about pin configuration status inside the STM32 chip.
 */
esp_err_t STM32_analogRead ( uint8_t STM32_I2C_Slave_Address, uint8_t ADC_instance, uint8_t ADC_channel, uint16_t *STM32_ADC_reading );

/**
 * @brief Requests the GPS data\n 
 * @param STM32_I2C_Slave_Address I2C device address. Don't use even numbers! STM32 HAL library bug!
 * I know how to fix it but the code will become less portable (corrections to be made are inside HAL 
 * system files).
 * Once ther're enough addresses, so it's ok.
 * @return GPS_output_data a pointer to the GPS data structured variables array.
 * @return ESP-IDF I2C communication library error code. Correcponds to I2C transaction status.
 * Does not report about pin configuration status inside the STM32 chip.
 */
esp_err_t STM32_GPS_data_Read ( uint8_t STM32_I2C_Slave_Address, GPS_data *GPS_output_data );

/**
 * @brief Sets the RTC date and time.\n 
 * The data validity check is done on the STM32 side (leap year correction)
 * @param STM32_I2C_Slave_Address I2C device address. Don't use even numbers! STM32 HAL library bug!
 * @param year 1
 * @param month
 * @param day
 * @param hour 24-hour format
 * @param min
 * @param sec
 * @param dow the day of the week
 * @return ESP-IDF I2C communication library error code. Correcponds to I2C transaction status.
 * Does not report about pin configuration status inside the STM32 chip.
 */
esp_err_t STM32_RTC_Set (uint8_t STM32_I2C_Slave_Address,uint8_t year, uint8_t month, 
						uint8_t day,uint8_t hour, uint8_t min, 
						uint8_t sec, uint8_t dow);


/**
 * @brief Sets the RTC date and time.\n 
 * The data validity check is done on the STM32 side (leap year correction)
 * @param STM32_I2C_Slave_Address I2C device address. Don't use even numbers! STM32 HAL library bug!
 * @return year 1
 * @return month
 * @return day
 * @return hour 24-hour format
 * @return min
 * @returnsec
 * @return dow the day of the week
 * @return ESP-IDF I2C communication library error code. Correcponds to I2C transaction status.
 * Does not report about pin configuration status inside the STM32 chip.
 */
esp_err_t STM32_RTC_Get (uint8_t STM32_I2C_Slave_Address,uint8_t *year, uint8_t *month, 
						uint8_t *day,uint8_t *hour, uint8_t *min, 
						uint8_t *sec, uint8_t *dow);


/**
 * @brief Resets the stm32 for bootloader purposes.\n 
 * @param STM32_I2C_Slave_Address I2C device address. Don't use even numbers! STM32 HAL library bug!
 */
esp_err_t STM32_Reset (uint8_t STM32_I2C_Slave_Address);
#endif /* STM32_CONTROL_H_ */