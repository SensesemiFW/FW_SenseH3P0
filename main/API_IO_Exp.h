#ifndef _API_IO_EXP_H_
#define _API_IO_EXP_H_

#include <stdint.h>
#include <esp_err.h>
#include <stdbool.h>

#define REG_ADDR_INPUT_PORT_0   0x00U
#define REG_ADDR_INPUT_PORT_1   0x01U
#define REG_ADDR_OUTPUT_PORT_0   0x02U
#define REG_ADDR_OUTPUT_PORT_1   0x03U
#define REG_ADDR_CONFIGURATION_PORT_0 0x06U
#define REG_ADDR_CONFIGURATION_PORT_1 0x07U
#define REG_ADDR_INPUT_PORT0_LATCH 0x44U
#define REG_ADDR_INPUT_PORT1_LATCH 0x45U
#define REG_PULLUP_PULLDOWN_EN_0  0x46U
#define REG_PULLUP_PULLDOWN_EN_1  0x47U
#define REG_PULLUP_PULLDOWN_SEL_0 0x48U
#define REG_PULLUP_PULLDOWN_SEL_1 0x49U



#define ALL_PINS_HIGH 0xFFU
#define ALL_PINS_LOW  0x00U


typedef enum{
	PSU_MODE,
	HIBERNATE, // NC
	TP12,
	EN_VLED,
	EN_ANALOG,
	EN_IR,
	EFM_DISP_EN2,
	BUZZER_EN
}IO_Exp1_P0_t;

typedef enum{
	EFM_DISP_RESN,
	EFM_DISP_EN1,
	NOTIFICATION_LED,
	DC_LEAD_OFF_V,
	ECG_RESETN,
	TP11,// NC
	ECG_CSN,
	DISPLAY_CSN

}IO_Exp1_P1_t;



typedef enum{
	TP1,
	USB_5V_SENSE,
	TP2,
	ECG12_A0,
	ECG12_A1,
	ECG12_A2,
	BAT_CHARGE,
	TP3

}IO_Exp2_P0_t;


typedef enum{
	TP4,
	TP5,
	TP6,
	ECG_ALARM,
	TP7,
	DC_LEAD_OFF,
	TP8,
	TP9
}IO_Exp2_P1_t;

typedef enum
{
	LOW,
	HIGH
}VoltageLevel_t;


typedef enum
{
	IO_EXPANDER_1,
	IO_EXPANDER_2,
}IOExpanderNumber_t;


extern uint8_t  SelectedIoExpander;

void API_IO_Exp_init(void);
void API_IO_Exp_Deinit(void);
void API_IO_Exp_Read_reg(uint8_t address, uint8_t cmd_byte,uint8_t *data_buff);
void API_IO_Exp_Write_Reg(uint8_t address, uint8_t cmd_byte, uint8_t value[],uint8_t len);
uint8_t API_IO_Exp_Test(void);
void API_IO_Exp_Select(IOExpanderNumber_t ioExpNumber);

VoltageLevel_t API_IO_Exp1_P0_Read_pin(IO_Exp1_P0_t pin,bool read_again);
VoltageLevel_t API_IO_Exp1_P1_Read_pin(IO_Exp1_P1_t pin,bool read_again);

bool API_IO_Exp1_P0_write_pin(IO_Exp1_P0_t pin,VoltageLevel_t vl);
bool API_IO_Exp1_P1_write_pin(IO_Exp1_P1_t pin,VoltageLevel_t vl);


VoltageLevel_t API_IO_Exp2_P0_Read_pin(IO_Exp2_P0_t pin,bool read_again);
VoltageLevel_t API_IO_Exp2_P1_Read_pin(IO_Exp2_P1_t pin,bool read_again);

bool API_IO_Exp2_P0_write_pin(IO_Exp2_P0_t pin,VoltageLevel_t vl);
bool API_IO_Exp2_P1_write_pin(IO_Exp2_P1_t pin,VoltageLevel_t vl);

void IO_Exp_pin_read();
void Test_i2c_io_exp(void);


void API_IO_Exp_Power_Control(IO_Exp1_P0_t power_ctrl_pin_num,VoltageLevel_t enable_or_disable);
bool Self_IO_Exp_test(void);

void Power_Down_All_Modules(void);
void Power_Up_All_Modules(void);
void API_IO_Exp_Reset(void);

bool IsUSB_Charger_Connected(void);

void API_IO_Exp_Configure_All_InputPins_Disable_pullups(void);
void API_IO_Exp_Configure_All_PinsOut_DriveLow(void);

void API_IO_Exp_PowerOnReset_Configuration(void);

void API_IO_Exp_Soft_POR_Defualt_Register_Setup(void);


#endif
