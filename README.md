# STM32G03x_multipurpose_firmware
STM32 ADC+NMEA decoding+ESP32&lt;->STM32 I2C comminication implementation
STM32G03x controller receives NMEA packets from UART, decodes GPS coordinates, performs ADC readings, provides digital outputs. STM32 chips acts as a I2C slave frontend extention for ESP32 I2C master device. 
ESP32 reads data from STM32 via I2C and can reflash STM32 using factory SM32 I2C bootloader when needed.
Thus the STM32G03x chip acts like an I/O ports extention for ESP32.
