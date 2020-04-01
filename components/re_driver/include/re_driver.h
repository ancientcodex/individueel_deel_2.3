#ifndef RE_H
#define RE_H

#ifdef __cplusplus
extern "C"
{
#endif
// Error library
#include "esp_err.h"
// I2C driver
#include "driver/i2c.h"
// FreeRTOS (for delay)
#include "freertos/task.h"

#define RE_I2C_ADDR 0x3F

    typedef enum
    {
        RE_ERR_OK = 0x00,
        RE_ERR_CONFIG = 0x01,
        RE_ERR_INSTALL = 0x02,
        RE_ERR_FAIL = 0x03
    } re_driver_err_t;

#ifndef WRITE_BIT
#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
#endif

#ifndef READ_BIT
#define READ_BIT I2C_MASTER_READ /*!< I2C master read */
#endif

// bit to enable checking for ACK
#ifndef ACK_CHECK_EN
#define ACK_CHECK_EN 0x1
#endif

    typedef enum
    {
        INT_ENABLE = 0x04,        
        LED_BRIGHTNESS_RED = 0x0D, 
        LED_BRIGHTNESS_GRN = 0x0E, 
        LED_BRIGHTNESS_BLU = 0x0F, 
        STAT = 0x01,               
        FW_VERSION_MSB = 0x02,     
        FW_VERSION_LSB = 0x03,
        RE_COUNT_LSB = 0x05,
        RE_COUNT_MSB = 0x06,
        RE_DIFF_LSB = 0x07,
        RE_DIFF_MSB = 0x08,
        RE_TSLM_LSB = 0x09,
        RE_TSLM_MSB = 0x0A,
        RE_TSLB_LSB = 0x0B,
        RE_TSLB_MSB = 0x0C,
        LED_CON_RED_LSB = 0x10,
        LED_CON_RED_MSB = 0x11,
        LED_CON_GRN_LSB = 0x12,
        LED_CON_GRN_MSB = 0x13,
        LED_CON_BLU_LSB = 0x14,
        LED_CON_BLU_MSB = 0x15,
        TRN_INT_TO_LSB = 0x16,
        TRN_INT_TO_MSB = 0x17,
    } RE_reg_t;

    typedef struct
    {
        uint8_t i2c_address;
        i2c_port_t port;
        uint8_t sda_pin;
        uint8_t scl_pin;
    } RE_t;

    bool re_driver_is_connected(RE_t *RE);
    re_driver_err_t re_driver_initialize(RE_t *RE);
    // check if RE is Connected
    bool re_driver_isConnected(RE_t *RE);
    //Status of the Qwiic Twist. 3:0 = buttonPressed(3), buttonClicked(2), buttonInterrupt(1), encoderInterrupt(0).
    re_driver_err_t re_driver_getStatus(RE_t *RE, uint8_t *data);
    //Set the bit to 1 to enable the interrupt for the button or encoder. buttonInterruptEnable(1), encoderInterruptEnable(0). Default is 0x03
    re_driver_err_t re_driver_interruptEnable(RE_t *RE, bool v);
    //Value between 0 and 255 representing the brightness of the red LED. Default is 255
    re_driver_err_t re_driver_setColorRed(RE_t *RE, uint8_t v);
    //Value between 0 and 255 representing the brightness of the green LED. Default is 255
    re_driver_err_t re_driver_setColorGrn(RE_t *RE, uint8_t v);
    //Value between 0 and 255 representing the brightness of the blue LED. Default is 255
    re_driver_err_t re_driver_setColorBlu(RE_t *RE, uint8_t v);
    //Value between 255 and -255 indicating the amount to change the red LED brightness with each tick movement of the encoder. Default is 0
    re_driver_err_t re_driver_conectColorRed(RE_t *RE, uint16_t v);
    //Value between 255 and -255 indicating the amount to change the green LED brightness with each tick movement of the encoder. Default is 0
    re_driver_err_t re_driver_conectColorGrn(RE_t *RE, uint16_t v);
    //Value between 255 and -255 indicating the amount to change the blue LED brightness with each tick movement of the encoder. Default is 0
    re_driver_err_t re_driver_conectColorBlu(RE_t *RE, uint16_t v);
    //Value between 32767 and -32768 indicating the number of ticks the user has twisted the knob. 24 ticks per rotation.
    re_driver_err_t re_driver_getCount(RE_t *RE, uint16_t *data);
    //Value between 32767 and -32768 indicating the number of ticks the user has twisted the knob since last movement.
    re_driver_err_t re_driver_getDiff(RE_t *RE, uint16_t *data);
    //Value between 0 and 65535 indicating the number of milliseconds since last movement.
    re_driver_err_t re_driver_getTSLM(RE_t *RE, uint16_t *data);
    //Value between 0 and 65535 indicating the number of milliseconds since last press/release event.
    re_driver_err_t re_driver_getTSLB(RE_t *RE, uint16_t *data);

#ifdef __cplusplus
}
#endif

#endif
