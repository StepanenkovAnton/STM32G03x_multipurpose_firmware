#ifndef DEV_TESTS_H_
#define DEV_TESTS_H_

#include "main.h"

/****************************************************************************************/
/*                                                                                      */
/*                                        ENUMS                                         */
/*                                                                                      */
/****************************************************************************************/

/*!
 * @brief Contains names of all available master based tests.
 *
 * All new test indexes must be added here and implemented in the runTest function.
 */
typedef enum test_names
{
    TEST_IO_CONFIG = 0,
    TEST_IO_READ_WRITE = 1,
    TEST_RTC_READ_WRITE = 2,
} test_names;

/****************************************************************************************/
/*                                                                                      */
/*                           Global functions declarations                              */
/*                                                                                      */
/****************************************************************************************/

//! Performs the given test.
void runTest( test_names test_id, I2C_HandleTypeDef *i2cHandle );

#endif /* DEV_TESTS_H_ */
