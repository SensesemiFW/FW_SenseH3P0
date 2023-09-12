#ifndef _HARDWARE_H_
#define _HARDWARE_H_


#define ESP32_MCU_DRDY_PIN   39

#define DISP_MOSI   26
#define DISP_MISO   2
#define DISP_CLK    19

#define IO_EXP_I2C_PORT_NUMBER 0
#define IO_EXP_I2C_MASTER_SCL_PIN 22
#define IO_EXP_I2C_MASTER_SDA_PIN 21
#define IO_EXP_I2C_MASTER_CLOCK_FREQ_HZ 400000
#define IO_EXP1_SLAVE_ADDR  0x44
#define IO_EXP2_SLAVE_ADDR  0x46

#define MAX86150_I2C_PORT_NUMBER  1
#define MAX86150_DRDY_INTR_PIN   25// 4
#define MAX86150_ADDR  0xBC
#define MAX86150_I2C_MASTER_CLOCK_FREQ_HZ 10000U
#define MAX86150_I2C_MASTER_SCL_PIN  8U
#define MAX86150_I2C_MASTER_SDA_PIN  7U

#define PUSH_BTN_PIN     36U



#endif