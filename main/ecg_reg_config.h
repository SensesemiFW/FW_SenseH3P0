
#ifndef SRC_ECG_REG_CONFIG_H_
#define SRC_ECG_REG_CONFIG_H_

#define ECG_BIT_RATE						1000000

/* register address */

#define CONFIG_REG							0x00                   // Main Configuration //

#define FLEX_CH1_CN_REG						0x01                   // Flex Routing Switch Control for Channel 1 //
#define FLEX_CH2_CN_REG						0x02                   // Flex Routing Switch Control for Channel 2 //
#define FLEX_CH3_CN_REG						0x03                   // Flex Routing Switch Control for Channel 3 //
#define FLEX_PACE_CN_REG					0x04                   // Flex Routing Switch Control for Pace Channel //
#define FLEX_VBAT_CN_REG					0x05                   // Flex Routing Switch Control for Battery Monitoring //

#define LOD_CN_REG							0x06                   // Lead Off Detect Control //
#define LOD_EN_REG							0x07                   // Lead Off Detect Enable //
#define LOD_CURRENT_REG						0x08                   // Lead Off Detect Current //
#define LOD_AC_CN_REG						0x09                   // AC Lead Off Detect Current //

#define CMDET_EN_REG						0x0A                   // Common Mode Detect Enable //
#define CMDET_CN_REG						0x0B                   // Common Mode Detect Control //
#define RLD_CN_REG							0x0C                   // Right Leg Drive Control //

#define WILSON_EN1_REG						0x0D
#define WILSON_EN2_REG						0x0E
#define WILSON_EN3_REG						0x0F
#define WILSON_CN_REG						0x10

#define REF_CN_REG							0x11                   // Internal Reference Voltage Control //

#define OSC_CN_REG							0x12                   // Clock Source and Output Clock Control //

#define AFE_RES_REG                        0x13                   // Analog Front-End Frequency and Resolution //
#define AFE_SHDN_CN_REG                    0x14                   // Analog Front-End Shutdown Control //
#define AFE_FAULT_CN_REG                   0x15                   // Analog Front-End Fault Detection Control //
#define AFE_DITHER_EN_REG                  0x16                   // Enable Dithering in Sigma-Delta //
#define AFE_PACE_CN_REG                    0x17                   // Analog Pace Channel Output Routing Control //

#define ERROR_LOD_REG                      0x18                   // Lead Off Detect Error Status //
#define ERROR_STATUS_REG                   0x19                   // Other Error Status //
#define ERROR_RANGE1_REG                   0x1A                   // Channel 1 Amplifier Out of Range Status //
#define ERROR_RANGE2_REG                   0x1B                   // Channel 1 Amplifier Out of Range Status //
#define ERROR_RANGE3_REG                   0x1C                   // Channel 1 Amplifier Out of Range Status //
#define ERROR_SYNC_REG                     0x1D                   // Synchronization Error //
#define ERROR_MISC_REG			  		   0x1E		    		  // Miscellaneous Errors //
#define DIGO_STRENGTH_REG	           	   0x1F		    		  // Digital Output Drive Strength //


#define R2_RATE_REG                        0x21                   // R2 Decimation Rate //
#define R3_RATE_CH1_REG                    0x22                   // R3 Decimation Rate for Channel 1 //
#define R3_RATE_CH2_REG                    0x23                   // Decimation Rate for Channel 2 //
#define R3_RATE_CH3_REG					   0x24                   // R3 Decimation Rate for Channel 3 //
#define R3_RATE_CH1_CH2_CH3_REG_128			0x80
#define R3_RATE_CH1_CH2_CH3_REG_64		   0x40
#define R3_RATE_CH1_CH2_CH3_REG_32		   0x20
#define R1_RATE_REG                        0x25                   // R1 Decimation Rate //
#define DIS_EFILTER_REG                    0x26                   // ECG Filter Disable //
#define DRDYB_SRC_REG                      0x27                   // Data Ready Pin Source //
#define SYNCOUTB_SRC_REG                   0x28                   // Sync Out Pin Source //
#define MASK_DRDYB_REG                     0x29                   // Optional Mask Control for DRDYB Output //
#define MASK_ERR_REG                       0x2A                   // Mask Error on ALARMB Pin //

#define ALARM_FILTER_REG                   0x2E                   // Digital Filter for Analog Alarm Signals
#define CH_CNFG_REG                        0x2F                   // Configure Channel for Loop Read Back Mode

#define DATA_STATUS_REG                    0x30                   // ECG and Pace Data Ready Status //
#define DATA_CH1_PACE_H_REG                0x31                   // Channel1 Pace Data High [15:8] //
#define DATA_CH1_PACE_L_REG                0x32                   // Channel1 Pace Data Low [7:0] //
#define DATA_CH2_PACE_H_REG                0x33                   // Channel2 Pace Data High [15:8] //
#define DATA_CH2_PACE_L_REG                0x34                   // Channel2 Pace Data Low [7:0] //
#define DATA_CH3_PACE_H_REG                0x35                   // Channel3 Pace Data High [15:8] //
#define DATA_CH3_PACE_L_REG                0x36                   // Channel3 Pace Data Low [7:0] //
#define DATA_CH1_ECG_H_REG                 0x37                   // Channel1 ECG Data High [23:16] //
#define DATA_CH1_ECG_M_REG                 0x38                   // Channel1 ECG Data Medium [15:8] //
#define DATA_CH1_ECG_L_REG                 0x39	                  // Channel1 ECG Data Low [7:0] //
#define DATA_CH2_ECG_H_REG                 0x3A                   // Channel2 ECG Data High [23:16] //
#define DATA_CH2_ECG_M_REG                 0x3B                   // Channel2 ECG Data Medium [15:8] //
#define DATA_CH2_ECG_L_REG                 0x3C                   // Channel2 ECG Data Low [7:0] //
#define DATA_CH3_ECG_H_REG                 0x3D                   // Channel3 ECG Data High [23:16] //
#define DATA_CH3_ECG_M_REG                 0x3E                   // Channel3 ECG Data Medium [15:8] //
#define DATA_CH3_ECG_L_REG                 0x3F                   // Channel3 ECG Data Low [7:0] //

#define REVID_REG                          0x40               	  // Revision ID //
#define DATA_LOOP_REG                      0x50                   // Loop Read Back Address //

/*Settings*/

#define READ_BIT                           0x80
#define WRITE_BIT                          0x7F


//#define CH1_INP2_INN4						0x14					//Connect channel 1’s INP to IN2 and INN to IN4.
#define CH2_INP_INN_DISCNCT				  	0x00					//DisConnect channel 2’s INP and INN for Single lead configuration
#define CH3_INP_INN_DISCNCT				   	0x00					//DisConnect channel 3’s INP and INN for Single lead configuration
#define PACE_CH_INP_INN_DISCNCT				0x00					//DisConnect pace channel’s INP and INN for Single lead configuration
#define VBAT_MONITOR_DISABLE				0x00					//Battery voltage monitor disable
#define LOD_CURRENT_SEL						0xFF					//LOD current selected  2.4x10-6A
#define LOD_AC_CTRL							0x00					//Selected default settings
#define CMDET_CTRL_LOWBW					0x00					//Selected high BW = 1 and low capacitive drive mode = 00

#define WILSON_EN1_NO_CONN					0x00					//No Wilson Ref connection to 1st buffer amplifier
#define ECG_12_LEAD_WILSON_EN1_NO_CONN		0x01					//No Wilson Ref connection to 1st buffer amplifier
#define WILSON_EN2_NO_CONN					0x00					//No Wilson Ref connection to 2nd buffer amplifier
#define ECG_12_LEAD_WILSON_EN2_NO_CONN		0x02					//No Wilson Ref connection to 1st buffer amplifier
#define WILSON_EN3_NO_CONN					0x00					//No Wilson Ref connection to 3rd buffer amplifier
#define ECG_12_LEAD_WILSON_EN3_NO_CONN		0x03					//No Wilson Ref connection to 1st buffer amplifier
#define WILSON_CN_DISCONN					0x00					//Goldberger ref disabled and Wilson reference output internally disconnected from IN6
#define ECG_12_LEAD_WILSON_CN_DISCONN		0x01					//Goldberger ref disabled and Wilson reference output internally disconnected from IN6


#define CMDET_IN1_IN2_IN3					0x07					//Enable the common-mode detector on input pins IN1, IN2 and IN3.
#define RLD_IN4								0x04					//RLD output connected to IN4.
#define RLD_IN5								0x05					//RLD output connected to IN5.
#define VREF_CTRL_ON						0x00					//Internal Reference voltage and CM and RLD reference voltage ON
#define OSC_EXT								0x04					//Use external crystal and feed the internal oscillator's output to the digital.
#define AFE_SHDN_CH3						0x24					//Shuts down unused channel 3’s signal path.
#define AFE_FAULT_CTRL_ACT_CH1_CH2_CH3		0x00					//Fault detection Active in channel1, channel2 and channel3
#define AFE_DITHER_EN						0x00					// Enable Dithering in Sigma-Delta- this is not present in datasheet
#define AFE_PACE_CH_SHUTDOWN				0x01					//Analog pace channel shutdown
#define DIGO_STRENGTH_HIGH_DRIVE			0x03					//High digital output drive strength

#define AFE_FREQ_2kHZ_CH1					0x08					//Ckl freq = 204.8kHz, High resolution mode = disabled


#define R3_RATE_CH2_6					   	0x02 					//Configures the R3 decimation rate as 6 for channel 2.
#define R3_RATE_CH3_6					   	0x02 					//Configures the R3 decimation rate as 6 for channel 3.
#define ECG_FILTER_EN_CH1_CH2_CH3			0x00					//Enable ECG filter for channels 1,2 and 3
#define SYNCOUTB_SRC_DESEL					0x00					//Pin configured as output. No src selected to drive SYNCB
#define MASK_DRDYB							0x00					//optional mask control for DRDYB
#define ALARM_ACTIVE						0x00					//Alarm condition active
#define ALARM_FILTER						0x33					//default value set
#define DRDYB_SRC_CH1					   	0x08 		        	//Configures the DRDYB source to channel 1 ECG (or fastest channel).
#define CH_CNFG_CH1_CH2					   	0x30                	//Enables channel 1 ECG and channel 2 ECG for loop read-back mode.
#define START_CONV						   	0x01					//Starts data conversion
#define STOP_CONV						  	0x00					//Stops data conversion

#define CH2_INP_INN						   	0x19					//Connect channel 2’s INP to IN3 and INN to IN1
#define DC_LEAD_OFF_CTRL				   	0x00					//DC lead-off control - Active
#define DC_LEAD_OFF_EN_CH1					0x01               		//Enable DC lead-off for CH1
#define DC_LEAD_OFF_EN_CH1_CH2_CH3			0x0F               		//Enable DC lead-off for CH1,CH2,CH3, current to CH1 is anti-phase
#define AFE_SHDN_CH2_CH3				   	0x36					//Shuts down unused channel 2's and 3’s signal path.
#define CH_CNFG_CH1						   	0x10					//Enables channel 1 ECG for loop read-back mode.
#define CH1_TEST						   	0x51					//Connect channel 1’s INP to IN2 and INN to IN1 and Connect channel one to positive test signal



/*--------------------------------------BP Config------------------------------------------------------------------- */

#define CH1_INP2_INN4						0x14					//Connect channel 1’s INP to IN2 and INN to IN4.
#define CMDET_IN2_IN4					   	0x0A					//Enable the common-mode detector on input pins IN1 and IN2.
#define CMDET_IN1_IN2_IN3					0x07					//Enable the common-mode detector on input pins IN1, IN2 and IN3.

//800sps
#define STD_PACE_DATA_RATE_R1_2				0x01					//Double PACE Data Rate R1 = 2
#define R3_RATE_CH1_32					   	0x20 				  	//Configures the R3 decimation rate as 32 for channel 1.

//200sps
#define STD_PACE_DATA_RATE_R1_4				0x00					//Standard PACE Data Rate R1 = 4 // ODR = 200sps R1 = 4
#define R3_RATE_CH1_64						0x40				  	//Configures the R3 decimation rate as 64 for channel 1. // ODR = 200sps R3 = 64
#define R2_RATE_4						   	0x01                   	//Configures the R2 decimation rate as 6 for all channels.


/*------------------------------------ECG register values for decimation rate----------------------*/
#define CH1_INP_INN						    0x11					//Connect channel 1’s INP to IN2 and INN to IN1.
#define CMDET_IN1_IN2_IN3					0x07					//Enable the common-mode detector on input pins IN1,IN2 and IN3.

#define R2_RATE_8							0x08                   	//Configures the R2 decimation rate as 8 for all channels.
#define R2_RATE_6							0x06
#define R3_RATE_CH1_128					  	0x80 				  	//Configures the R3 decimation rate as 128 for channel 1.
#define R1_RATE_CH1_CH2_CH3				   	0x00
#define AFE_FREQ_2kHZ_CH1_CH2_CH3			0x38
#define AFE_SHDN_CH1_CH2_CH3_ACTIVE			0X00					// Sigma-delta modulator and instrumentation amplifier are active on CH1,Ch2,Ch3
#define AFE_PACE_CH_SHDN_OP_RLD				0X05					// Shutdown Pace Channel,Connect the analog pace channel output to the RLDIN pin.
#define CH_CNFG_CH1_CH2_CH3					0X30					// Enable data read back from ch1,ch2,ch3
/*-------------------------------------------------------------------------------------------------*/

#endif /* SRC_ECG_REG_CONFIG_H_ */
