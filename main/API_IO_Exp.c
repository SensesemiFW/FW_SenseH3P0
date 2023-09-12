
#include "max86150.h"
#include "stdint.h"
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include <string.h>

#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "API_timer.h"
#include "API_IO_Exp.h"
#include "Hardware.h"
#include "Error_Handling.h"


#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define DATA_LENGTH 512                  /*!< Data buffer length of test buffer */
#define RW_TEST_LENGTH 128               /*!< Data length for r/w test, [0,DATA_LENGTH] */
#define DELAY_TIME_BETWEEN_ITEMS_MS 1000 /*!< delay time between different test items */


#define I2C_MASTER_NUM I2C_NUMBER(CONFIG_I2C_MASTER_PORT_NUM) /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0


#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1

uint8_t  SelectedIoExpander;

static void print_pin_satus(VoltageLevel_t pin_status , char tag[])
{
	if(pin_status == LOW)
	{
		//printf("\n%s=LOW",tag);
	}
	else if(pin_status == HIGH)
	{
		//printf("\n%s=HIGH",tag);

	}
}

void API_IO_Exp_Select(IOExpanderNumber_t ioExpNumber)
{
	uint8_t port_data[2] = {0x00,0x00};

	port_data[0] = 0;

	if(ioExpNumber == IO_EXPANDER_1){
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x4F,port_data,2); // pull up config
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x06,port_data,2); // output port

	}

	else if(ioExpNumber == IO_EXPANDER_1)
	{
		API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x4F,port_data,2); // pull up config
		API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x06,port_data,2); // output port

	}

}

void API_IO_Exp_Write_Reg(uint8_t address, uint8_t cmd_byte, uint8_t value[],uint8_t len)
{
	esp_err_t error;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, address | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, cmd_byte, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, value[0], ACK_CHECK_EN);
	if(len>1){
		i2c_master_write_byte(cmd, value[1], ACK_CHECK_EN);
	}

	i2c_master_stop(cmd);
	error = i2c_master_cmd_begin(IO_EXP_I2C_PORT_NUMBER, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	if(error != ESP_OK) Catch_RunTime_Error(IO_EXPANDER_REG_WRITE_FAIL);

}


void API_IO_Exp_Read_reg(uint8_t address, uint8_t cmd_byte,uint8_t *data_buff)
{
		esp_err_t error;

	    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, address | WRITE_BIT, ACK_CHECK_EN);
		i2c_master_write_byte(cmd, cmd_byte, ACK_CHECK_EN);
		i2c_master_start(cmd);

		i2c_master_write_byte(cmd, address | READ_BIT, ACK_CHECK_EN);

		i2c_master_read_byte(cmd,data_buff, ACK_CHECK_EN);

		i2c_master_stop(cmd);
		error = i2c_master_cmd_begin(IO_EXP_I2C_PORT_NUMBER, cmd, 1000 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);

		if(error != ESP_OK) Catch_RunTime_Error(IO_EXPANDER_REG_READ_FAIL);

}

uint8_t API_IO_Exp_Test(void)
{
	uint8_t port_data[2] = {0x00,0x00};
	uint8_t read_port=0;

//	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x44,port_data);
	//API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x2,port_data,2);



	port_data[0] = 0;

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x4F,port_data,1); // pull up config

	port_data[0] = 0;// output
		port_data[1] = 0; //output

		API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x06,port_data,2); // output port

	port_data[0] = 0xFF;
	port_data[1] = 0x01; // disp config

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x2,port_data,2);


    while(1){
    API_IO_Exp_Read_reg(IO_EXP1_SLAVE_ADDR, 0x00,&read_port);

    //printf("\n Read value: P0=%X",read_port);
    read_port = 0;
    API_IO_Exp_Read_reg(IO_EXP1_SLAVE_ADDR, 0x01,&read_port);
    //printf(" P1=%X",read_port);

    read_port = 0;
    }

    return true;
}




void API_IO_Exp_init(void)
{
	esp_err_t error;

   int i2c_master_port = IO_EXP_I2C_PORT_NUMBER;
   i2c_config_t conf;

   conf.mode = I2C_MODE_MASTER;
   conf.sda_io_num = IO_EXP_I2C_MASTER_SDA_PIN;
   conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
   conf.scl_io_num = IO_EXP_I2C_MASTER_SCL_PIN;
   conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
   conf.master.clk_speed = IO_EXP_I2C_MASTER_CLOCK_FREQ_HZ;
   conf.clk_flags = 0;
   i2c_param_config(i2c_master_port, &conf);

   error = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

	// i2c_filter_enable(i2c_master_port,1);

   if(error != ESP_OK) Catch_RunTime_Error(IO_EXPANDER_INIT_FAIL);

}

void API_IO_Exp_Deinit(void)
{
	 i2c_driver_delete(IO_EXP_I2C_PORT_NUMBER);
	 //i2c_set_pin(IO_EXP_I2C_PORT_NUMBER,IO_EXP_I2C_MASTER_SDA_PIN,IO_EXP_I2C_MASTER_SCL_PIN,
	                     // 1, 1, I2C_MODE_MASTER);
}


VoltageLevel_t API_IO_Exp1_P0_Read_pin(IO_Exp1_P0_t pin,bool read_again)
{
	uint8_t read_port=0;
	VoltageLevel_t pin_vol = LOW;

	if(SelectedIoExpander == 2)
	{
		API_IO_Exp_Select(IO_EXPANDER_1);
		SelectedIoExpander = 1;
	}

	if(read_again){
    API_IO_Exp_Read_reg(IO_EXP1_SLAVE_ADDR, 0x02,&read_port);
   // //printf("\nReag = %X",read_port);
	}

	uint8_t mask = read_port >> pin;
	mask &= 0x01;
	////printf("\n Mask = 0x%X",mask);

    if(mask)
    {
    	pin_vol = HIGH;
    }

    return pin_vol;
}


VoltageLevel_t API_IO_Exp1_P1_Read_pin(IO_Exp1_P1_t pin,bool read_again)
{
	uint8_t read_port=0;
	VoltageLevel_t pin_vol = LOW;

	if(SelectedIoExpander == 2)
		{
			API_IO_Exp_Select(IO_EXPANDER_1);
			SelectedIoExpander = 1;
		}


	if(read_again){
    API_IO_Exp_Read_reg(IO_EXP1_SLAVE_ADDR, 0x03,&read_port);
   // //printf("\nReag = %X",read_port);
	}

	uint8_t mask = read_port >> pin;
	mask &= 0x01;
	////printf("\n Mask = 0x%X",mask);

    if(mask)
    {
    	pin_vol = HIGH;
    }

    return pin_vol;
}







bool API_IO_Exp1_P0_write_pin(IO_Exp1_P0_t pin,VoltageLevel_t vl)
{
	uint8_t read = 0;
	uint8_t modify_read = 0;
	uint8_t temp = 0;

	if(SelectedIoExpander == 2)
		{
			API_IO_Exp_Select(IO_EXPANDER_1);
			SelectedIoExpander = 1;
		}
    API_IO_Exp_Read_reg(IO_EXP1_SLAVE_ADDR, 0x02,&read);

    ////printf("\n vl=%d",vl);

    if(vl == HIGH){
    temp =  (1<<pin);
    modify_read = read | temp;

   // //printf("\npin = %d read = %d  modify_read=%d",pin,read,modify_read);

    }

    else{
       temp =  (1<<pin);

       modify_read = read & (~temp);

     //  //printf("\nvl = %d pin = %d read = %d  modify_read=%d",vl,pin,read,modify_read);

       }

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x2,&modify_read,1);

	return true;
}

bool API_IO_Exp1_P1_write_pin(IO_Exp1_P1_t pin,VoltageLevel_t vl)
{
	uint8_t read = 0;
	uint8_t modify_read = 0;
	uint8_t temp = 0;

	if(SelectedIoExpander == 2)
		{
			API_IO_Exp_Select(IO_EXPANDER_1);
			SelectedIoExpander = 1;
		}
    API_IO_Exp_Read_reg(IO_EXP1_SLAVE_ADDR, 0x03,&read);

    if(vl == HIGH){
    temp =  (1<<pin);
    modify_read = read | temp;

    }

    else if (vl == LOW){
       temp =  (1<<pin);
       temp = temp ^ 0xFF;

       modify_read = read & temp;

       }

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x3,&modify_read,1);

	return true;

}


VoltageLevel_t API_IO_Exp2_P0_Read_pin(IO_Exp2_P0_t pin,bool read_again)
{

	uint8_t read_port=0;
	VoltageLevel_t pin_vol = LOW;

	if(SelectedIoExpander == 1)
		{
			API_IO_Exp_Select(IO_EXPANDER_2);
			SelectedIoExpander = 2;
		}

	if(read_again){
    API_IO_Exp_Read_reg(IO_EXP2_SLAVE_ADDR, 0x02,&read_port);
   // //printf("\nReag = %X",read_port);
	}

	uint8_t mask = read_port >> pin;
	mask &= 0x01;
	////printf("\n Mask = 0x%X",mask);

    if(mask)
    {
    	pin_vol = HIGH;
    }

    return pin_vol;

}

VoltageLevel_t API_IO_Exp2_P1_Read_pin(IO_Exp2_P1_t pin,bool read_again)
{

	uint8_t read_port=0;
	VoltageLevel_t pin_vol = LOW;

	if(SelectedIoExpander == 1)
		{
			API_IO_Exp_Select(IO_EXPANDER_2);
			SelectedIoExpander = 2;
		}


	if(read_again){
    API_IO_Exp_Read_reg(IO_EXP2_SLAVE_ADDR, 0x03,&read_port);
   // //printf("\nReag = %X",read_port);
	}

	uint8_t mask = read_port >> pin;
	mask &= 0x01;
	////printf("\n Mask = 0x%X",mask);

    if(mask)
    {
    	pin_vol = HIGH;
    }

    return pin_vol;

}

bool API_IO_Exp2_P0_write_pin(IO_Exp2_P0_t pin,VoltageLevel_t vl)
{

	uint8_t read = 0;
	uint8_t modify_read = 0;
	uint8_t temp = 0;

	if(SelectedIoExpander == 1)
		{
			API_IO_Exp_Select(IO_EXPANDER_2);
			SelectedIoExpander = 2;
		}

    API_IO_Exp_Read_reg(IO_EXP2_SLAVE_ADDR, 0x02,&read);

    if(vl == HIGH){
    temp =  (1<<pin);
    modify_read = read | temp;

    }

    else if (vl == LOW){
       temp =  (1<<pin);
       temp = temp ^ 0xFF;

       modify_read = read & temp;

       }

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x2,&modify_read,1);

	return true;

}

bool API_IO_Exp2_P1_write_pin(IO_Exp2_P1_t pin,VoltageLevel_t vl)
{

	uint8_t read = 0;
	uint8_t modify_read = 0;
	uint8_t temp = 0;


	if(SelectedIoExpander == 1)
		{
			API_IO_Exp_Select(IO_EXPANDER_2);
			SelectedIoExpander = 2;
		}

    API_IO_Exp_Read_reg(IO_EXP2_SLAVE_ADDR, 0x03,&read);

    if(vl == HIGH){
    temp =  (1<<pin);
    modify_read = read | temp;

    }

    else if (vl == LOW){
       temp =  (1<<pin);
       temp = temp ^ 0xFF;

       modify_read = read & temp;

       }

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x3,&modify_read,1);

	return true;

}


void IO_Exp_pin_read()
{
	VoltageLevel_t vol_level;
	//printf("IO Exp testing\n");

	API_IO_Exp_Select(IO_EXPANDER_1);

	while(1){

		/*API_IO_Exp1_P0_write_pin(PSU_MODE,HIGH);
		API_IO_Exp1_P1_write_pin(EFM_DISP_RESN,HIGH);


		API_IO_Exp2_P0_write_pin(USB_5V_SENSE,LOW);
		API_IO_Exp2_P1_write_pin(ECG_ALARM,LOW);
*/

		vol_level = API_IO_Exp1_P0_Read_pin(BUZZER_EN,true);
		print_pin_satus(vol_level , "BUZZER_EN");

		/*vol_level = API_IO_Exp1_P1_Read_pin(DISPLAY_CSN,true);
		print_pin_satus(vol_level , "BUZZER_EN");

		vol_level = API_IO_Exp2_P0_Read_pin(USB_5V_SENSE,true);
		print_pin_satus(vol_level , "USB_5V_SENSE");

		vol_level = API_IO_Exp2_P1_Read_pin(ECG_ALARM,true);*/
		//print_pin_satus(vol_level , "BUZZER_EN");

	}
	 //printf("\n Done");

}


void uid_buzzer_test(void)
{

}


void Test_i2c_io_exp(void)
{

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);

	i2c_master_write_byte(cmd, 0x11, ACK_CHECK_EN); // 0x44 Send continiously

	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(IO_EXP_I2C_PORT_NUMBER, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

}


void API_IO_Exp_Power_Control(IO_Exp1_P0_t power_ctrl_pin_num,VoltageLevel_t enable_or_disable)
{
   API_IO_Exp1_P0_write_pin(power_ctrl_pin_num,enable_or_disable);

}

bool Self_IO_Exp_test()
{
	    bool test_status = false;

		VoltageLevel_t ioexp1_p0_read_vl;
		VoltageLevel_t ioexp1_p1_read_vl;
		VoltageLevel_t ioexp2_p0_read_vl;
		VoltageLevel_t ioexp2_p1_read_vl;

		API_IO_Exp_Select(IO_EXPANDER_1);

		API_IO_Exp1_P0_write_pin(EN_ANALOG, HIGH);
		ioexp1_p0_read_vl =  API_IO_Exp1_P1_Read_pin(EN_ANALOG, true);

		API_IO_Exp1_P1_write_pin(EFM_DISP_RESN,HIGH);
		ioexp1_p1_read_vl = API_IO_Exp1_P1_Read_pin(EFM_DISP_RESN, true);

		API_IO_Exp_Select(IO_EXPANDER_2);

		API_IO_Exp2_P0_write_pin(ECG12_A0, HIGH);
		ioexp2_p0_read_vl = API_IO_Exp2_P0_Read_pin(ECG12_A0, true);

		API_IO_Exp2_P1_write_pin(ECG12_A0, HIGH);
		ioexp2_p1_read_vl = API_IO_Exp2_P1_Read_pin(ECG12_A0, true);

		if((ioexp1_p0_read_vl == HIGH) && (ioexp1_p1_read_vl == HIGH) && (ioexp2_p0_read_vl == HIGH) && (ioexp2_p1_read_vl == HIGH))
		{
			test_status = true;
		}

		return test_status;

}

void Power_Down_All_Modules(void)
{
	uint8_t value = 0;
	// api_io_exp_setup(1);
	//API_IO_Exp1_P1_write_pin(EFM_DISP_EN1,LOW);

	//API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x2,&value,1);
	//API_IO_Exp1_P1_write_pin(ECG_CSN,HIGH);
	//API_IO_Exp1_P1_write_pin(DISPLAY_CSN,HIGH);
	/*API_IO_Exp1_P1_write_pin(NOTIFICATION_LED,HIGH);

	API_IO_Exp1_P0_write_pin(EFM_DISP_EN2,LOW);
	API_IO_Exp1_P1_write_pin(EFM_DISP_EN1,LOW);
	API_IO_Exp1_P0_write_pin(BUZZER_EN,LOW);
	API_IO_Exp1_P0_write_pin(REG_5V_EN,LOW);
	API_IO_Exp1_P0_write_pin(EN_VLED,LOW);
	API_IO_Exp1_P0_write_pin(EN_ANALOG,LOW);
	API_IO_Exp1_P0_write_pin(EN_IR,LOW);
	*/

	uint8_t input_config_ioexp1_p0 = 0xFE;
	uint8_t input_config_ioexp1_p1 = 0xFF;

	uint8_t pull_up_selection_ioexp1_p0 = 0x01;

/*	uint8_t input_config_ioexp2_p0 = 0xFF;
	uint8_t input_config_ioexp2_p1 = 0xFF;

	uint8_t disable_pull_ups = 0x00;

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x06,&input_config_ioexp1_p0,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x07,&input_config_ioexp1_p1,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x46,&pull_up_selection_ioexp1_p0,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x47,&disable_pull_ups,1);

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x06,&input_config_ioexp2_p0,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x07,&input_config_ioexp2_p1,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x46,&disable_pull_ups,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x47,&disable_pull_ups,1);*/


	/*PSU_MODE,
	BIBERNATE, // NC
	TP12,
	EN_VLED,
	EN_ANALOG,
	EN_IR,
	EFM_DISP_EN2,
	BUZZER_EN*/

	API_IO_Exp_Configure_All_InputPins_Disable_pullups();
	API_IO_Exp1_P0_write_pin(HIBERNATE,LOW);
	API_IO_Exp1_P0_write_pin(EN_VLED,LOW);
	API_IO_Exp1_P0_write_pin(EN_ANALOG,LOW);
	API_IO_Exp1_P0_write_pin(EN_IR,LOW);
	API_IO_Exp1_P0_write_pin(EFM_DISP_EN2,LOW);
	API_IO_Exp1_P0_write_pin(BUZZER_EN,LOW);

/*	EFM_DISP_RESN,
	EFM_DISP_EN1,
	NOTIFICATION_LED,
	C_LEAD_OFF_V,// NC
	ECG_RESETN,
	TP11,// NC
	ECG_CSN,
	DISPLAY_CSN*/

 API_IO_Exp1_P1_write_pin(EFM_DISP_RESN,LOW);
 API_IO_Exp1_P1_write_pin(EFM_DISP_EN1,LOW);
 API_IO_Exp1_P1_write_pin(NOTIFICATION_LED,HIGH);
 API_IO_Exp1_P1_write_pin(ECG_RESETN,LOW);
 API_IO_Exp1_P1_write_pin(ECG_CSN,LOW);
 API_IO_Exp1_P1_write_pin(DISPLAY_CSN,LOW);



/*
	TP1,
	USB_5V_SENSE,
	TP2,
	ECG12_A0,
	ECG12_A1,
	ECG12_A2,
	BAT_CHARGE,
	TP3
*/

  API_IO_Exp2_P0_write_pin(ECG12_A0,LOW);
  API_IO_Exp2_P0_write_pin(ECG12_A1,LOW);
  API_IO_Exp2_P0_write_pin(ECG12_A2,LOW);



/*	TP4,
	TP5,
	TP6,
	ECG_ALARM,
	TP7,
	DC_LEAD_OFF,
	TP8,
	TP9*/




}

void Power_Up_All_Modules(void)
{
	API_IO_Exp1_P1_write_pin(NOTIFICATION_LED,HIGH);

	API_IO_Exp1_P0_write_pin(EFM_DISP_EN2,HIGH);
	API_IO_Exp1_P1_write_pin(EFM_DISP_EN1,HIGH);
	API_IO_Exp1_P0_write_pin(BUZZER_EN,LOW);


}


void API_IO_Exp_Reset(void)
{
	uint8_t value =0xFF;

	API_IO_Exp_Select(IO_EXPANDER_1);

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x02,&value,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x03,&value,1);

	API_IO_Exp_Select(IO_EXPANDER_2);

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x02,&value,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x03,&value,1);


	uint8_t input_config_ioexp2_p0 = (byte)0b00000010;

	uint8_t disable_pull_ups_ioexp2_p0 = ~input_config_ioexp2_p0;

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x06,&input_config_ioexp2_p0,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x46,&disable_pull_ups_ioexp2_p0,1);

}


bool IsUSB_Charger_Connected(void)
{
	bool is_charger_connected = false;

	uint8_t read = 0;

	API_IO_Exp_Select(IO_EXPANDER_2);

	 API_IO_Exp_Read_reg(IO_EXP2_SLAVE_ADDR,0x00,&read);


	 if(read & 0x02) // Detect pin 2
	 {
		 is_charger_connected = true;
		 printf("\nUSB Charger connected");
	 }
	 else
	 {
		 printf("\nUSB Charger Disconnected");
	 }

	return is_charger_connected;
}

void API_IO_Exp_Disable_Pullups_to_HIBERNATE_and_5V_USB_SENSE_Pins(void)
{
	uint8_t input_config = 0x02;
	uint8_t disable_pull_ups = ~0x02;

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x06,&input_config,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x46,&disable_pull_ups,1);

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x06,&input_config,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x46,&disable_pull_ups,1);
}

void API_IO_Exp_Configure_All_InputPins_Disable_pullups(void)
{
	uint8_t input_config_ioexp1_p0 = (byte)0b00000101;
	uint8_t input_config_ioexp1_p1 = (byte)0b00101000;

	uint8_t disable_pull_ups_ioexp1_p0 = ~input_config_ioexp1_p0;
	uint8_t disable_pull_ups_ioexp1_p1 = ~input_config_ioexp1_p1;

	uint8_t input_config_ioexp2_p0 = (byte)0b11000111;
	uint8_t input_config_ioexp2_p1 = (byte)0b11111111;

	uint8_t disable_pull_ups_ioexp2_p0 = ~input_config_ioexp2_p0;
	uint8_t disable_pull_ups_ioexp2_p1 = ~input_config_ioexp2_p1;

	uint8_t current_control_p0_n1 = 0xFF;//(byte)0b10101010;
	uint8_t current_control_p0_n2 = 0xFF;//(byte)0b10101010;
	uint8_t current_control_p1_n1 = 0xFF;//(byte)0b10101010;
	uint8_t current_control_p1_n2 = 0xFF;// (byte)0b10101010;


	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x06,&input_config_ioexp1_p0,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x07,&input_config_ioexp1_p1,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x46,&disable_pull_ups_ioexp1_p0,1);
    API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x47,&disable_pull_ups_ioexp1_p1,1);


	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x06,&input_config_ioexp2_p0,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x07,&input_config_ioexp2_p1,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x46,&disable_pull_ups_ioexp2_p0,1);
    API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x47,&disable_pull_ups_ioexp2_p1,1);

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x40,&current_control_p0_n1,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x41,&current_control_p0_n2,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x42,&current_control_p1_n1,1);
    API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x43,&current_control_p1_n2,1);

}

void API_IO_Exp_Configure_All_PinsOut_DriveLow(void)
{
	uint8_t output_config_ioexp1_p0 = (byte)0b00000000;
	uint8_t output_config_ioexp1_p1 = (byte)0b00000000;

	uint8_t output_config_ioexp2_p0 = (byte)0b00000000;
	uint8_t output_config_ioexp2_p1 = (byte)0b00000000;

	uint8_t ioexp1_p0_all_pin_values = (byte)0b00000000;
	uint8_t ioexp1_p1_all_pin_values = (byte)0b00000000;

	uint8_t ioexp2_p0_all_pin_values = (byte)0b00000000;
	uint8_t ioexp2_p1_all_pin_values = (byte)0b00000000;

	uint8_t disable_pull_ups_ioexp1_p0 = (byte)0b00000000;
	uint8_t disable_pull_ups_ioexp1_p1 = (byte)0b00000000;

	uint8_t disable_pull_ups_ioexp2_p0 = (byte)0b00000000;
	uint8_t disable_pull_ups_ioexp2_p1 = (byte)0b00000000;

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x06,&output_config_ioexp1_p0,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x07,&output_config_ioexp1_p1,1);

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x06,&output_config_ioexp2_p0,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x07,&output_config_ioexp2_p1,1);

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x46,&disable_pull_ups_ioexp1_p0,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x47,&disable_pull_ups_ioexp1_p1,1);

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x46,&disable_pull_ups_ioexp2_p0,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x47,&disable_pull_ups_ioexp2_p1,1);

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x02,&ioexp1_p0_all_pin_values,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x03,&ioexp1_p1_all_pin_values,1);

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x02,&ioexp2_p0_all_pin_values,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x03,&ioexp2_p1_all_pin_values,1);


}

void API_IO_Exp_PowerOnReset_Configuration(void)
{
    uint8_t writeValue = ALL_PINS_HIGH;

	API_IO_Exp_init();

	API_IO_Exp_Select(IO_EXPANDER_1);

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_ADDR_CONFIGURATION_PORT_0,&writeValue,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_ADDR_CONFIGURATION_PORT_1,&writeValue,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_ADDR_INPUT_PORT0_LATCH,&writeValue,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, REG_ADDR_INPUT_PORT1_LATCH,&writeValue,1);

	API_IO_Exp_Select(IO_EXPANDER_2);

	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_ADDR_CONFIGURATION_PORT_0,&writeValue,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_ADDR_CONFIGURATION_PORT_1,&writeValue,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_ADDR_INPUT_PORT0_LATCH,&writeValue,1);
	API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, REG_ADDR_INPUT_PORT1_LATCH,&writeValue,1);

}

void API_IO_Exp_Soft_POR_Defualt_Register_Setup(void)
{

    uint8_t writeHigh = ALL_PINS_HIGH;
    uint8_t writeLow = ALL_PINS_LOW;

	API_IO_Exp_Select(IO_EXPANDER_1);

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x46,&writeHigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x47,&writeHigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x48,&writeHigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x49,&writeHigh,1);

	API_IO_Exp_Select(IO_EXPANDER_2);

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x40,&writeHigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x41,&writeHigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x42,&writeHigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x43,&writeHigh,1);

	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x46,&writeHigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x47,&writeHigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x48,&writeHigh,1);
	API_IO_Exp_Write_Reg(IO_EXP1_SLAVE_ADDR, 0x49,&writeHigh,1);

		uint8_t input_config_ioexp2_p0 = (byte)0b00000010;

		uint8_t disable_pull_ups_ioexp2_p0 = ~input_config_ioexp2_p0;

		API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x06,&input_config_ioexp2_p0,1);
		API_IO_Exp_Write_Reg(IO_EXP2_SLAVE_ADDR, 0x46,&disable_pull_ups_ioexp2_p0,1);

}
