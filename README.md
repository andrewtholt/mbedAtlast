# mbedAtlast

Profiles are in ./mbed-os/tools/profiles

```C++
// mosi, miso, sck, cs
blockDevice = new SDBlockDevice (PA_7, PA_6, PA_5, PA_8);
```

On Nucleo-F411RE

| Signal | Pin |
|----|----|
|mosi |PA_7|	
|miso| PA_6|
|sck |PA_5|
|cs |PA_8|

on STM32f4-discovery
| Signal | Pin |
|----|----|
|SD_MOSI|PC_3|   
|SD_MISO|PC_2|    
|SD_SCK|PB_10|    
|SD_CS|PE_2|
    
    // Generic signals namings    
    LED1        = PA_5,    
    LED2        = PA_5,    
    LED3        = PA_5,    
    LED4        = PA_5,    
    LED_RED     = LED1,    
    USER_BUTTON = PC_13,    
    // Standardized button names    
    BUTTON1 = USER_BUTTON,    
    SERIAL_TX   = STDIO_UART_TX,    
    SERIAL_RX   = STDIO_UART_RX,    
    USBTX       = STDIO_UART_TX,    
    USBRX       = STDIO_UART_RX,    
    I2C_SCL     = PB_8,       
    I2C_SDA     = PB_9,    
    SPI_MOSI    = PA_7,    
    SPI_MISO    = PA_6,    
    SPI_SCK     = PA_5,    
    SPI_CS      = PB_6,    
    PWM_OUT     = PB_3,  




