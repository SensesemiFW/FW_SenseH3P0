#include "max86150.h"
#include "Max86150_Config.h"
#include "stdint.h"
#include <stdio.h>
#include "driver/i2c.h"
#include <string.h>
#include "driver/gpio.h"
#include "API_timer.h"
#include "API_utility.h"
#include "bpf.h"
#include "Hardware.h"
#include "Error_Handling.h"
#include "API_IO_Exp.h"
#include "push_button.h"

#define DRDY_INPUT_PIN_SEL  (1ULL<<MAX86150_DRDY_INTR_PIN)
#define DRDY_INTR_FLAG_DEFAULT 0

#define DATA_LENGTH 512                  /*!< Data buffer length of test buffer */
#define RW_TEST_LENGTH 128               /*!< Data length for r/w test, [0,DATA_LENGTH] */
#define DELAY_TIME_BETWEEN_ITEMS_MS 1 /*!< delay time between different test items */

#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0


#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1


 static esp_err_t api_max86150_write_reg(i2c_port_t i2c_num, uint8_t slave_addr,uint8_t reg_add, uint8_t reg_data);
 static esp_err_t api_max86150_read_reg(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t reg_addr, uint8_t *reg_data);
 static void IRAM_ATTR drdy_intr_catch(void* arg);

 static uint8_t activeDevices = 0U; //Gets set during setup. Allows check() to calculate how many bytes to read from FIFO
 static uint32_t DRDY_count = 0U; // for DRDY pulses
 static bool DRDY_Intr = false;

//Begin Interrupt configuration
uint8_t getINT1(void)
{
  return (readRegister8(MAX86150_ADDR, MAX86150_INTSTAT1));
}
uint8_t getINT2(void) {
  return (readRegister8(MAX86150_ADDR, MAX86150_INTSTAT2));
}

void enableAFULL(void) {
  bitMask(MAX86150_INTENABLE1, MAX86150_INT_A_FULL_MASK, MAX86150_INT_A_FULL_ENABLE);
}
void disableAFULL(void) {
  bitMask(MAX86150_INTENABLE1, MAX86150_INT_A_FULL_MASK, MAX86150_INT_A_FULL_DISABLE);
}

void enableDATARDY(void) {
  bitMask(MAX86150_INTENABLE1, MAX86150_INT_DATA_RDY_MASK, MAX86150_INT_DATA_RDY_ENABLE);
}
void disableDATARDY(void) {
  bitMask(MAX86150_INTENABLE1, MAX86150_INT_DATA_RDY_MASK, MAX86150_INT_DATA_RDY_DISABLE);
}

void enableALCOVF(void) {
  bitMask(MAX86150_INTENABLE1, MAX86150_INT_ALC_OVF_MASK, MAX86150_INT_ALC_OVF_ENABLE);
}
void disableALCOVF(void) {
  bitMask(MAX86150_INTENABLE1, MAX86150_INT_ALC_OVF_MASK, MAX86150_INT_ALC_OVF_DISABLE);
}

void enablePROXINT(void) {
  bitMask(MAX86150_INTENABLE1, MAX86150_INT_PROX_INT_MASK, MAX86150_INT_PROX_INT_ENABLE);
}
void disablePROXINT(void) {
  bitMask(MAX86150_INTENABLE1, MAX86150_INT_PROX_INT_MASK, MAX86150_INT_PROX_INT_DISABLE);
}
//End Interrupt configuration

void Max86150_SoftReset(void)
{
	bitMask(MAX86150_SYSCONTROL, MAX86150_RESET_MASK, MAX86150_RESET);

	uint8_t response = readRegister8(MAX86150_ADDR, MAX86150_SYSCONTROL);
	(void)response;

	Delay_ms(1000);
}

void shutDown(void) {
  // Put IC into low power mode (datasheet pg. 19)
  // During shutdown the IC will continue to respond to I2C commands but will
  // not update with or take new readings (such as temperature)
  bitMask(MAX86150_SYSCONTROL, MAX86150_SHUTDOWN_MASK, MAX86150_SHUTDOWN);
}

void wakeUp(void) {
  // Pull IC out of low power mode (datasheet pg. 19)
  bitMask(MAX86150_SYSCONTROL, MAX86150_SHUTDOWN_MASK, MAX86150_WAKEUP);
}

void setLEDMode(uint8_t mode) {
  // Set which LEDs are used for sampling -- Red only, RED+IR only, or custom.
  // See datasheet, page 19
  bitMask(MAX86150_PPGCONFIG1, MAX86150_MODE_MASK, mode);
}

void setADCRange(uint8_t adcRange) {
  // adcRange: one of MAX86150_ADCRANGE_2048, _4096, _8192, _16384
  //bitMask(MAX86150_PARTICLECONFIG, MAX86150_ADCRANGE_MASK, adcRange);
}

void setSampleRate(uint8_t sampleRate) {
  // sampleRate: one of MAX86150_SAMPLERATE_50, _100, _200, _400, _800, _1000, _1600, _3200
 //bitMask(MAX86150_PARTICLECONFIG, MAX86150_SAMPLERATE_MASK, sampleRate);
}

void setPulseWidth(uint8_t pulseWidth) {
  // pulseWidth: one of MAX86150_PULSEWIDTH_69, _188, _215, _411
  bitMask(MAX86150_PPGCONFIG1, MAX86150_PULSEWIDTH_MASK, pulseWidth);
}

// NOTE: Amplitude values: 0x00 = 0mA, 0x7F = 25.4mA, 0xFF = 50mA (typical)
// See datasheet, page 21
void setPulseAmplitudeRed(uint8_t amplitude)
{
	writeRegister8(MAX86150_ADDR, MAX86150_LED2_PULSEAMP, amplitude);

	(void)readRegister8(MAX86150_ADDR,MAX86150_LED2_PULSEAMP);
}

void setPulseAmplitudeIR(uint8_t amplitude)
{
  writeRegister8(MAX86150_ADDR, MAX86150_LED1_PULSEAMP, amplitude);
}

void setPulseAmplitudeProximity(uint8_t amplitude) {
  writeRegister8(MAX86150_ADDR, MAX86150_LED_PROX_AMP, amplitude);
}

void setProximityThreshold(uint8_t threshMSB)
{
  // The threshMSB signifies only the 8 most significant-bits of the ADC count.
  writeRegister8(MAX86150_ADDR, MAX86150_PROXINTTHRESH, threshMSB);
}

//Given a slot number assign a thing to it
//Devices are SLOT_RED_LED or SLOT_RED_PILOT (proximity)
//Assigning a SLOT_RED_LED will pulse LED
//Assigning a SLOT_RED_PILOT will ??
void enableSlot(uint8_t slotNumber, uint8_t device)
{
	  switch (slotNumber) {
	    case (1):
	      bitMask(MAX86150_FIFOCONTROL1, MAX86150_SLOT1_MASK, device);
	      break;
	    case (2):
	      bitMask(MAX86150_FIFOCONTROL1, MAX86150_SLOT2_MASK, device << 4);
	      break;
	    case (3):
	      bitMask(MAX86150_FIFOCONTROL2, MAX86150_SLOT3_MASK, device);
	      break;
	    case (4):
	      bitMask(MAX86150_FIFOCONTROL2, MAX86150_SLOT4_MASK, device << 4);
	      break;
	    default:
	      //Shouldn't be here!
	      break;
	  }
}

//Clears all slot assignments
void disableSlots(void)
{
  writeRegister8(MAX86150_ADDR, MAX86150_FIFOCONTROL1, 0);
  writeRegister8(MAX86150_ADDR, MAX86150_FIFOCONTROL2, 0);
}

//
// FIFO Configuration
//

void setFIFOAverage(uint8_t numberOfSamples)
{
  bitMask(MAX86150_FIFOCONFIG, MAX86150_SAMPLEAVG_MASK, numberOfSamples);
}

//Resets all points to start in a known state
void Max86150_Clear_Fifo(void) {
  writeRegister8(MAX86150_ADDR, MAX86150_FIFOWRITEPTR, 0);
  writeRegister8(MAX86150_ADDR, MAX86150_FIFOOVERFLOW, 0);
  writeRegister8(MAX86150_ADDR, MAX86150_FIFOREADPTR, 0);
}

//Enable roll over if FIFO over flows
void enableFIFORollover(void) {
  bitMask(MAX86150_FIFOCONFIG, MAX86150_ROLLOVER_MASK, MAX86150_ROLLOVER_ENABLE);
}

//Disable roll over if FIFO over flows
void disableFIFORollover(void) {
  bitMask(MAX86150_FIFOCONFIG, MAX86150_ROLLOVER_MASK, MAX86150_ROLLOVER_DISABLE);
}

//Power on default is 32 samples
//Note it is reverse: 0x00 is 32 samples, 0x0F is 17 samples
void setFIFOAlmostFull(uint8_t numberOfSamples) {
  bitMask(MAX86150_FIFOCONFIG, MAX86150_A_FULL_MASK, numberOfSamples);
}

//Read the FIFO Write Pointer
uint8_t getWritePointer(void) {
  return (readRegister8(MAX86150_ADDR, MAX86150_FIFOWRITEPTR));
}

//Read the FIFO Read Pointer
uint8_t getReadPointer(void) {
  return (readRegister8(MAX86150_ADDR, MAX86150_FIFOREADPTR));
}

// Set the PROX_INT_THRESHold
void setPROXINTTHRESH(uint8_t val) {
  writeRegister8(MAX86150_ADDR, MAX86150_PROXINTTHRESH, val);
}

//
// Device ID and Revision
//
uint8_t readPartID() {
  return readRegister8(MAX86150_ADDR, MAX86150_PARTID);
}

//Setup the sensor
//The MAX86150 has many settings. By default we select:
// Sample Average = 4
// Mode = MultiLED
// ADC Range = 16384 (62.5pA per LSB)
// Sample rate = 50
//Use the default setup if you are just getting started with the MAX86150 sensor
void Max86150_Configure_Registers(byte powerLevel, byte sampleAverage, byte ledMode, int sampleRate, int pulseWidth, int adcRange)
{
	activeDevices=3;
	writeRegister8(MAX86150_ADDR,MAX86150_SYSCONTROL,0x01);
	Delay_ms(200);
	writeRegister8(MAX86150_ADDR,MAX86150_FIFOCONFIG,0x7F);

	//sampleAverage=32;
	//FIFO Configuration
	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//The chip will average multiple samples of same type together if you wish
	if (sampleAverage == 1) setFIFOAverage(MAX86150_SAMPLEAVG_1); //No averaging per FIFO record
	else if (sampleAverage == 2) setFIFOAverage(MAX86150_SAMPLEAVG_2);
	else if (sampleAverage == 4) setFIFOAverage(MAX86150_SAMPLEAVG_4);
	else if (sampleAverage == 8) setFIFOAverage(MAX86150_SAMPLEAVG_8);
	else if (sampleAverage == 16) setFIFOAverage(MAX86150_SAMPLEAVG_16);
	else if (sampleAverage == 32) setFIFOAverage(MAX86150_SAMPLEAVG_32);
	else setFIFOAverage(MAX86150_SAMPLEAVG_4);

	uint16_t FIFOCode = 0x00;

	FIFOCode = FIFOCode<<4 | 0x0009;// : FIFOCode;  //insert ECG front of ETI in FIFO
	FIFOCode = FIFOCode<<8 | 0x0021;//) : FIFOCode; //insert Red(2) and IR (1) in front of ECG in FIFO

	/********* CRITICAL FOR LED GLOW ************/
	//FIFO Control 1 = FD2|FD1, FIFO Control 2 = FD4|FD3

	writeRegister8(MAX86150_ADDR,MAX86150_FIFOCONTROL1,(0b00100001));
	writeRegister8(MAX86150_ADDR,MAX86150_FIFOCONTROL2,(0b00001001));
	//writeRegister8(MAX86150_ADDR,MAX86150_FIFOCONTROL1, (char)(FIFOCode & 0x00FF) );
	//writeRegister8(MAX86150_ADDR,MAX86150_FIFOCONTROL2, (char)(FIFOCode >>8) );
	/********* END CRITICAL FOR LED GLOW ************/

	writeRegister8(MAX86150_ADDR,MAX86150_PPGCONFIG1,0b11011111);	//0b11011111

	writeRegister8(MAX86150_ADDR,MAX86150_PPGCONFIG2, 0x02);
	writeRegister8(MAX86150_ADDR,MAX86150_LED_RANGE, 0x00 ); // PPG_ADC_RGE: 32768nA

	writeRegister8(MAX86150_ADDR,MAX86150_SYSCONTROL,0x04);//start FIFO

	writeRegister8(MAX86150_ADDR,MAX86150_ECG_CONFIG1,0x00);

	//writeRegister8(MAX86150_ADDR,0xFF,0x00); //exit test mode
	//debug.Write("Registers written");

	//writeRegister8(MAX86150_ADDR,0x0E,0xDB);
	//writeRegister8(MAX86150_ADDR,0x0E,0xDB);

	setPulseAmplitudeRed(0xFF);
	setPulseAmplitudeIR(0xFF);

	//setPulseAmplitudeGreen(powerLevel);
	//setPulseAmplitudeProximity(powerLevel);
	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

	//Multi-LED Mode Configuration, Enable the reading of the three LEDs
	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	enableSlot(1, SLOT_RED_LED);
	if (ledMode > 1) enableSlot(2, SLOT_IR_LED);
	if (ledMode > 2) enableSlot(3, SLOT_ECG);
	//enableSlot(1, SLOT_RED_PILOT);
	//enableSlot(2, SLOT_IR_PILOT);
	//enableSlot(3, SLOT_GREEN_PILOT);
	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

	Max86150_Clear_Fifo(); //Reset the FIFO before we begin checking the sensor
}

//Polls the sensor for new data
//Call regularly
//If new data is available, it updates the head and tail in the main struct
//Returns number of new samples obtained
uint16_t Max86150_CheckDataAvailibity(void)
{
  //Read register FIDO_DATA in (3-byte * number of active LED) chunks
  //Until FIFO_RD_PTR = FIFO_WR_PTR

  byte readPointer = getReadPointer();
  byte writePointer = getWritePointer();
  int bytesLeftToRead = 0;

  int numberOfSamples = 0;

  if (readPointer != writePointer)
  {
    numberOfSamples = writePointer - readPointer;
    if (numberOfSamples < 0) numberOfSamples += 32; //Wrap condition

     bytesLeftToRead = numberOfSamples * activeDevices * 3;
  }

    return bytesLeftToRead;
}

//Given a register, read it, mask it, and then set the thing
void bitMask(uint8_t reg, uint8_t mask, uint8_t thing)
{
  // Grab current register context
  uint8_t originalContents = readRegister8(MAX86150_ADDR, reg);

  // Zero-out the portions of the register we're interested in
  originalContents = originalContents & mask;

  // Change contents
  writeRegister8(MAX86150_ADDR, reg, originalContents | thing);
}

uint8_t readRegister8(uint8_t address, uint8_t reg) {

	uint8_t reg_data = 0;

	api_max86150_read_reg(MAX86150_I2C_PORT_NUMBER,address,reg,&reg_data);

	return reg_data;
}

void writeRegister8(uint8_t address, uint8_t reg, uint8_t value)
{
	uint8_t reg_data = 0;

	for(int retry=0;retry<5;retry++)
	{
		api_max86150_write_reg(MAX86150_I2C_PORT_NUMBER,address,reg,value);
		reg_data = readRegister8(address,reg);
		if(reg_data == value) break;
	}

	if(reg_data != value)
	{
		//printf("Reg config fail .......... Reg addr =0x%X, write = 0x%X, Read = 0x%X\n",reg,value,reg_data);
	}
}

 void API_MAX86150_I2C_Init(void)
{
	 esp_err_t error;
	 i2c_config_t conf;

	 int i2c_master_port = MAX86150_I2C_PORT_NUMBER;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = MAX86150_I2C_MASTER_SDA_PIN;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_io_num = MAX86150_I2C_MASTER_SCL_PIN;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = MAX86150_I2C_MASTER_CLOCK_FREQ_HZ;
	conf.clk_flags = 0;

	error = i2c_param_config(i2c_master_port, &conf);
	error |= i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

	// i2c_filter_enable(i2c_master_port,1);

	if(error != ESP_OK) Catch_RunTime_Error(MAX86150_INIT_FAIL);
}


 void API_MAX86150_I2C_DeInit(void)
 {
	  i2c_driver_delete(MAX86150_I2C_PORT_NUMBER);
 }
 static esp_err_t api_max86150_read_reg(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t reg_addr, uint8_t *reg_data)
 {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();

		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, slave_addr | WRITE_BIT, ACK_CHECK_EN);
		i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
		i2c_master_start(cmd);

		i2c_master_write_byte(cmd, slave_addr | READ_BIT, ACK_CHECK_EN);

		i2c_master_read_byte(cmd,reg_data, NACK_VAL);
		i2c_master_stop(cmd);
		esp_err_t ret = i2c_master_cmd_begin(MAX86150_I2C_PORT_NUMBER, cmd, 1000 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);

		return ret;
 }


 static esp_err_t api_max86150_write_reg(i2c_port_t i2c_num, uint8_t slave_addr,uint8_t reg_add, uint8_t reg_data)
 {
 	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

 	i2c_master_start(cmd);
 	i2c_master_write_byte(cmd, slave_addr | WRITE_BIT, ACK_CHECK_EN);
 	i2c_master_write_byte(cmd, reg_add, ACK_CHECK_EN);
 	i2c_master_write_byte(cmd, reg_data, NACK_VAL);

 	i2c_master_stop(cmd);
 	esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
 	i2c_cmd_link_delete(cmd);


 	return ret;
 }


 esp_err_t API_max86150_Read_brust(uint8_t address, uint8_t reg_start_addr,uint8_t data_buff[],uint8_t nbf_bytes)
 {
	    esp_err_t error;

		i2c_cmd_handle_t cmd = i2c_cmd_link_create();

		error = i2c_master_start(cmd);
		error |= i2c_master_write_byte(cmd, address | WRITE_BIT, ACK_CHECK_EN);
		error |=i2c_master_write_byte(cmd, reg_start_addr, ACK_CHECK_EN);
		error |=i2c_master_start(cmd);
		i2c_master_write_byte(cmd, address | READ_BIT, ACK_CHECK_EN);
		if (nbf_bytes > 1)
		{
			error |=i2c_master_read(cmd, data_buff, nbf_bytes - 1, ACK_VAL);
		}
		error |=i2c_master_read_byte(cmd,data_buff+nbf_bytes-1, NACK_VAL);

		error |=i2c_master_stop(cmd);

		error |=  i2c_master_cmd_begin(MAX86150_I2C_PORT_NUMBER, cmd, 1000 / portTICK_PERIOD_MS);

		i2c_cmd_link_delete(cmd);

		if(error != ESP_OK)
		{
			printf("\nerror=%d",error);
			assert(error != ESP_OK);
		}

		return error;
 }



 void API_max86150_drdy_handle(void)
 {

	    gpio_config_t io_conf;

	    //interrupt of rising edge
	    io_conf.intr_type = GPIO_INTR_NEGEDGE;
	    //bit mask of the pins
	    io_conf.pin_bit_mask = DRDY_INPUT_PIN_SEL;
	    //set as input mode
	    io_conf.mode = GPIO_MODE_INPUT;
	    //enable pull-up mode
	    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	    gpio_config(&io_conf);

	    //change gpio intrrupt type for one pin
	    gpio_set_intr_type(MAX86150_DRDY_INTR_PIN, GPIO_INTR_NEGEDGE);

	    //install gpio isr service
	    gpio_install_isr_service(DRDY_INTR_FLAG_DEFAULT);
	    //hook isr handler for specific gpio pin
	   gpio_isr_handler_add(MAX86150_DRDY_INTR_PIN, drdy_intr_catch, (void*) MAX86150_DRDY_INTR_PIN);

 }


 static void IRAM_ATTR drdy_intr_catch(void* arg)
 {
	 DRDY_Intr = true;
	 DRDY_count++;
 }


 bool API_MAX86150_Setup(void)
 {
     bool status = false;

     API_IO_Exp_Power_Control(EN_VLED,LOW);

	 if(readPartID() == 0x1EU)
	 {
		 Max86150_SoftReset();
		 Max86150_Configure_Registers(0x10,4,0x07, 50, 50, 16384);
		 //API_max86150_drdy_handle();

		 enableDATARDY();

		 status = true;

	 }

	 else
	 {
		 printf("Device ID read Fail...........\n");

	 }

	 if(!status)  Catch_RunTime_Error(MAX86150_INIT_FAIL);

		API_IO_Exp_Power_Control(EN_VLED,HIGH);

	 return status;

 }


 bool API_MAX86150_Raw_Data_capture(uint32_t Red_data[],uint32_t IR_data[],uint32_t ECG_Data[],uint32_t nbf_samples,bool is_dummy_capture,uint8_t red_or_ir_or_ecg)
 {
		uint8_t sample_buff[20U];

		uint32_t one_sample = 0U;

	 	  for(uint8_t i=0U;i<nbf_samples;i++)
	 	   {
	 		   memset(sample_buff,0x00,sizeof(sample_buff));

	 		   if(Max86150_CheckDataAvailibity()>=9U)
	 		   {
	 				API_max86150_Read_brust(MAX86150_ADDR, 0x07U,sample_buff,6U);//9

	 				if(!is_dummy_capture)
	 				{
	 					one_sample  = sample_buff[2U] | (sample_buff[1U] << 8U) | (sample_buff[0U] << 16U);
	 					Red_data[i] = one_sample>>2U;

	 					one_sample = sample_buff[5U] | (sample_buff[4U] << 8U) | (sample_buff[3U] << 16U);
	 					IR_data[i] = one_sample;
	 				}
	 		   }
	 	   }

	 	  return ESP_OK;

 }

 void API_DRDY_Pulse_Count(void) // DRDY Count
 {
	 DRDY_count = 0;
	 API_TIMER_Register_Timer(TIMER_5SEC);

		while(1)
		{
			 if(API_TIMER_Get_Timeout_Flag(TIMER_5SEC) == true)
			 {
				printf("DRDY_count = %ld\n", DRDY_count);
				API_TIMER_Register_Timer(TIMER_5SEC);
				DRDY_count = 0;
			 }

		}

 }
