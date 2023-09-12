#ifndef MAX_MAX86150_H
#define MAX_MAX86150_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

#define MAX86150_ADDRESS          0x5E //7-bit I2C Address

typedef uint8_t byte;

  // Configuration
  void Max86150_SoftReset();
  void shutDown();
  void wakeUp();

  void setLEDMode(uint8_t mode);

  void setADCRange(uint8_t adcRange);
  void setSampleRate(uint8_t sampleRate);
  void setPulseWidth(uint8_t pulseWidth);

  void setPulseAmplitudeRed(uint8_t value);
  void setPulseAmplitudeIR(uint8_t value);
  void setPulseAmplitudeProximity(uint8_t value);

  void setProximityThreshold(uint8_t threshMSB);

  //Multi-led configuration mode (page 22)
  void enableSlot(uint8_t slotNumber, uint8_t device); //Given slot number, assign a device to slot
  void disableSlots(void);

  // Data Collection

  //Interrupts (page 13, 14)
  uint8_t getINT1(void); //Returns the main interrupt group
  uint8_t getINT2(void); //Returns the temp ready interrupt
  void enableAFULL(void); //Enable/disable individual interrupts
  void disableAFULL(void);
  void enableDATARDY(void);
  void disableDATARDY(void);
  void enableALCOVF(void);
  void disableALCOVF(void);
  void enablePROXINT(void);
  void disablePROXINT(void);
  void enableDIETEMPRDY(void);
  void disableDIETEMPRDY(void);

  //FIFO Configuration (page 18)
  void setFIFOAverage(uint8_t samples);
  void enableFIFORollover();
  void disableFIFORollover();
  void setFIFOAlmostFull(uint8_t samples);

  //FIFO Reading
  uint16_t Max86150_CheckDataAvailibity(void); //Checks for new data and fills FIFO
  uint8_t available(void); //Tells caller how many new samples are available (head - tail)
  void nextSample(void); //Advances the tail of the sense array
  uint32_t getFIFORed(void); //Returns the FIFO sample pointed to by tail
  uint32_t getFIFOIR(void);
  //Returns the FIFO sample pointed to by tail
  int32_t getFIFOECG(void); //Returns the FIFO sample pointed to by tail

  uint8_t getWritePointer(void);
  uint8_t getReadPointer(void);
  void Max86150_Clear_Fifo(void); //Sets the read/write pointers to zero

  //Proximity Mode Interrupt Threshold
  void setPROXINTTHRESH(uint8_t val);

  // Die Temperature
  float readTemperature();
  float readTemperatureF();

  // Detecting ID/Revision
  uint8_t getRevisionID();
  uint8_t readPartID();
	uint8_t readRegLED();

  // Setup the IC with user selectable settings
  //void setup(byte powerLevel = 0x1F, byte sampleAverage = 4, byte ledMode = 3, int sampleRate = 400, int pulseWidth = 411, int adcRange = 4096);

  // Low-level I2C communication
  uint8_t readRegister8(uint8_t address, uint8_t reg);
  void writeRegister8(uint8_t address, uint8_t reg, uint8_t value);

  void API_MAX86150_I2C_Init(void);
  void API_MAX86150_I2C_DeInit(void);

  void readRevisionID();

  void bitMask(uint8_t reg, uint8_t mask, uint8_t thing);
  void Max86150_Configure_Registers(byte powerLevel, byte sampleAverage, byte ledMode, int sampleRate, int pulseWidth, int adcRange);

  esp_err_t API_max86150_Read_brust(uint8_t address, uint8_t reg_start_addr,uint8_t data_buff[],uint8_t nbf_bytes);

  void API_max86150_drdy_handle(void);
  uint16_t API_MAX86150_Check_NewData(void);

  bool API_MAX86150_Setup(void);
  bool API_MAX86150_Raw_Data_capture(uint32_t Red_data[],uint32_t IR_data[],uint32_t ECG_Data[],uint32_t nbf_samples,bool is_dummy_capture,uint8_t red_or_ir_or_ecg);

  void API_DRDY_Pulse_Count(void); // DRDY Count

#endif
