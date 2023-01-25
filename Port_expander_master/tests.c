#include "tests.h"

/****************************************************************************************/
/*                                                                                      */
/*                           Static functions declarations                              */
/*                                                                                      */
/****************************************************************************************/

static void runGpioConfigTest( I2C_HandleTypeDef *i2cHandle );

static void runGpioReadWriteTest( I2C_HandleTypeDef *i2cHandle );

static void runRtcReadWriteTest( I2C_HandleTypeDef *i2cHandle );

/****************************************************************************************/
/*                                                                                      */
/*                       Local types and variables declarations                         */
/*                                                                                      */
/****************************************************************************************/

uint8_t i2cReceiveData[6] = { 0 };
uint8_t i2cTransmitData[8] = { 0 };

/****************************************************************************************/
/*                                                                                      */
/*                          Global functions implementations                            */
/*                                                                                      */
/****************************************************************************************/

/*!
 * @brief Performs the given test.
 *
 * @param test_id - name of the test, that will be run;
 * @param i2cHandle - pointer to the handle of the master I2C that is connected to the slave device.
 *
 * All new tests must be called from this functions.
 * It is better to implement tests as separate static functions. Every function must contain description of what behaviour
 * can be expected from the slave device.
 */
void runTest( test_names test_id, I2C_HandleTypeDef *i2cHandle )
{
    switch( test_id )
    {
        case TEST_IO_CONFIG:
            runGpioConfigTest( i2cHandle );
            break;

        case TEST_IO_READ_WRITE:
            runGpioReadWriteTest( i2cHandle );
            break;

        case TEST_RTC_READ_WRITE:
            runRtcReadWriteTest( i2cHandle );
            break;

        default:
            break;
    }
}

/****************************************************************************************/
/*                                                                                      */
/*                          Static functions implementations                            */
/*                                                                                      */
/****************************************************************************************/

/*!
 * @brief Tests whether individual GPIO configure and write functions are working.
 *
 * If everything works correctly, PA5 on the slave will be set up as GPIO and it will be pulled high.
 * If LED is connected to the PA5 like on the Nucleo board, it should light up.
 *
 * All tests use HAL_MAX_DELAY, so if communication error occur, program will get stuck.
 */
static void runGpioConfigTest( I2C_HandleTypeDef *i2cHandle )
{
    i2cTransmitData[0] = 0x10; // GPIO configure function byte
    i2cTransmitData[1] = 0x05; // Port A 5
    i2cTransmitData[2] = GPIO_MODE_OUTPUT_PP; // General gpio push pull mode

    HAL_I2C_Master_Transmit( i2cHandle, 0x77 << 1, i2cTransmitData, 3, HAL_MAX_DELAY ); // transmit setup data

    HAL_Delay(100);

    i2cTransmitData[0] = 0x11; // GPIO output function byte
    i2cTransmitData[1] = 0x05; // Port A 5
    i2cTransmitData[2] = 1; // turn on

    HAL_I2C_Master_Transmit( i2cHandle, 0x77 << 1, i2cTransmitData, 3, HAL_MAX_DELAY ); // transmit setup data
}

/*!
 * @brief Tests whether individual GPIO read and write functions are working.
 *
 * It is considered, that GPIO configure function is working (runGpioConfigTest was successful).
 * Function sets up slaves PA5 as output. After that PA5 toggles every 500ms and current value of the GPIO is read back to master.
 * If LED is connected to the PA5 like on the Nucleo board, it should be blinking with 1s period.
 * Read values can be observed from the i2cTransmitData using live expressions in Debug.
 *
 * This test uses an infinite loop, so control will not reach out of that function.
 */
static void runGpioReadWriteTest( I2C_HandleTypeDef *i2cHandle )
{
    i2cTransmitData[0] = 0x10; // GPIO configure function byte
    i2cTransmitData[1] = 0x05; // Port A 5
    i2cTransmitData[2] = GPIO_MODE_OUTPUT_PP; // General gpio push pull mode

    HAL_I2C_Master_Transmit( i2cHandle, 0x77 << 1, i2cTransmitData, 3, HAL_MAX_DELAY ); // transmit setup data

    i2cTransmitData[0] = 0x11; // GPIO output function byte
    i2cTransmitData[1] = 0x05; // Port A 5
    i2cTransmitData[2] = 1; // turn on

    while( 1 )
    {
        HAL_I2C_Master_Transmit( i2cHandle, 0x77 << 1, i2cTransmitData, 3, HAL_MAX_DELAY );
        i2cTransmitData[2] = !i2cTransmitData[2];
        HAL_Delay( 500 );

        // 0x1205 - individual GPIO input data on PA5.
        HAL_I2C_Mem_Read( i2cHandle, 0x77 << 1, 0x1205, 2, i2cReceiveData, 1, HAL_MAX_DELAY );
        HAL_Delay( 10 );
    }
}

/*!
 * @brief Sets up slave RTC and continuously reads back its values.
 *
 * Functions writes none default RTC values to slave. After that it reads values back every second.
 * Read values can be observed from the i2cTransmitData using live expressions in Debug. It should be expected, that
 * clocks will work, thus every second master will receive a slightly different value of time.
 *
 * This test uses an infinite loop, so control will not reach out of that function.
 */
static void runRtcReadWriteTest( I2C_HandleTypeDef *i2cHandle )
{
    i2cTransmitData[0] = 0x50;  // RTC date and time.
    i2cTransmitData[1] = 0;     // Empty byte for protocol consistency.
    i2cTransmitData[2] = 21;    // Day
    i2cTransmitData[3] = 9;     // Month
    i2cTransmitData[4] = 14;    // Year
    i2cTransmitData[5] = 8;     // Hour
    i2cTransmitData[6] = 7;     // Minute
    i2cTransmitData[7] = 35;    // second

    HAL_I2C_Master_Transmit( i2cHandle, 0x77 << 1, i2cTransmitData, 8, HAL_MAX_DELAY );

    while( 1 )
    {
        HAL_Delay( 1000 );
        HAL_I2C_Mem_Read( i2cHandle, 0x77 << 1, 0x5000, 2, i2cReceiveData, 6, HAL_MAX_DELAY );
    }
}

// EOF
