#include "API_Display.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_err.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "freertos/task.h"
#include "protofonts.h"
#include "icons.h"
#include "API_IO_Exp.h"
#include "API_timer.h"
#include "API_utility.h"
#include "API_Flash_org.h"
#include "Hardware.h"
#include "Error_Handling.h"
#include "push_button.h"
#include "rtc.h"
#include "Firmware_version.h"
#include "OTA_Upgrade.h"
#include "ProjectConfiguration.h"
#include "bluetooth.h"

uint8_t hospital_pid[MAX_PID_RECORDS];

typedef struct __attribute__((__packed__))
{
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;


}DATE;


typedef struct __attribute__((__packed__))
{
    uint8_t hour;
    uint8_t minute;
    uint8_t seconds;

}TIME;


typedef struct __attribute__((__packed__))
{
	DATE Date;
	TIME Time;
}DATE_TIME;

uint8_t TestStateCounter = 0;


// DT018ATFT LCD Controller Commands
#define NOP                     0x00
#define SOFT_RESET              0x01
#define GET_RED_CHANNEL         0x06
#define GET_GREEN_CHANNEL       0x07
#define GET_BLUE_CHANNEL        0x08
#define GET_PIXEL_FORMAT        0x0C
#define GET_POWER_MODE          0x0A
#define GET_ADDRESS_MODE        0x0B
#define GET_DISPLAY_MODE        0x0D
#define GET_SIGNAL_MODE         0x0E
#define GET_DIAGNOSTIC_RESULT   0x0F
#define ENTER_SLEEP_MODE        0x10
#define EXIT_SLEEP_MODE         0x11
#define ENTER_PARTIAL_MODE      0x12
#define ENTER_NORMAL_MODE       0x13
#define EXIT_INVERT_MODE        0x20
#define ENTER_INVERT_MODE       0x21
#define SET_GAMMA_CURVE         0x26
#define SET_DISPLAY_OFF         0x28
#define SET_DISPLAY_ON          0x29
#define SET_COLUMN_ADDRESS      0x2A
#define SET_PAGE_ADDRESS        0x2B
#define WRITE_MEMORY_START      0x2C
#define WRITE_LUT               0x2D
#define READ_MEMORY_START       0x2E
#define SET_PARTIAL_AREA        0x30
#define SET_SCROLL_AREA         0x33
#define SET_TEAR_OFF            0x34
#define SET_TEAR_ON             0x35
#define SET_ADDRESS_MODE        0x36
#define SET_SCROLL_START        0X37
#define EXIT_IDLE_MODE          0x38
#define ENTER_IDLE_MODE         0x39
#define SET_PIXEL_FORMAT        0x3A
#define WRITE_MEMORY_CONTINUE   0x3C
#define READ_MEMORY_CONTINUE    0x3E
#define SET_TEAR_SCANLINE       0x44
#define GET_SCANLINE            0x45
#define READ_ID1                0xDA
#define READ_ID2                0xDB
#define READ_ID3                0xDC
#define FRAME_RATE_CONTROL1     0xB1
#define FRAME_RATE_CONTROL2     0xB2
#define FRAME_RATE_CONTROL3     0xB3
#define DISPLAY_INVERSION       0xB4
#define SOURCE_DRIVER_DIRECTION 0xB7
#define GATE_DRIVER_DIRECTION   0xB8
#define POWER_CONTROL1          0xC0
#define POWER_CONTROL2          0xC1
#define POWER_CONTROL3          0xC2
#define POWER_CONTROL4          0xC3
#define POWER_CONTROL5          0xC4
#define VCOM_CONTROL1           0xC5
#define VCOM_CONTROL2           0xC6
#define VCOM_OFFSET_CONTROL     0xC7
#define WRITE_ID4_VALUE         0xD3
#define NV_MEMORY_FUNCTION1     0xD7
#define NV_MEMORY_FUNCTION2     0xDE
#define POSITIVE_GAMMA_CORRECT  0xE0
#define NEGATIVE_GAMMA_CORRECT  0xE1
#define GAM_R_SEL               0xF2

#define DISP_MAX_ROWS     159  // 0 to 159
#define DISP_MAX_COLS     127  // 0 to 127
#define DISP_FRAME_LEN    6    // 6 Indicates 9 - bits transmission
#define DISP_TOTAL_PIXELS (128*160)

#define ENABLE_3_WIRE_DISP     true
#define DONT_DISPLAY           0

static void send_d_or_c_bit(bool d_or_c);

spi_device_handle_t disp_spi;

bool Is_time_displayed;
PID_TYPE_t   Selected_PID_type;

static void api_disp_write_string(const char *string, float x, uint8_t y, uint16_t fgColour, uint16_t bgColour);
static void api_disp_write_char(unsigned char c, uint8_t x, uint8_t y, uint16_t fgColour, uint16_t bgColour);
static void api_disp_draw_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t colour);
static void api_disp_highlight_pixel(uint8_t x, uint8_t y, uint16_t colour);
static void api_disp_set_pointer_driver_side(uint8_t x0, uint8_t x1,uint8_t y0, uint8_t y1);
static void api_disp_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t colour);
static void api_disp_fill_rectangle (int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

static void api_disp_display_hospital_PID_and_select(void);
static void api_disp_display_individual_PIDS_and_select(void);
static void api_disp_display_screen2_PIDS(void);
static void api_disp_display_screen1_PIDS(void);

static void api_disp_display_time(uint8_t hour, uint8_t minute);
static void api_disp_display_date(uint8_t day,char * month);

static void api_disp_display_icon(const uint8_t *ImgPointer, uint8_t left_offset, uint8_t top_offset, uint16_t IconColor, uint16_t BGColor);

static void api_disp_high_speed_data_transmit(uint8_t data[], int len);
static void api_disp_tx_buffer(uint8_t data[],uint16_t len);


/*  co-ordinate macros of the top,middle and bottom strip */
/**************************************************/

#define DISP_TOP_SECTION_ROW_START_ADDR      0
#define DISP_TOP_SECTION_ROW_END_ADDR        20
#define DISP_TOP_SECTION_COL_START_ADDR      0
#define DISP_TOP_SECTION_COL_END_ADDR        128

#define DISP_MIDDLE_SECTION_ROW_START_ADDR   21
#define DISP_MIDDLE_SECTION_ROW_END_ADDR     140
#define DISP_MIDDLE_SECTION_COL_START_ADDR   0
#define DISP_MIDDLE_SECTION_COL_END_ADDR     128

#define DISP_BTM_SECTION_ROW_START_ADDR      141
#define DISP_BTM_SECTION_ROW_END_ADDR        160
#define DISP_BTM_SECTION_COL_START_ADDR       0
#define DISP_BTM_SECTION_COL_END_ADDR        128


/**************************************************/

/*  co-ordinate macros of BT icon , date_time text,and battery icon*/
/**************************************************/

#define DISP_TOP_SEC_DATE_TIME_ROW_START_ADDR    2
#define DISP_TOP_SEC_DATE_TIME_COL_START_ADDR    35
#define DISP_TOP_SEC_BAT_ROW_START_ADDR          4
#define DISP_TOP_SEC_BAT_COL_START_ADDR          105
#define DISP_TOP_SEC_BT_ROW_START_ADDR    		  0
#define DISP_TOP_SEC_BT_COL_START_ADDR            0

/**************************************************/

/*  co-ordinate macros for to display version ids */
/**************************************************/

#define DISP_APP_VER_COL_ADDR            0
#define DISP_APP_VER_ROW_ADDR            9
#define DISP_BOOTLOADER_VER_COL_ADDR     0
#define DISP_BOOTLOADER_VER_ROW_ADDR     10

/**************************************************/

/*  co-ordinate macros for to display pid highlighters */
/**************************************************/

#define PID_HIGHLIGHTER_COL_START_ADDR     5
#define PID_HIGHLIGHTER_ROW_START_ADDR     20
#define PID_HIGHLIGHTER_COL_END_ADDR       125
#define PID_HIGHLIGHTER_ROW_END_ADDR       38

/**************************************************/

/* Macros and buffer for PATIENT ID display and selecting */
/**************************************************/

#define DISP_PID_BUFF_NBF_ROWS    10  // NBF ---> Number of
#define DISP_PID_BUFF_NBF_COLS    14
#define DISP_MAXIMUM_PIDS         9
#define DISP_LEN_OF_ONE_PID       12 // including AGE and GENDER part , each of 1 byte size

/* Macros and buffer for PATIENT ID display and selecting */
/**************************************************/

#define DISP_END_ICON_ROW_START_ADDR      141
#define DISP_END_ICON_COL_START_ADDR      2
#define DISP_RETRY_ICON_ROW_START_ADDR    141
#define DISP_RETRY_ICON_COL_START_ADDR    50
#define DISP_NEXT_ICON_ROW_START_ADDR     141
#define DISP_NEXT_ICON_COL_START_ADDR     105

/**************************************************/

char pid_buffer[DISP_PID_BUFF_NBF_ROWS][DISP_PID_BUFF_NBF_COLS];

/**************************************************/


/*  All necessary static variable  ***************/
/**************************************************/

struct DISPLAY_ICON bottom_icon,mid_icon;
static uint8_t pid_highlighter_col_start_addr;
static uint8_t pid_highlighter_row_start_addr;
static uint8_t pid_highlighter_col_end_addr;
static uint8_t pid_highlighter_row_end_addr;
static uint8_t num_of_moves   = 0x00;
static uint8_t font_bp_char;
static uint8_t font_hor;
static uint8_t font_vert;
static uint8_t font_bp_line;
static unsigned char* font;
static bool disp_pids_screen1 = TRUE;
static bool disp_pids_screen2 = FALSE;

/**************************************************/

bool Test_Exit_Flag;

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void disp_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    //gpio_set_level(PIN_NUM_DC, dc);
}




void API_Display_spi_init(void)
{
    esp_err_t error;
    spi_bus_config_t buscfg;
    spi_device_interface_config_t devcfg;

    memset(&buscfg,0,sizeof(buscfg));

	 buscfg.miso_io_num=DISP_MISO;
	 buscfg.mosi_io_num=DISP_MOSI;
	 buscfg.sclk_io_num=DISP_CLK;
	 buscfg.quadwp_io_num=-1;
	 buscfg.quadhd_io_num=-1;
	 buscfg.max_transfer_sz=128*160;

	memset(&devcfg,0,sizeof(devcfg));

	devcfg.clock_speed_hz=8000000;           //Clock out at 10 MHz

	devcfg.input_delay_ns = 0;
	devcfg.mode=0;                                //SPI mode 0
	devcfg.spics_io_num=-1;               //CS pin
	devcfg.queue_size=10;                          //We want to be able to queue 7 transactions at a time
	//.pre_cb=disp_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line

    error = spi_bus_initialize(SPI2_HOST, &buscfg, 0);

    error |= spi_bus_add_device(SPI2_HOST, &devcfg, &disp_spi);

    if(error != ESP_OK) Catch_RunTime_Error(DISPLAY_INIT_FAIL);
}

/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 *
 * Since command transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */

/*
void api_disp_write_com(spi_device_handle_t spi, const uint8_t cmd,int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    gpio_set_level(PIN_NUM_DC, 0);

    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&cmd;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}*/

void api_disp_write_com(uint8_t cmd)
{
static	esp_err_t ret;
	static  spi_transaction_t t;

	static uint16_t tx = 0;

	tx = ((cmd&0x0001)<<15) | (cmd>>1) | 0x0000;



	//tx = (uint16_t)(cmd) | 0x0100;


#ifndef ENABLE_3_WIRE_DISP
	gpio_set_level(PIN_NUM_DC, 0);
#endif

	//send_d_or_c_bit(0);

	memset(&t, 0, sizeof(t));       //Zero out the transaction

    //t.cmd = 1;

	t.length=9;                     //Command is 8 bits
	t.tx_buffer=&tx;               //The data is the cmd itself
	t.user=(void*)0;                //D/C needs to be set to 0
	ret=spi_device_transmit(disp_spi, &t);  //Transmit!
	assert(ret==ESP_OK);

}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
static void api_disp_high_speed_data_transmit(uint8_t data[], int len)
{
	printf("\n api_disp_high_speed_data_transmit");
	uint8_t tx_buff[40]={0};
    spi_transaction_t t;

    uint8_t speed = 20;
    uint16_t rem;
    uint16_t que;
    uint8_t bitPos=1;

     que = len/speed;
     rem = len % speed;
		t.addr = 1;
		t.user=(void*)1;                //D/C needs to be set to 1
		int tx_chenck=0;

		if(que)
		{
			memset(&t, 0, sizeof(t));       //Zero out the transaction

			for( tx_chenck=0;tx_chenck<que;tx_chenck++)
			{

				tx_buff[0] = (data[tx_chenck*speed]>>1) | 0x80;

					for(int i=1;i<speed+1;i++)
					{
						tx_buff[i] = ((data[(tx_chenck*speed)+(i-1)]&0x01)<<(8-bitPos)) | ((data[(tx_chenck*speed)+i]>>bitPos) | (0x80>>(bitPos)));

						bitPos++;
						if(bitPos == 9) bitPos=1;
					}
					printf("\n\n");
					printf("data[0]=%X\n,data[1]=%X\n,tx_buff[0]=%X\n,tx_buff[1]=%X\n,tx_buff[2]=%X\n",data[0],data[1],tx_buff[0],tx_buff[1],tx_buff[2]);

				t.length=speed*9;                 //Len is in bytes, transaction length is in bits.
				t.tx_buffer=tx_buff;               //Data
				spi_device_transmit(disp_spi, &t);  //Transmit!
			}

       }

		if(rem)
		{
			memset(&t, 0, sizeof(t));       //Zero out the transaction

			tx_buff[0] = (data[tx_chenck*speed]>>1) | 0x80;

			for(int i=1;i<rem+1;i++)
			{
				tx_buff[i] = ((data[(tx_chenck*speed)+(i-1)]&0x01)<<(8-bitPos)) | ((data[(tx_chenck*speed)+i]>>bitPos) | (0x80>>(bitPos)));

				bitPos++;
				if(bitPos == 8) bitPos=1;
			}

			//printf("\n\n");
			//printf("data[0]=%X\n,data[1]=%X\n,tx_buff[0]=%X\n,tx_buff[1]=%X\n,tx_buff[2]=%X\n",data[0],data[1],tx_buff[0],tx_buff[1],tx_buff[2]);

			t.length=rem*9;                 //Len is in bytes, transaction length is in bits.
			t.tx_buffer=tx_buff;               //Data
			spi_device_transmit(disp_spi, &t);  //Transmit!
		}
}

void api_disp_write_data_1byte(spi_device_handle_t spi, const uint8_t data, int len)
{

    esp_err_t ret;
    spi_transaction_t t;
    if (len==0) return;             //no need to send anything

    uint16_t tx = 0;

    	tx = (uint16_t)(data) | 0x0080;

#ifndef ENABLE_3_WIRE_DISP

    gpio_set_level(PIN_NUM_DC, 1);
#endif

	//send_d_or_c_bit(1);
    t.addr = 1;

    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=len*9;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=&tx;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.

}

void api_disp_write_data(uint8_t data)
{
    esp_err_t ret;
    spi_transaction_t t;
    uint16_t tx = 0;

      // printf("\n data%X",data);
//    tx = (uint16_t)(data) | 0x0000;
    	tx = ((data&0x0001)<<15) | (data>>1) | 0x0080;



#ifndef ENABLE_3_WIRE_DISP

    gpio_set_level(PIN_NUM_DC, 1);
#endif
	//send_d_or_c_bit(1);
    //t.addr = 1;

    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=9;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=&tx;               //Data
  //  t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_transmit(disp_spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.

}

void api_disp_tx_double(uint16_t data,bool data_or_cmd)
{

	esp_err_t ret;
	spi_transaction_t t;
	uint8_t tx[3] = {0};

	uint8_t msb=data>>8;
	uint8_t lsb=data;

	uint8_t mask=0;

	if(data_or_cmd) mask=0x80;

	tx[0] = (msb>>1) | mask;
	tx[1] =  (msb<<7) | mask>>1;
	tx[1] |= lsb>>2;
	tx[2] = lsb<<6;

	memset(&t, 0, sizeof(t));       //Zero out the transaction
	t.length=9*2;                 //Len is in bytes, transaction length is in bits.
	t.tx_buffer=&tx;               //Data
	ret=spi_device_transmit(disp_spi, &t);  //Transmit!
	assert(ret==ESP_OK);            //Should have had no issues.


}

void API_Display_spi_Deinit(void)
{
	 spi_bus_remove_device(&disp_spi);
	 spi_bus_remove_device(&disp_spi);
}
void API_Display_setup(void)
{

	int orientation = LCD_ORIENTATION0;

	/*gpio_set_level(DISP_CS_PORT,0);//cs=0
	gpio_set_level(DISP_RESET,0);//cs=0
	Delay_ms(1000);
	gpio_set_level(DISP_RESET,1);//cs=0
	Delay_ms(1000);*/

	API_IO_Exp1_P1_write_pin(EFM_DISP_RESN, LOW);
	Delay_ms(1000);
	API_IO_Exp1_P1_write_pin(EFM_DISP_RESN, HIGH);
	Delay_ms(1000);

	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, LOW);

	api_disp_write_com(SET_ADDRESS_MODE);
	api_disp_write_data(POWER_CONTROL1);

	api_disp_write_com(SET_PIXEL_FORMAT);
	api_disp_write_data(0x05);

	api_disp_write_com(POWER_CONTROL1); //Set VRH1[4:0] & VC[2:0] for VCI1 & GVDD
	api_disp_write_data(GET_BLUE_CHANNEL);
	api_disp_write_data(NOP);

	api_disp_write_com(POWER_CONTROL2); //Set BT[2:0] for AVDD & VCL & VGH & VGL
	api_disp_write_data(0x03);

	api_disp_write_com(POWER_CONTROL3);
	api_disp_write_data(0x05);

	api_disp_write_com(VCOM_CONTROL1); //Set VMH[6:0] & VML[6:0] for VOMH & VCOML
	api_disp_write_data(0x43);
	api_disp_write_data(0x43);

	api_disp_write_com(VCOM_OFFSET_CONTROL);
	api_disp_write_data(VCOM_CONTROL1);

	api_disp_write_com(FRAME_RATE_CONTROL1);
	api_disp_write_data(GET_BLUE_CHANNEL);
	api_disp_write_data(0x14);

	api_disp_write_com(0xEC); //Set pumping clock frequency
	api_disp_write_data(GET_PIXEL_FORMAT);

	api_disp_write_com(GAM_R_SEL); //Enable Gamma bit
	api_disp_write_data(SOFT_RESET);
	for(int i=0;i<300;i++)
			{
				for(int j=0;j<200;j++);
			}
	api_disp_write_com(POSITIVE_GAMMA_CORRECT);
	api_disp_write_data(READ_MEMORY_CONTINUE); //p63
	api_disp_write_data(0x1C); //p62
	api_disp_write_data(0x29); //p61
	api_disp_write_data(0x24); //p59
	api_disp_write_data(0x1D); //p57
	api_disp_write_data(0x09); //p50
	api_disp_write_data(0x50); //p43
	api_disp_write_data(0xC8); //p27/36
	api_disp_write_data(0x42); //p20
	api_disp_write_data(0x19); //p13
	api_disp_write_data(0x1C); //p6
	api_disp_write_data(0x0F); //p4
	api_disp_write_data(0x0E); //p2
	api_disp_write_data(0x05); //p1
	api_disp_write_data(NOP); //p0
	api_disp_write_com(0xE1);
	api_disp_write_data(SOFT_RESET); //p63
	for(int i=0;i<300;i++)
			{
				for(int j=0;j<200;j++);
			}
	api_disp_write_data(0x06); //p62
	api_disp_write_data(0x19); //p61
	api_disp_write_data(0x0B); //p59
	api_disp_write_data(0x12); //p57
	api_disp_write_data(0x10); //p50
	api_disp_write_data(0x27); //p43
	api_disp_write_data(0x58); //p27/36
	api_disp_write_data(SET_PIXEL_FORMAT); //p20
	api_disp_write_data(GET_BLUE_CHANNEL); //p13
	api_disp_write_data(0x1A); //p6
	api_disp_write_data(0x1E); //p4
	api_disp_write_data(0x27); //p2
	api_disp_write_data(SET_PIXEL_FORMAT); //p1
	api_disp_write_data(0x3F); //p0
	api_disp_write_com(0x11); //Exit Sleep

	for(int i=0;i<300;i++)
	{
		for(int j=0;j<200;j++);
	}
	for(int i=0;i<300;i++)
		{
			for(int j=0;j<200;j++);
		}

	api_disp_write_com(0x29); // Display On
	for(int i=0;i<300;i++)
			{
				for(int j=0;j<200;j++);
			}
	api_disp_write_com(0x2A); //Set Column Address
	api_disp_write_data(NOP);
	api_disp_write_data(NOP);
	api_disp_write_data(NOP);
	api_disp_write_data(0x7F);
	for(int i=0;i<300;i++)
			{
				for(int j=0;j<200;j++);
			}
	api_disp_write_com(0x2B); //Set Page Address
	api_disp_write_data(NOP);
	api_disp_write_data(NOP);
	api_disp_write_data(NOP);
	api_disp_write_data(0x9F);
	api_disp_write_com(SET_ADDRESS_MODE);//For setting the display ram memory pointer to start position
	api_disp_write_data(orientation);
	for(int i=0;i<300;i++)
			{
				for(int j=0;j<200;j++);
			}
	api_disp_write_com(0x2c);
	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, HIGH);

}



void API_DISP_Clear_Full_Screen(uint16_t color)
{
	int i;
	//uint8_t data_buff[DISP_TOTAL_PIXELS];
//	gpio_set_level(DISP_CS_PORT,0);//cs=0
	//vTaskDelay(100 / portTICK_RATE_MS);

	api_disp_write_com(SET_COLUMN_ADDRESS); // Column address setting
api_disp_write_data_1byte(disp_spi,0x00,1);         // start address 0
	api_disp_write_data_1byte(disp_spi,0x00,1);         // start address 0
	api_disp_write_data_1byte(disp_spi,0x00,1);         // end address 0
	api_disp_write_data_1byte(disp_spi,0x7f,1);         // end address 127

	api_disp_write_com(SET_PAGE_ADDRESS);  //Row address setting
	api_disp_write_data_1byte(disp_spi,0x00,1);         // start address 0
	api_disp_write_data_1byte(disp_spi,0x00,1);         // start address 0
	api_disp_write_data_1byte(disp_spi,0x00,1);         // end address 0
	api_disp_write_data_1byte(disp_spi,0xA0,1);           // end address 157
	api_disp_write_com(WRITE_MEMORY_START); //command for storing the pixel data in the display_RAM
	//printf("\n\n\n");
	//vTaskDelay(100 / portTICK_RATE_MS);

	for(i = 0; i < DISP_TOTAL_PIXELS; i++)//Transmit all 128*160 pixel data ,per pixel will have 16bit
	{
		api_disp_write_data_1byte(disp_spi,color>>8,1);
		api_disp_write_data_1byte(disp_spi,color,1);
		//api_disp_write_data(disp_spi,data_buff,DISP_TOTAL_PIXELS);
		//printf("%d,",i);
	}

//	gpio_set_level(DISP_CS_PORT,1);
}



void API_DISP_Clear_Full_Screen_3_Wire(uint16_t color)
{
	int i;
	API_IO_Exp1_P1_write_pin(ECG_CSN,HIGH);

	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, LOW);
	api_disp_write_com(SET_COLUMN_ADDRESS); // Column address setting
api_disp_write_data(0x00);         // start address 0
api_disp_write_data(0x00);         // start address 0
api_disp_write_data(0x00);         // end address 0
api_disp_write_data(0x7f);         // end address 127

	api_disp_write_com(SET_PAGE_ADDRESS);  //Row address setting
	api_disp_write_data(0x00);         // start address 0
	api_disp_write_data(0x00);         // start address 0
	api_disp_write_data(0x00);         // end address 0
	api_disp_write_data(0xA0);           // end address 160
	api_disp_write_com(WRITE_MEMORY_START); //command for storing the pixel data in the display_RAM

	for(i = 0; i < DISP_TOTAL_PIXELS; i++)//Transmit all 128*160 pixel data ,per pixel will have 16bit
	{
	   api_disp_tx_double(color,true);
	}

	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, HIGH);

//gpio_set_level(DISP_CS_PORT,1);
}

void API_DISP_Clear_Screen_Fast(uint16_t color)
{

	uint32_t color_swap=0;
	uint8_t lb=0;
	uint8_t hb=0;

	color_swap = (color<<8)&0xFF00;
	color_swap |= (color>>8)&0x00FF;

	/*lb = color & 0x00FF;
	hb = ((color>>8) & 0x00FF);

	color_swap = hb | (lb<<8)| (hb<<16) | (lb<<24) |  (hb<<32) | (lb<<40) | (hb<<48) | (lb<<56);
*/
	//printf("\n %X ",color_swap);

	api_disp_write_com(SET_COLUMN_ADDRESS); // Column address setting
    api_disp_write_data_1byte(disp_spi,0x00,1);         // start address 0
	api_disp_write_data_1byte(disp_spi,0x00,1);         // start address 0
	api_disp_write_data_1byte(disp_spi,0x00,1);         // end address 0
	api_disp_write_data_1byte(disp_spi,0x7f,1);         // end address 127

	api_disp_write_com(SET_PAGE_ADDRESS);  //Row address setting
	api_disp_write_data_1byte(disp_spi,0x00,1);         // start address 0
	api_disp_write_data_1byte(disp_spi,0x00,1);         // start address 0
	api_disp_write_data_1byte(disp_spi,0x00,1);         // end address 0
	api_disp_write_data_1byte(disp_spi,0xA0,1);           // end address 157
	api_disp_write_com(WRITE_MEMORY_START); //command for storing the pixel data in the display_RAM

	//api_disp_write_data_brust(disp_spi, &color_swap, DISP_TOTAL_PIXELS);

}

 uint8_t api_disp_display_char (const char *string, const uint8_t *Font, uint16_t FontColor, uint16_t BGColor, uint8_t x, uint8_t y)
{
	uint8_t count;
	uint8_t xTemp = x;
	uint8_t *tempFontptr;
	uint16_t numOfPixels;
	char *charSeq = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890:% .-/`<>!?";//for 19x10 font -fixed : DO NOT CHANGE
	uint16_t total_pixels = font19x10[0] * font19x10[1];
//	gpio_set_level(DISP_CS_PORT,0);

	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, LOW);

	while (*string!='\0')
	{
		//identify char location
		for (count=0; charSeq[count]!=*string;count++)
		if (count>TOTAL_ASCII_CHARACTERS) return 0;//character doesn't exist


		tempFontptr=(uint8_t *)font19x10+(19*11*count) + 2;
		// Horizontal Address Start Position
		api_disp_write_com(SET_COLUMN_ADDRESS);
		api_disp_write_data(0x00);
		api_disp_write_data(x);
		api_disp_write_data(0x00);
		api_disp_write_data(x+10);
		// Vertical Address end Position
		api_disp_write_com(SET_PAGE_ADDRESS);
		api_disp_write_data(0x00);
		api_disp_write_data(y);
		api_disp_write_data(0x00);
		api_disp_write_data(y+19);

		api_disp_write_com(0x2C);

		for(numOfPixels = 0; numOfPixels < total_pixels ; numOfPixels++)
		{
			if (*tempFontptr==0x00){
				 api_disp_tx_double(FontColor,true);

			}
			else
			{
				 api_disp_tx_double(BGColor,true);
			}
			tempFontptr++;
		}
		string++;
		x=x+10;
		if(x>118)
		{
			y=y+19;
			x=xTemp;
		}
		if (y>150)
		break;
	}

	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, HIGH);

return 1;
}



 static void api_disp_display_icon(const uint8_t *ImgPointer, uint8_t left_offset, uint8_t top_offset, uint16_t IconColor, uint16_t BGColor){


	uint8_t maxC = 0x00;
	uint8_t maxR = 0x00;
	uint16_t numOfPixels = 0x00;

	//gpio_set_level(DISP_CS_PORT,0);
	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, LOW);

	maxR=*ImgPointer;ImgPointer++;
	maxC=*ImgPointer; ImgPointer++;   //define maximum Row and column for icon

	api_disp_write_com(SET_COLUMN_ADDRESS); // Horizontal Address Start Position
	api_disp_write_data(0x00);
	api_disp_write_data(left_offset);
	api_disp_write_data(0x00);
	api_disp_write_data(left_offset+maxC-1);

	api_disp_write_com(SET_PAGE_ADDRESS); // Vertical Address end Position
	api_disp_write_data(0x00);
	api_disp_write_data(top_offset);
	api_disp_write_data(0x00);
	api_disp_write_data(top_offset+maxR);

	api_disp_write_com(0x2C);

	for(numOfPixels = 0; numOfPixels < maxR*maxC ; numOfPixels++)
	{
		if (*ImgPointer==0x0000){
			api_disp_write_data(IconColor>>8);
			api_disp_write_data(IconColor);
		}
		else{
			api_disp_write_data(BGColor>>8);
			api_disp_write_data(BGColor);
		}
		ImgPointer++;
	}

	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, HIGH);

	//gpio_set_level(DISP_CS_PORT,1);
}

 void api_icon_test(void){
 api_disp_display_icon(SPO2Icon1,35,50,BLUE,WHITE);
 }

 bool API_Display_interface_init(void)
 {
	API_IO_Exp_Select(IO_EXPANDER_1);

	API_IO_Exp1_P1_write_pin(ECG_CSN, HIGH);

	API_IO_Exp1_P0_write_pin(EFM_DISP_RESN, HIGH);
	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, LOW);

	API_Display_spi_init();
	API_Display_setup();


	return true;
 }




 void API_Disp_Display_Text(struct DISPLAY_TEXT display_first_row,struct  DISPLAY_TEXT display_second_row, struct DISPLAY_TEXT display_third_row,struct  DISPLAY_TEXT display_fourth_row, struct DISPLAY_TEXT display_fifth_row,struct DISPLAY_TEXT display_six_row,struct DISPLAY_TEXT display_seventh_row)
 {

 	if(display_first_row.text_status == display){
 		api_disp_display_char (display_first_row.text_starting_addr, font19x10, display_first_row.color, WHITE, 5, 20);
 	}

 	else if(display_first_row.text_status == erase){
 		api_disp_display_char ("            ", font19x10, display_first_row.color, WHITE, 0, 20);
 	}

 	if(	display_second_row.text_status == display){
 		api_disp_display_char (display_second_row.text_starting_addr, font19x10, display_second_row.color,WHITE , 5, 40);
 	}

 	else if(display_second_row.text_status ==erase){
 		api_disp_display_char ("            ", font19x10, display_second_row.color,WHITE , 0, 40);
 	}

 	if(display_third_row.text_status == display){
 		api_disp_display_char (display_third_row.text_starting_addr, font19x10, display_third_row.color, WHITE, 5, 60);
 	}

 	else if(display_third_row.text_status ==erase){
 		api_disp_display_char ("            ", font19x10, display_third_row.color, WHITE, 0, 60);
 	}

 	if(display_fourth_row.text_status == display){
 		api_disp_display_char (display_fourth_row.text_starting_addr, font19x10, display_fourth_row.color, WHITE, 5, 80);
 	}

 	else if(display_fourth_row.text_status ==erase){
 		api_disp_display_char ("            ", font19x10, display_fourth_row.color, WHITE, 0, 80);
 	}

 	if(display_fifth_row.text_status == display){
 		api_disp_display_char (display_fifth_row.text_starting_addr, font19x10, display_fifth_row.color, WHITE, 5, 100);
 	}

 	else if(display_fifth_row.text_status == erase){
 		api_disp_display_char ("            ", font19x10, display_fifth_row.color, WHITE, 0, 100);
 	}

 	if(display_six_row.text_status == display){
 		api_disp_display_char (display_six_row.text_starting_addr, font19x10, display_six_row.color, WHITE, 5, 120);
 	}

 	else if(display_six_row.text_status == erase){
 		api_disp_display_char ("            ", font19x10, display_six_row.color, WHITE, 0, 120);
 	}

 	if(display_seventh_row.text_status == display){
 		api_disp_display_char (display_seventh_row.text_starting_addr, font19x10, display_seventh_row.color, BLUE, 5, 140);
 	}

 	else if(display_seventh_row.text_status == erase){
 		api_disp_display_char ("            ", font19x10, display_seventh_row.color, BLUE, 0, 140);
 	}

 }

 void disp_text()
 {
 api_disp_display_char ("Sensesemi", font19x10, BLUE, WHITE, 10,10);
 }


 static void send_d_or_c_bit(bool d_or_c)
{
	#ifdef ENABLE_3_WIRE_DISP

		if(d_or_c)
		{
			gpio_set_level(DISP_MOSI, 0);

		}

		else
		{
			gpio_set_level(DISP_MOSI, 1);

		}

		gpio_set_level(DISP_CLK, 0);
		Delay_ms(1);
		gpio_set_level(DISP_CLK, 1);
		Delay_ms(1);
	#endif

 }


 /*************************************************************************************/
 /*************************************************************************************/

 static  void api_disp_display_time(uint8_t hour, uint8_t minute)
 {
 	char string1[7];
 	char string2[5];
 	struct DISPLAY_TEXT  date_time_text;
 	struct DISPLAY_ICON  battery_icon,bt_icon;

 	MemSet(string1,'\0',sizeof(string1));
 	MemSet(string2,'\0',sizeof(string2));

 	IntergerToString(string1,hour);

 	if(Get_strlen(string1) == 1)
 	{
 		string1[0] = '0';
 		IntergerToString(string1+1,hour);
 	}

 	else if (Get_strlen(string1) == 0)
 	{
 		string1[0] = '0';
 		string1[1] = '0';
 	}

 	StrCat(string1,":");
 	IntergerToString(string2,minute);

 	if(Get_strlen(string2) == 1)
 		{
 			string2[0] = '0';
 			IntergerToString(string2+1,minute);
 		}

 	else if (Get_strlen(string2) == 0)
 		{
 			string2[0] = '0';
 			string2[1] = '0';
 		}

 	StrCat(string1,string2);

 	date_time_text.text_starting_addr=string1;
 	date_time_text.color=WHITE;
 	date_time_text.text_status=display;

 	battery_icon.icon_status=FALSE;
 	bt_icon.icon_status=FALSE;

 	API_Display_Top_Section(bt_icon,date_time_text, battery_icon);

 }


 static  void api_disp_display_date(uint8_t day,char * month)
 {

 	char string1[10];
 	struct DISPLAY_TEXT  date_time_text;
 	struct DISPLAY_ICON  battery_icon,bt_icon;

 	MemSet(string1,'\0',sizeof(string1));

 	IntergerToString(string1,day);
 	if(Get_strlen(string1) == 1)
 		{
 			string1[0] = '0';
 			IntergerToString(string1+1,day);
 		}

 	StrCat(string1,"/");
 	StrCat(string1,month);

 	date_time_text.text_starting_addr=string1;
 	date_time_text.color=WHITE;
 	date_time_text.text_status=display;

 	bt_icon.icon_status=FALSE;
 	battery_icon.icon_status=FALSE;

 	API_Display_Top_Section(bt_icon,date_time_text, battery_icon);

 }

 void API_Display_Top_Section (struct DISPLAY_ICON display_BT_icon, struct DISPLAY_TEXT display_time_icon, struct DISPLAY_ICON display_batt_icon)
 {
 	if(display_BT_icon.icon_status == ON){
 		api_disp_display_icon(display_BT_icon.icon_starting_addr, DISP_TOP_SEC_BT_COL_START_ADDR+4, DISP_TOP_SEC_BT_ROW_START_ADDR, display_BT_icon.color, BLUE );
 	}

 	else if(display_BT_icon.icon_status == OFF){
 		api_disp_display_icon(display_BT_icon.icon_starting_addr, DISP_TOP_SEC_BT_COL_START_ADDR, DISP_TOP_SEC_BT_ROW_START_ADDR, BLUE, BLUE );
 	}

 	if(display_time_icon.text_status==display)
 	{
 		api_disp_display_char ("       ", font19x10, WHITE, BLUE, DISP_TOP_SEC_DATE_TIME_COL_START_ADDR, DISP_TOP_SEC_DATE_TIME_ROW_START_ADDR);
 		api_disp_display_char (display_time_icon.text_starting_addr, font19x10, WHITE, BLUE, DISP_TOP_SEC_DATE_TIME_COL_START_ADDR, DISP_TOP_SEC_DATE_TIME_ROW_START_ADDR);
 	}

 	else if (display_time_icon.text_status==erase){
 		api_disp_display_char ("          ", font19x10, WHITE, BLUE, DISP_TOP_SEC_DATE_TIME_COL_START_ADDR, DISP_TOP_SEC_DATE_TIME_ROW_START_ADDR);
 	}

 	if(display_batt_icon.icon_status == ON){
 		api_disp_display_icon(display_batt_icon.icon_starting_addr, DISP_TOP_SEC_BAT_COL_START_ADDR, DISP_TOP_SEC_BAT_ROW_START_ADDR, display_batt_icon.color, BLUE );
 	}

 	else if (display_batt_icon.icon_status == OFF){
 		api_disp_display_icon(display_batt_icon.icon_starting_addr, DISP_TOP_SEC_BAT_COL_START_ADDR, DISP_TOP_SEC_BAT_ROW_START_ADDR+10, BLUE, BLUE );
 	}

 }

 void API_Display_Middle_Section (struct DISPLAY_ICON display_middle_section_icon, struct DISPLAY_TEXT display_mid_sec_first_row,struct  DISPLAY_TEXT display_mid_sec_second_row, struct DISPLAY_TEXT display_mid_sec_third_row){

 	uint8_t left_offset = 46; // (128/2)-(36/2)=46
 	uint8_t top_offset  = 30;
 	//////////////////////////ICON////////////////////////////////////////
 	if(display_middle_section_icon.icon_status == ON){
 		api_disp_display_icon (display_middle_section_icon.icon_starting_addr, left_offset, top_offset, display_middle_section_icon.color, WHITE);
 	}

 	else if(display_middle_section_icon.icon_status == OFF){
 		api_disp_display_icon (display_middle_section_icon.icon_starting_addr, left_offset, top_offset, WHITE, WHITE);
 	}

 	//////////////////////////ROW1//////////////////////////////////////////
 	if(display_mid_sec_first_row.text_status == display){
 		api_disp_display_char (display_mid_sec_first_row.text_starting_addr, font19x10, display_mid_sec_first_row.color, WHITE, 5, 80);
 	}

 	else if(display_mid_sec_first_row.text_status == erase){
 	    api_disp_display_char ("           ", font19x10, display_mid_sec_first_row.color, WHITE, 0, 80);
 	}

 	///////////////////////////ROW2//////////////////////////////////////////
 	if(	display_mid_sec_second_row.text_status == display){
 	    api_disp_display_char (display_mid_sec_second_row.text_starting_addr, font19x10, display_mid_sec_second_row.color,WHITE , 5, 100);
 	}

 	else if(	display_mid_sec_second_row.text_status == erase){
 	    api_disp_display_char ("           ", font19x10, display_mid_sec_second_row.color,WHITE , 0, 100);
 	}

 	////////////////////////////ROW3////////////////////////////////////////
 	if(display_mid_sec_third_row.text_status == display){
 		api_disp_display_char (display_mid_sec_third_row.text_starting_addr, font19x10, display_mid_sec_third_row.color, WHITE, 5, 120);
 	}

 	else if(display_mid_sec_third_row.text_status == erase){
 		api_disp_display_char ("           ", font19x10, display_mid_sec_third_row.color, WHITE, 0, 120);
 	}
 }

 void  API_Display_Bottom_Section (struct DISPLAY_ICON display_bot_sec_icon, struct DISPLAY_TEXT display_bot_sec_text)
 {
 	if(display_bot_sec_icon.icon_status==ON){
 		api_disp_display_icon (display_bot_sec_icon.icon_starting_addr, 0, 140, display_bot_sec_icon.color, WHITE);
 	}

 	else if(display_bot_sec_icon.icon_status==OFF){
 		api_disp_display_icon (display_bot_sec_icon.icon_starting_addr, 0, 144, BLUE,BLUE);
 	}

 	if(display_bot_sec_text.text_status == display){
 			api_disp_display_char (display_bot_sec_text.text_starting_addr, font19x10, display_bot_sec_text.color, BLUE, 5, 142);
 	}

 	else if (display_bot_sec_text.text_status == erase){
 				api_disp_display_char ("            ", font19x10, display_bot_sec_text.color, BLUE, 5, 142);
 	}
 }


 void API_DISP_Display_Screen(DISP_SCREENS_t display_screen)
 {
 	struct DISPLAY_TEXT mid_text1,mid_text2,mid_text3,bottom_text;

 	mid_icon.color          = BLUE;
 	mid_icon.icon_status    = ON;
 	bottom_icon.color       = BLUE;
 	bottom_icon.icon_status = ON;

 	mid_text1.color  = BLUE;
 	mid_text2.color  = BLUE;
 	mid_text3.color  = BLUE;

 	mid_text1.text_status = display;
 	mid_text2.text_status = display;
 	mid_text3.text_status = display;


 	switch(display_screen)
 	{
 		case DISP_PID_SCREEN :{
 			mid_icon.icon_starting_addr = sensesemi;
 			mid_text2.text_status = FALSE;
 			mid_text1.text_starting_addr = " Select PID ";
 			mid_text3.text_status 	     = FALSE;

 			break;
 	  	  }

 		case DISP_BP_SCREEN :{
 		  mid_icon.icon_starting_addr  = BPIcon1;
 		  mid_text1.text_starting_addr = "    Blood   ";
 		  mid_text2.text_starting_addr = "  Pressure  ";
 		  mid_text3.text_status 	   = FALSE;

 	  	    break;
 	  	  }

 		case DISP_ECG_SCREEN :{
 		    mid_icon.icon_starting_addr  = ECGIcon1;
 		    mid_text1.text_starting_addr = "    ECG     ";
 			mid_text2.text_starting_addr = FALSE;
 			mid_text3.text_status        = FALSE;

 			break;
 	  	  }

 		case DISP_BG_SCREEN :{
 		    mid_icon.icon_starting_addr  = BGIcon1;
 			mid_text1.text_starting_addr = "   Blood    ";
 			mid_text2.text_starting_addr = "  Glucose   ";
 			mid_text3.text_status        = FALSE;

 	  		  break;
 	  	  }

 		case DISP_SPO2_SCREEN :{
 		    mid_icon.icon_starting_addr  = SPO2Icon1;
 			mid_text1.text_starting_addr = "    SPO2    ";
 			mid_text2.text_status        = FALSE;
 			mid_text3.text_status        = FALSE;

 	  		  break;
 	  	  }

 		case DISP_TEMP_SCREEN :{
 		    mid_icon.icon_starting_addr  = temperature;
 			mid_text1.text_starting_addr = "    Body    ";
 			mid_text2.text_starting_addr = "Temperature ";
 			mid_text3.text_status        = FALSE;

 	  		  break;
 	  	  }
 		case DISP_ECG_12_LEAD_SCREEN:
 			mid_icon.icon_starting_addr  = ECGIcon1;
 			mid_text1.text_starting_addr = "  12 Lead   ";
 			mid_text2.text_status        = FALSE;
 			mid_text3.text_status        = FALSE;
 			bottom_text.text_starting_addr = "  Start ";
 			  break;

 		case DISP_PLACE_FINGERS_ON_ELECTRODES :{

 			mid_text1.text_starting_addr = "   Place    ";
 			mid_text2.text_starting_addr = " fingers on";
 			mid_text3.text_starting_addr = " electrodes ";
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 		  }

 		case DISP_UNSUCCESSFULL :{

 		    mid_icon.icon_starting_addr  = ThumbsDownIcon1;
 		    mid_text1.text_starting_addr = "Unsuccessful";
 			mid_text2.text_status        = FALSE;
 			mid_text3.text_status        = FALSE;

 			  break;
 	      }


 		case DISP_V1_LEAD_CAPTURE_SUCCESSFULL :{

 			mid_icon.icon_starting_addr  = ThumbsUpIcon1;
 			mid_text1.text_starting_addr = " V1 capture ";
 			mid_text2.text_starting_addr = " successful ";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 		  }

 		case DISP_V2_LEAD_CAPTURE_SUCCESSFULL :{

 			mid_icon.icon_starting_addr  = ThumbsUpIcon1;
 			mid_text1.text_starting_addr = " V2 capture ";
 			mid_text2.text_starting_addr = " successful ";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 		  }

 		case DISP_V3_LEAD_CAPTURE_SUCCESSFULL :{

 			mid_icon.icon_starting_addr  = ThumbsUpIcon1;
 			mid_text1.text_starting_addr = " V3 capture ";
 			mid_text2.text_starting_addr = " successful ";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 		  }

 		case DISP_V4_LEAD_CAPTURE_SUCCESSFULL :{

 			mid_icon.icon_starting_addr  = ThumbsUpIcon1;
 			mid_text1.text_starting_addr = " V4 capture ";
 			mid_text2.text_starting_addr = " successful ";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 		  }
 		case DISP_V5_LEAD_CAPTURE_SUCCESSFULL :{

 			mid_icon.icon_starting_addr  = ThumbsUpIcon1;
 			mid_text1.text_starting_addr = " V5 capture ";
 			mid_text2.text_starting_addr = " successful ";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 		  }

 		case DISP_V6_LEAD_CAPTURE_SUCCESSFULL :{

 			mid_icon.icon_starting_addr  = ThumbsUpIcon1;
 			mid_text1.text_starting_addr = " V6 capture ";
 			mid_text2.text_starting_addr = " successful ";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 		  }

 		case DISP_SUCCESSFULL :{

 			mid_icon.icon_starting_addr  = ThumbsUpIcon1;
 			mid_text1.text_starting_addr = " Successful";
 			mid_text2.text_status        = FALSE;
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 			}
 		case DISP_TEST_IN_PROGRESS :{
 			mid_text1.text_starting_addr = "   Test in  ";
 			mid_text2.text_starting_addr = "  Progress  ";
 			mid_text3.text_status        = FALSE;
 			bottom_text.text_starting_addr = "    Exit    ";
 			  break;
 	  	  }


 		case DISP_V1_LEAD_CAPTURE_IN_PROGRESS:{
 			mid_icon.icon_starting_addr  	= ECGIcon1;
 			mid_text1.text_starting_addr 	= " V1 capture ";
 			mid_text2.text_starting_addr 	= " in progress";
 			mid_text3.text_status        	= FALSE	;
 			bottom_icon.icon_starting_addr 	= FALSE ;

 			  break;
 		}

 		case DISP_V2_LEAD_CAPTURE_IN_PROGRESS:{
 			mid_icon.icon_starting_addr  = ECGIcon1;
 			mid_text1.text_starting_addr = " V2 capture ";
 			mid_text2.text_starting_addr = " in progress";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 		}

 		case DISP_V3_LEAD_CAPTURE_IN_PROGRESS:{
 			mid_icon.icon_starting_addr  = ECGIcon1;
 			mid_text1.text_starting_addr = " V3 capture ";
 			mid_text2.text_starting_addr = " in progress";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 		}


 		case DISP_V4_LEAD_CAPTURE_IN_PROGRESS:{
 			mid_icon.icon_starting_addr  = ECGIcon1;
 			mid_text1.text_starting_addr = " V4 capture ";
 			mid_text2.text_starting_addr = " in progress";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;
 			  break;
 		}


 		case DISP_V5_LEAD_CAPTURE_IN_PROGRESS:{
 			mid_icon.icon_starting_addr  = ECGIcon1;
 			mid_text1.text_starting_addr = " V5 capture ";
 			mid_text2.text_starting_addr = " in progress";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;

 			  break;
 		}

 		case DISP_V6_LEAD_CAPTURE_IN_PROGRESS:{
 			mid_icon.icon_starting_addr  = ECGIcon1;
 			mid_text1.text_starting_addr = " V6 capture ";
 			mid_text2.text_starting_addr = " in progress";
 			mid_text3.text_status        = FALSE;
 			bottom_icon.icon_starting_addr 	= FALSE ;

 			  break;
 	  	  }

 		case DISP_INSERT_STRIP_PALCE_A_DROP_OF_BLOOD :{

 			mid_text1.text_starting_addr = "Insert Strip";
 			mid_text2.text_starting_addr = "Place a Drop";
 			mid_text3.text_starting_addr = "  Of Blood ";
 			break;
 		  }

 		case DISP_STRIP_OR_BLOOD_NOT_DETECTED :{

 			mid_text1.text_starting_addr = "Strip/Blood ";
 			mid_text2.text_starting_addr = "    Not     ";
 			mid_text3.text_starting_addr = "  Detected  ";

 	  			break;
 	  		 }
 		case DISP_PLACE_FINGER :{

 			mid_text1.text_starting_addr = "Place Finger";
 			mid_text2.text_status        = FALSE;
 			mid_text3.text_status        = FALSE;

 				break;
 			 }

 		case DISP_PALCE_ELECTRODE_ON_FOREHEAD :{

 			mid_text1.text_starting_addr = "   Place   ";
 			mid_text2.text_starting_addr = "Electrode on";
 			mid_text3.text_starting_addr = "  Forehead  ";
 			mid_icon.icon_starting_addr   = temperature;
 	  			break;
 	  		 }

 		case DISP_DEVICE_UPGRADING :{
 			mid_icon.icon_starting_addr = sensesemi;
 			mid_text1.text_starting_addr = "   Device   ";
 			mid_text2.text_starting_addr = "  Upgrading ";
 			mid_text3.text_status        = FALSE;
 			//mid_icon.icon_status = OFF;

 				break;
 	  	  	 }

 		case DISP_DEVICE_UPGRADED :{
 				mid_icon.icon_starting_addr = sensesemi;
 	  			mid_text1.text_starting_addr = "   Device   ";
 	  			mid_text2.text_starting_addr = "  Upgraded  ";
 	  			mid_text3.text_status        = FALSE;
 				//mid_icon.icon_status = OFF;

 	  				break;
 	  	  	  	 }

 		case DISP_DEVICE_UPGRADATION_FAIL :{
 				mid_icon.icon_starting_addr = sensesemi;
 				mid_text1.text_starting_addr = " Upgradation";
 				mid_text2.text_starting_addr = "    Fail    ";
 				mid_text3.text_status        = FALSE;
 				//mid_icon.icon_status = OFF;

 	  	  			break;
 	  	  	  	 }

 		case DISP_PLEASE_CARRYON_OTHER_VITALS :{

 				mid_text1.text_starting_addr = "Please carry";
 				mid_text2.text_starting_addr = "  On with   ";
 				mid_text3.text_starting_addr = "Other vital ";

 				break;
 	  	  	  }

 		case DISP_DATA_SYNC_IN_PROGRESS :{
 				mid_icon.icon_starting_addr = BLESync;
 				mid_text1.text_starting_addr = "Data Sync in";
 				mid_text2.text_starting_addr = " Progress.. ";
 				mid_text3.text_status        = false;

 				break;
 	  	  	  }


 		case DISP_DATA_SYNC_FAIL :{
 				mid_icon.icon_starting_addr = BLESync;
 				mid_text1.text_starting_addr = "  Data Sync ";
 				mid_text2.text_starting_addr = "   Aborted  ";
 				mid_text3.text_status        = false;

 				break;
 	  	  	  }
 		case DISP_DATA_SYNC_COMPLETED :{
			mid_icon.icon_starting_addr = BLESync;
			mid_text1.text_starting_addr = "  Data Sync ";
			mid_text2.text_starting_addr = "  Completed ";
			mid_text3.text_status        = false;

			break;
		  }

 		case DISP_FINGER_NOT_DETECTED :{

 				mid_text1.text_starting_addr = " Finger not ";
 				mid_text2.text_starting_addr = "  Detected  ";
 				mid_text3.text_status        = FALSE;

 				break;

 	  	  	  }

 		case DISP_TEST_START:{
 				mid_text1.text_starting_addr = "Test starts ";
 				mid_text2.text_starting_addr = "     in     ";
 				mid_text3.text_status        = FALSE;
 				bottom_text.text_starting_addr = "    Exit    ";
 				break;


 				}
 		case DISP_PID_NOT_SELECTED:{

 				mid_text1.text_starting_addr = "    PID     ";
 				mid_text2.text_starting_addr = "Not selected ";
 				mid_text3.text_status        = FALSE ;
 				bottom_text.text_status		 =	FALSE ;
 				break;
 	  	  	  }

 		case DISP_DETECTING_FINGER :{

 				mid_text1.text_starting_addr = " Detecting  ";
 				mid_text2.text_starting_addr = "   Finger   ";
 				mid_text3.text_status        = FALSE;
 				break;
 	  	  	  }

 		case DISP_TAKE_TEST_FROM_STD_DEVICE :{

 				mid_text1.text_starting_addr = "  Take test ";
 				mid_text2.text_starting_addr = "  From STD  ";
 				mid_text3.text_starting_addr = "   Device   ";
 				mid_icon.icon_status = FALSE;

 				break;
 	  	  	  }

 		case DISP_LOW_VOLTAGE_PLEASE_CHARGE :{

 				mid_text1.text_starting_addr = "Low voltage ";
 				mid_text2.text_starting_addr = "   Please   ";
 				mid_text3.text_starting_addr = "   Charge   ";
 				mid_icon.icon_starting_addr  = low_battery;
 				mid_icon.color          	 = RED;
 				break;
 	        }

 		case DISP_PLACE_FINGER_PROPERLY :

 				mid_text1.text_starting_addr = "   Place    ";
 				mid_text2.text_starting_addr = "   Finger   ";
 				mid_text3.text_starting_addr = "  Properly  ";
 				break;

 		case DISP_REPLACE_LEAD2_PROPERLY :

 					mid_text1.text_starting_addr = "  Replace   ";
 					mid_text2.text_starting_addr = "   Lead 2   ";
 					mid_text3.text_starting_addr = "  position  ";
 					break;


 		case DISP_VERY_LOW_VOLTAGE_PLEASE_CHARGE :{

 	  	 				mid_text1.text_starting_addr = " Very low   ";
 	  	 				mid_text2.text_starting_addr = " Voltage    ";
 	  	 				mid_text3.text_starting_addr = "            ";
 	  	 				mid_icon.icon_starting_addr  = low_battery;
 	  	 				mid_icon.color          	 = RED;
 	  	 				break;
 	  	 	        }
 		case DISP_RETAKE_TEST:{
 		  mid_text1.text_starting_addr = "  Re-take   ";
 		  mid_text2.text_starting_addr = "    test    ";
 		  mid_text3.text_status = FALSE;
 		  break;
 	  }
 		case DISP_DEVICE_RESETTING :{
 				mid_icon.icon_starting_addr = sensesemi;
 				mid_text1.text_starting_addr = "   Device   ";
 				mid_text2.text_starting_addr = " Resetting..";
 				mid_text3.text_starting_addr = "            ";
 				//mid_icon.icon_status         = FALSE;
 				break;
 			}
 		case DISP_INSERT_TEST_STRIP:
 		  {
 			  	mid_icon.icon_starting_addr = BGIcon1 ;
 			  	mid_text1.text_starting_addr = " Insert Test";
 				mid_text2.text_starting_addr = "   Strip    ";
 				mid_text3.text_starting_addr = "            ";

 				break;
 		  }

 		case DISP_TEST_STRIP_INSERTED:
 			{
 				mid_icon.icon_starting_addr = BGIcon1 ;
 				mid_text1.text_starting_addr = " Test Strip";
 				mid_text2.text_starting_addr = "  Detected ";
 				mid_text3.text_starting_addr = "            ";
 				break;
 			}

 		case PALCE_A_DROP_OF_BLOOD:
 			{
 				mid_icon.icon_starting_addr = BGIcon1 ;
 				mid_text1.text_starting_addr = "            ";
 				mid_text2.text_starting_addr = " Drop Blood ";
 				mid_text3.text_starting_addr = "            ";
 				break;

 			}
 		case DISP_BLOOD_DETECTED:
 		  {
 			  	mid_icon.icon_starting_addr = BGIcon1 ;
 			  	mid_text1.text_starting_addr = "   Blood    ";
 				mid_text2.text_starting_addr = "  Detected  ";
 				mid_text3.text_starting_addr = "            ";
 				break;
 		  }

 		case COUNTDOWN:
 		  {
 			  	mid_icon.icon_starting_addr = BGIcon1 ;
 			  	mid_text1.text_starting_addr = " Count-Down ";
 				mid_text2.text_starting_addr = "            ";
 				mid_text3.text_starting_addr = "            ";
 				break;

 		  }
 		case DISP_STRIP_REMOVED:
 	  	  {
 	  		  	mid_icon.icon_starting_addr = BGIcon1 ;
 	  		  	mid_text1.text_starting_addr = " Test Strip ";
 				mid_text2.text_starting_addr = "  Removed   ";
 				mid_text3.text_starting_addr = "            ";
 				break;

 	  	  }

 		case DISP_DEVICE_SLEEP :

 				mid_text1.text_starting_addr = "   Device   ";
 				mid_text2.text_starting_addr = "   Sleep    ";
 				mid_text3.text_starting_addr = "            ";
 				mid_icon.icon_status         = FALSE;
 				break;

 		case DISP_DEVICE_ACTIVE :

 				mid_text1.text_starting_addr = "   Device   ";
 				mid_text2.text_starting_addr = "   Active   ";
 				mid_text3.text_starting_addr = "            ";
 				mid_icon.icon_status         = FALSE;
 				break;

 		case DISP_PLEASE_SYNC_TIME :{

 				mid_icon.icon_starting_addr =  Time_icon ;
 				mid_text1.text_starting_addr = " Please Sync";
 				mid_text2.text_starting_addr = "  the time  ";
 				mid_text3.text_starting_addr = "            ";
 				break;
 		}
 		case DISP_PLACE_ELECTRODE_ON_V1_LEAD :
 		{
 				mid_icon.icon_starting_addr  = ECGIcon1;
 				mid_text1.text_starting_addr = " Connect to ";
 				mid_text2.text_starting_addr = " V1 position";
 				mid_text3.text_status 		 =FALSE;
 				bottom_icon.icon_starting_addr 	= FALSE ;
 				break;
 		}
 		case DISP_PLACE_ELECTRODE_ON_V2_LEAD :
 		{
 				mid_icon.icon_starting_addr  = ECGIcon1;
 				mid_text1.text_starting_addr = " Connect to ";
 				mid_text2.text_starting_addr = " V2 position";
 				mid_text3.text_status 		 =FALSE;
 				bottom_icon.icon_starting_addr 	= FALSE ;
 				break;
 		}
 		case DISP_PLACE_ELECTRODE_ON_V3_LEAD :
 		{
 				mid_icon.icon_starting_addr  = ECGIcon1;
 				mid_text1.text_starting_addr = " Connect to ";
 				mid_text2.text_starting_addr = " V3 position";
 				mid_text3.text_status 		 =FALSE;
 				bottom_icon.icon_starting_addr 	= FALSE ;
 				break;
 		}
 		case DISP_PLACE_ELECTRODE_ON_V4_LEAD :
 		{
 				mid_icon.icon_starting_addr  = ECGIcon1;
 				mid_text1.text_starting_addr = " Connect to ";
 				mid_text2.text_starting_addr = " V4 position";
 				mid_text3.text_status 		 =FALSE;
 				bottom_icon.icon_starting_addr 	= FALSE ;
 				break;
 		}
 		case DISP_PLACE_ELECTRODE_ON_V5_LEAD :
 		{
 				mid_icon.icon_starting_addr  = ECGIcon1;
 				mid_text1.text_starting_addr = " Connect to ";
 				mid_text2.text_starting_addr = " V5 position";
 				mid_text3.text_status 		 =FALSE;
 				bottom_icon.icon_starting_addr 	= FALSE ;
 				break;
 		}

 		case DISP_PLACE_ELECTRODE_ON_V6_LEAD :
 		{
 				mid_icon.icon_starting_addr  = ECGIcon1;
 				mid_text1.text_starting_addr = " Connect to ";
 				mid_text2.text_starting_addr = " V6 position";
 				mid_text3.text_status 		 =FALSE;
 				bottom_icon.icon_starting_addr 	= FALSE ;
 				break;
 		}
 		case DISP_RETRY:
 		{
 				mid_icon.icon_starting_addr  = ECGIcon1;
 				mid_text1.text_starting_addr = "   Retry    ";
 				mid_text2.text_status 		 = FALSE;
 				mid_text3.text_status 		 =FALSE;
 				bottom_icon.icon_starting_addr 	= FALSE ;
 				break;
 		}
 		case DISP_CONNECT_ELECTRODE_PROPERLY:
 		{
 				mid_icon.icon_starting_addr  = ECGIcon1;
 				mid_text1.text_starting_addr = " Connect all";
 				mid_text2.text_starting_addr = " electrodes ";
 				mid_text3.text_starting_addr = "  properly  ";
 				bottom_icon.icon_starting_addr 	= FALSE ;
 				break;
 		}


 		default :break;

 	    }

 	API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);
 	API_Clear_Display (DISP_BOTTOM_SEC ,BLUE);
 	API_Display_Middle_Section(mid_icon, mid_text1, mid_text2,mid_text3 );

 }

 void API_Clear_Display (DISP_SEC_t disp_sec ,uint16_t color)
 {
 	switch (disp_sec)
 	{
 		case  DISP_TOP_SEC :
 		{
 			 api_disp_fill_rectangle(DISP_TOP_SECTION_ROW_START_ADDR, DISP_TOP_SECTION_COL_START_ADDR, DISP_TOP_SECTION_ROW_END_ADDR, DISP_TOP_SECTION_COL_END_ADDR, color);
 		     break;
 		}

 		case  DISP_MIDDLE_SEC :
 		{
 			api_disp_fill_rectangle(DISP_MIDDLE_SECTION_ROW_START_ADDR, DISP_MIDDLE_SECTION_COL_START_ADDR, DISP_MIDDLE_SECTION_ROW_END_ADDR, DISP_MIDDLE_SECTION_COL_END_ADDR, color);
 			break;
 		}

 		case  DISP_BOTTOM_SEC :
 		{
 			api_disp_fill_rectangle(DISP_BTM_SECTION_ROW_START_ADDR, DISP_BTM_SECTION_COL_START_ADDR, DISP_BTM_SECTION_ROW_END_ADDR, DISP_BTM_SECTION_COL_END_ADDR, color);
 			break;
 		}
 	}
 }



 // Plot a string of characters to the LCD
 static void api_disp_write_string(const char *string, float x, uint8_t y, uint16_t fgColour, uint16_t bgColour)
 {
 	uint8_t origin = x;
 	x=x*font_vert;
 	y=y*font_hor;

 	for (uint8_t characterNumber = 0; characterNumber < Get_strlen((char *)string); characterNumber++)
 	{
 		// Check if we are out of bounds and move to
 		// the next line if we are
 		if (x > (128 - font_vert))
 		{
 			x = origin;
 			y += font_vert;
 		}

 		// If we move past the bottom of the screen just exit
 		if (y > (160 - font_hor)) break;

 		// Plot the current character
 		api_disp_write_char(string[characterNumber], x, y, fgColour, bgColour);
 		//x += font_hor;
 	   x += 8;
 	   }
  }


 // Plot a character at the specified x, y co-ordinates (top left hand corner of character)
  static void api_disp_write_char(unsigned char c, uint8_t x, uint8_t y, uint16_t fgColour, uint16_t bgColour)

 {     uint16_t sign;
 	 font = (unsigned char*)font;
 	 unsigned char z;
 	 unsigned int j,i,b;

 	 if ((c < 31) || (c > 127)) return;   // auf druckbares Zeichen prüfen

 	// To speed up plotting we define a x window of 6 pixels and then
 	// write out one row at a time.  This means the LCD will correctly
 	// update the memory pointer saving us a good few bytes

 	api_disp_write_com(SET_COLUMN_ADDRESS); // Horizontal Address Start Position
 	api_disp_write_data(0x00);
 	api_disp_write_data(x);
 	api_disp_write_data(0x00);
 	api_disp_write_data(x+font_hor-1);  // x + w -1 >> XEnd

 	api_disp_write_com(SET_PAGE_ADDRESS); // Vertical Address end Position
 	api_disp_write_data(0x00);
 	api_disp_write_data(y);
 	api_disp_write_data(0x00);
 	api_disp_write_data(y+font_vert-1);  // y + h -1 >> YEnd  0x7F

 	api_disp_write_com(WRITE_MEMORY_START);

 	sign = (((c -32) * font_bp_char) + 4); // start of char bitmap

 	// Plot the font data
 	for (j=0; j<font_vert; j++) //  vert line
 	{     	for (i=0; i<font_hor; i++)  //  horz line
 			{
 				z =  font[sign + (font_bp_line * i) + ((j & 0xF8) >> 3)+1];
 				b = 1 << (j & 0x07);
 				if (( z & b ) == 0x00)
 				{

 				  	 api_disp_tx_double(fgColour,true);
 				}
 				else
 					{
				  	 api_disp_tx_double(bgColour,true);
 					}

 			}
 	}

 }

  static void api_disp_draw_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t colour)
  {
  	    api_disp_draw_line(x0, y0, x0, y1, colour);
  	    api_disp_draw_line(x0, y1, x1, y1, colour);
  	    api_disp_draw_line(x1, y0, x1, y1, colour);
  	    api_disp_draw_line(x0, y0, x1, y0, colour);
  	    x0+=1;y0+=1;x1-=1;y1-=1;
  		api_disp_draw_line(x0, y0, x0, y1, colour);
  		api_disp_draw_line(x0, y1, x1, y1, colour);
  		api_disp_draw_line(x1, y0, x1, y1, colour);
  		api_disp_draw_line(x0, y0, x1, y0, colour);


  }


  static void api_disp_highlight_pixel(uint8_t x, uint8_t y, uint16_t colour)
  {
      // Horizontal Address Start Position
  	 api_disp_write_com(SET_COLUMN_ADDRESS);
  	 api_disp_write_data(0x00);         // start address x
  	 api_disp_write_data(x);         // start address x
  	 api_disp_write_data(0x00);         // end address 0
  	 api_disp_write_data(0x7f);         // end address 127

      // Vertical Address end Position
  	 api_disp_write_com(SET_PAGE_ADDRESS);
  	 api_disp_write_data(0x00);         // start address 0
  	 api_disp_write_data(y);         // start address 0
  	 api_disp_write_data(0x00);         // end address 0
  	 api_disp_write_data(0xA0);         // end address 159

      // Plot the point
  	 api_disp_write_com(WRITE_MEMORY_START);

  	 api_disp_tx_double(colour,true);


  }

  static void api_disp_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t colour)
  {
      int16_t dy = y1 - y0;
      int16_t dx = x1 - x0;
      int16_t stepx, stepy;

      if (dy < 0)
      {
          dy = -dy; stepy = -1;
      }
      else stepy = 1;

      if (dx < 0)
      {
          dx = -dx; stepx = -1;
      }
      else stepx = 1;

      dy <<= 1;                           // dy is now 2*dy
      dx <<= 1;                           // dx is now 2*dx

      api_disp_highlight_pixel(x0, y0, colour);

      if (dx > dy) {
          int fraction = dy - (dx >> 1);  // same as 2*dy - dx
          while (x0 != x1)
          {
              if (fraction >= 0)
              {
                  y0 += stepy;
                  fraction -= dx;         // same as fraction -= 2*dx
              }

              x0 += stepx;
              fraction += dy;                 // same as fraction -= 2*dy
              api_disp_highlight_pixel(x0, y0, colour);
          }
      }
      else
      {
          int fraction = dx - (dy >> 1);
          while (y0 != y1)
          {
              if (fraction >= 0)
              {
                  x0 += stepx;
                  fraction -= dy;
              }

              y0 += stepy;
              fraction += dx;
              api_disp_highlight_pixel(x0, y0, colour);
          }
      }

  }

  static void api_disp_set_pointer_driver_side(uint8_t x0, uint8_t x1,uint8_t y0, uint8_t y1)
  {
  		api_disp_write_com(SET_COLUMN_ADDRESS);  // Horizontal Address Start Position
  		api_disp_write_data(0x00);  // start address x
  		api_disp_write_data(0x0);    // start address x
  		api_disp_write_data(0x00);  // end address 0
  		api_disp_write_data(0x1);    // end address 127

  		// Vertical Address end Position
  		api_disp_write_com(SET_PAGE_ADDRESS);
  		api_disp_write_data(0x00);  // start address 0
  		api_disp_write_data(y0);    // start address 0
  		api_disp_write_data(0x00);  // end address 0
  		api_disp_write_data(y1);    // end address 159
  		api_disp_write_com(WRITE_MEMORY_START);
  }

  static void api_disp_fill_rectangle (int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
  {
  	uint16_t pixels;
  	uint16_t count;
	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, LOW);

  	api_disp_write_com(SET_COLUMN_ADDRESS); // Horizontal Address Start Position
  	api_disp_write_data(0x00);
  	api_disp_write_data(y0);
  	api_disp_write_data(0x00);
  	api_disp_write_data(y1);

  	api_disp_write_com(SET_PAGE_ADDRESS); // Vertical Address end Position
  	api_disp_write_data(0x00);
  	api_disp_write_data(x0);
  	api_disp_write_data(0x00);
  	api_disp_write_data(x1);

  	api_disp_write_com(0x2c);

  	pixels = (((x1 - x0) + 1) * ((y1 - y0) + 1)); // total pixels to write
  	for(count=0;count<pixels;count++)
  	{
		 api_disp_tx_double(color,true);
  	}

	API_IO_Exp1_P1_write_pin(DISPLAY_CSN, HIGH);


  }

  static void api_disp_display_hospital_PID_and_select(void)
  {
  	struct DISPLAY_TEXT mid_text1,mid_text2,mid_text3,mid_text4,mid_text5,mid_text6,mid_text7,bottom_text;
  	struct DISPLAY_ICON btm_left_icon,btm_middle_icon, btm_right_icon;
  	struct DISPLAY_TEXT btm_left_text,btm_middle_text,btm_right_text;

  	struct DISPLAY_ICON bottom_icon;
  	uint8_t col1 = 7;
  	uint8_t pid_highlighter_row_start_addr = 75;
  	uint8_t col2 = 118;
  	uint8_t pid_highlighter_row_end_addr = 99;
  	uint8_t button = 0x00;
  	bool selection_mode = TRUE;
  	uint8_t pid[10] =" ";

  	StrCat((char*)pid,(char*)hospital_pid);

  	if(Get_strlen((char*)hospital_pid) == 0x00)
  	{
  		API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);
  		API_Clear_Display (DISP_BOTTOM_SEC ,BLUE);
  		mid_text3.text_starting_addr="  PIDS NOT  ";
  		mid_text3.color=BLUE;
  		mid_text3.text_status=display;

  		mid_text4.text_starting_addr=" Registered ";
  		mid_text4.color=BLUE;
  		mid_text4.text_status=display;

  		mid_text1.text_status = FALSE;
  		mid_text2.text_status = FALSE;
  		mid_text5.text_status = FALSE;
  		mid_text6.text_status = FALSE;
  		mid_text7.text_status = FALSE;

  		API_Disp_Display_Text(mid_text1,mid_text2, mid_text3,mid_text4, mid_text5,mid_text6,mid_text7);

  		Delay_ms(2000);
  	}

  	else
  	{
  		mid_text1.text_status=FALSE;
  		mid_text2.text_status=FALSE;
  		//mid_text2.color=BLUE;
  		//mid_text2.text_status=display;
  		mid_text3.text_status=FALSE;

  		mid_text4.text_starting_addr=(char *)pid;
  		mid_text4.color=BLUE;
  		mid_text4.text_status=display;

  		mid_text5.text_status=FALSE;
  		mid_text6.text_status=FALSE;
  		mid_text7.text_status=FALSE;
  		//mid_text2.color = BLUE;
  		mid_text4.color=BLUE;

  		bottom_icon.icon_status           = FALSE;

  		bottom_text.text_starting_addr    = "       START";
  		bottom_text.color                 = WHITE;
  		bottom_text.text_status           = display;

  		API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);

  		API_Display_Bottom_Section(bottom_icon, bottom_text);
  		api_disp_display_icon(Home_image, DISP_END_ICON_COL_START_ADDR, DISP_END_ICON_ROW_START_ADDR, WHITE, BLUE);

  		API_Disp_Display_Text(mid_text1,mid_text2, mid_text3,mid_text4, mid_text5,mid_text6,mid_text7);
  		api_disp_draw_rectangle (col1, pid_highlighter_row_start_addr, col2, pid_highlighter_row_end_addr, BLUE);


  		API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);
  		selection_mode = TRUE;
  		while(selection_mode)
  		{
  			//button = API_Scan_Button();

  			/*if(WakeUp_count == 1) // for start and updating the pid in the struture
  			{
  				API_TIMER_Register_Timer(User_inactive_timer_value);
  				MemCpy(&record_header.patient_id,hospital_pid,10);
  				Selected_PID_type = VALID_PID;
  				selection_mode = FALSE;
  			}

  			if(API_TIMER_Get_Timeout_Flag(User_inactive_timer_value) == TRUE)
  			{
  				selection_mode = FALSE;
  				flash_data.sys_mode = SLEEP_MODE;
  			}

  			if(Power_up_flag == SYSTEM_POWER_DOWN)
  			{
  				flash_data.sys_mode = SLEEP_MODE;
  				break;
  			}*/

  		 }
  	   }
   }


  static void api_disp_display_individual_PIDS_and_select()
  {
  	char buffer[DISP_MAXIMUM_PIDS*DISP_LEN_OF_ONE_PID];
  	struct DISPLAY_TEXT bottom_text;

  	NAVIGATE_BUTTON_TYPE  button  = TOTAL_NUM_BTN;

  	uint8_t  track_id           = 0x00;
  	uint8_t  total_pids         = 0x00;
  	uint8_t  ascci_value_of_1   = 49;
  	uint8_t  ascci_value_of_dot = 46;
  	uint8_t  row_index          = 0x00;
  	uint8_t  col_index          = 0x00;
  	uint16_t buffer_index       = 0x00;

  	total_pids = Get_all_PIDs(buffer);
  	MemSet(pid_buffer,'\0',sizeof(pid_buffer));

  	for(col_index=0;col_index<total_pids;col_index++)
  	{
  		for( row_index=0; row_index<DISP_LEN_OF_ONE_PID; row_index++)
  		{
  			if(row_index == 0)
  			{
  				pid_buffer[col_index][row_index] = ascci_value_of_1 ;
  			}

  			else if(row_index == 1)
  			{
  				pid_buffer[col_index][row_index] = ascci_value_of_dot;
  			}

  			else
  			{
  				if(buffer[buffer_index] != '\0') // condition to eliminate the leading zeros
  				{
  					pid_buffer[col_index][row_index] = buffer[buffer_index];
  				}
  				buffer_index ++;
  			}
  		}

  		buffer_index += 2; // gender length + age length
  		ascci_value_of_1 ++;

  	}

  	if(disp_pids_screen1 == TRUE)
  	{
  		api_disp_display_screen1_PIDS();
  	}

  	if(disp_pids_screen2 == TRUE)
  	{
  		api_disp_display_screen2_PIDS();
  	}

  	bottom_icon.icon_starting_addr = STARTIcon1;
  	bottom_icon.color              = BLUE;
  	bottom_icon.icon_status        = ON;

  	bottom_text.text_starting_addr    = "    EXIT    ";
  	bottom_text.color                 = WHITE;
  	bottom_text.text_status           = FALSE;

  	API_Display_Bottom_Section(bottom_icon, bottom_text);
  	api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, BLUE);

  	API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);
  	////(User_inactive_timer_value);

  	//num_of_moves = 1; // indicates PID heighlighter is at guest PID position

  	for(Selected_PID_type = PID_NOT_SELECTED ; (FALSE == ((Selected_PID_type == VALID_PID) || (Selected_PID_type == GUEST_PID) )) ; )
  	{
  		//button = API_Scan_Button();

  		if(button == NEXT)
  		{
  			if(num_of_moves <= total_pids)
  			{
  				num_of_moves ++;
  			}

  			if(num_of_moves > total_pids)
  			{
  				api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, WHITE);

  				pid_highlighter_col_start_addr = PID_HIGHLIGHTER_COL_START_ADDR;
  				pid_highlighter_row_start_addr = PID_HIGHLIGHTER_ROW_START_ADDR;
  				pid_highlighter_col_end_addr   = PID_HIGHLIGHTER_COL_END_ADDR;
  				pid_highlighter_row_end_addr   = PID_HIGHLIGHTER_ROW_END_ADDR;

  				if(disp_pids_screen2)
  				{
  					disp_pids_screen2 = FALSE;
  					disp_pids_screen1 = TRUE;
  					api_disp_display_screen1_PIDS();
  				}

  				api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, BLUE);

  				num_of_moves = 0;
  				button = TOTAL_NUM_BTN;
  			}
  		}

  		else if(button == BACK)
  		{

  			if(num_of_moves == 6) // 6 is , total Rectangle(PID heighlighters) moves to arrive the rectangle at the bottom form the beginning
  			{
  					if(disp_pids_screen2)
  					{
  						disp_pids_screen2 = FALSE;
  						disp_pids_screen1 = TRUE;
  						api_disp_display_screen1_PIDS();
  						api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, WHITE);

  						pid_highlighter_row_start_addr += PID_HIGHLIGHTER_ROW_START_ADDR*5;
  						pid_highlighter_row_end_addr   += PID_HIGHLIGHTER_ROW_START_ADDR*5;

  						api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, BLUE);

  						num_of_moves = 5;
  						button = TOTAL_NUM_BTN;
  					}
  			  }

  			else
  			{
  				if(num_of_moves)
  				{
  					num_of_moves --;
  				}

  				else
  				{
  					button = TOTAL_NUM_BTN;
  				}
  			}

  		}

  		if(button == NEXT)
  		{
  				 API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);

  				 if(pid_highlighter_row_start_addr >= 120) // 120 is the last row start address of the rectangle
  				 {
  					api_disp_display_screen2_PIDS();

  					pid_highlighter_col_start_addr = PID_HIGHLIGHTER_COL_START_ADDR;
  					pid_highlighter_row_start_addr = PID_HIGHLIGHTER_ROW_START_ADDR;
  					pid_highlighter_col_end_addr   = PID_HIGHLIGHTER_COL_END_ADDR;
  					pid_highlighter_row_end_addr   = PID_HIGHLIGHTER_ROW_END_ADDR;
  					disp_pids_screen1 = FALSE;
  					disp_pids_screen2 = TRUE;

  					api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, BLUE);
  				 }

  				 else
  				 {
  					api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, WHITE);

  					pid_highlighter_row_start_addr += PID_HIGHLIGHTER_ROW_START_ADDR;
  					pid_highlighter_row_end_addr   += PID_HIGHLIGHTER_ROW_START_ADDR;

  					api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, BLUE);
  				 }

  		  }

  		else if(button == BACK)
  		{
  			 API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);
  			 api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, WHITE);

  			 pid_highlighter_row_start_addr -= PID_HIGHLIGHTER_ROW_START_ADDR;
  			 pid_highlighter_row_end_addr   -= PID_HIGHLIGHTER_ROW_START_ADDR;

  			 if(pid_highlighter_row_start_addr < PID_HIGHLIGHTER_ROW_START_ADDR)
  			 {
  				  api_disp_display_screen1_PIDS();
  				  disp_pids_screen1 = TRUE;
  				  disp_pids_screen2 = FALSE;
  				  pid_highlighter_row_start_addr = PID_HIGHLIGHTER_ROW_START_ADDR;
  				  pid_highlighter_row_end_addr   = PID_HIGHLIGHTER_ROW_END_ADDR;

  			 }

  			 api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, BLUE);
  		}

  		else if(button == ENTER)
  		{
  			 api_disp_draw_rectangle(pid_highlighter_col_start_addr, pid_highlighter_row_start_addr, pid_highlighter_col_end_addr, pid_highlighter_row_end_addr, GREEN);
  			 track_id = pid_highlighter_row_start_addr / PID_HIGHLIGHTER_ROW_START_ADDR;

  			 if(disp_pids_screen1 == TRUE)
  			 {
  				 if(track_id == 1)
  				 {
  					 MemSet(record_header.patient_id,0x00,sizeof(record_header.patient_id));
  					 Selected_PID_type = GUEST_PID;
  				 }

  				 else
  				 {
  					MemCpy(record_header.patient_id,&pid_buffer[track_id-2][2],sizeof(record_header.patient_id));
  					Selected_PID_type = VALID_PID;
  				 }
  			 }

  			 else if(disp_pids_screen2 == TRUE)
  			 {
  				 disp_pids_screen2 = TRUE;
  				 disp_pids_screen1 = FALSE;
  				 MemCpy(record_header.patient_id,&pid_buffer[track_id+4][2],sizeof(record_header.patient_id));

  				 Selected_PID_type = VALID_PID;
  			 }

  		 }

  		/*if(API_TIMER_Get_Timeout_Flag(User_inactive_timer_value)==true)
  		{
  			flash_data.sys_mode = SLEEP_MODE;

  			break;
  		}

  		if(Power_up_flag == SYSTEM_POWER_DOWN)
  		{
  			flash_data.sys_mode = SLEEP_MODE;
  			////(User_inactive_timer_value);
  			break;
  		}*/
      }

  }

  void API_Select_PID()
  {
  	uint8_t pid[10] =" ";

  	if(Get_strlen((char*)hospital_pid) == 0x00)
  		{
  		     Selected_PID_type = PID_NOT_SELECTED;
  		}
  	else
  	{

  	StrCat((char *)pid,(char *)hospital_pid);

  	MemCpy(&record_header.patient_id,hospital_pid,10);
  	Selected_PID_type = VALID_PID;
  	}
  }


  void API_Hold_Select_PID_State()
  {
  	API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);
  	while(1)
  	{/*
  		if(WakeUp_count == 1)
  		{
  			WakeUp_count = 0;
  			state = VIEW_TEST;// pragnya
  			break;
  		}

  		if(API_TIMER_Get_Timeout_Flag(User_inactive_timer_value) == TRUE)
  		{
  			flash_data.sys_mode = SLEEP_MODE;
  			break;
  		}

  	bool	flag = BT_Rx_Flag();
  						if(flag == TRUE)
  						{
  							BT_receive();
  							Enable_main_loop_wdog();
  						}
  	*/}
  }

VITAL_TYPE_t ret_msg=0;
uint8_t top_offset = 20;
uint8_t left_offset = 0;

 VITAL_TYPE_t API_Display_View_Screen()
 {
	VITAL_TYPE_t ret_msg=0xFF;
	uint8_t top_offset = 20;
	uint8_t count = 1;
	uint8_t left_offset = 20;
	uint8_t btn_press;

  	struct DISPLAY_TEXT text1,text2,text3,text4,text5,text6,text7;

  	text1.color= BLUE;
  	text1.text_starting_addr = "   Select   ";
  	text1.text_status = display;

  	text2.color = BLUE;
  	text2.text_starting_addr = " Quick Vital"; /*!  { spo2, HR(ECG-L1), BP }  */
  	text2.text_status = display;

  	text3.color = BLUE;
  	text3.text_starting_addr = " Multi Vital";/*!  { spo2, HR(ECG-L6), BP, Temp, BG }  */
  	text3.text_status = display;

  	text4.color = BLUE;
  	text4.text_starting_addr = "    Exit    ";
  	text4.text_status = display;

  	//text6.color = BLUE;
  	//text6.text_starting_addr = "    Exit    ";
	text5.text_status = 0;
  	text6.text_status = 0;
  	text7.text_status = 0;

  	API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);
  	API_Disp_Display_Text(text1, text2, text3, text4, text5, text6, text7);
  	API_Clear_Display(DISP_BOTTOM_SEC,BLUE);
  	api_disp_display_icon(star,left_offset,top_offset,RED,WHITE);

  	API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);
  	API_TIMER_Kill_Timer(TEST_ENTERY_TIMEOUT);
  	printf("\n\nIn View Screen\n");

	btn_press = API_Push_Btn_Get_Buttton_Press(); // Dummy read

  	while(1)
  	{
		//btn_press = API_Push_Btn_Get_hold_time();

  		btn_press = API_Push_Btn_Get_Buttton_Press();


		if(btn_press == 3)
		{
			esp_restart();
		}
  		if((btn_press == 1) || ((btn_press == 2)))
			{
			    API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);
				API_TIMER_Register_Timer(TEST_ENTERY_TIMEOUT);//timer register to refresh timer in order to start timer freshly

				Test_Exit_Flag = FALSE;
				api_disp_display_icon(star,left_offset,top_offset,WHITE,WHITE);
				count++;

				if(count == 2)
				{
					left_offset = 3;
				}
				 if(count == 4)
				 {
					left_offset = 30;
				 }

				 top_offset = top_offset + 20;

				 if (count == 5)
				 {
					count = 1;
					left_offset = 20;
					top_offset = 20;
					API_TIMER_Kill_Timer(TEST_ENTERY_TIMEOUT);
				  }

				api_disp_display_icon(star,left_offset,top_offset,RED,WHITE);
			}

  		if(API_TIMER_Get_Timeout_Flag(TEST_ENTERY_TIMEOUT)) // call get_time_out function
		{
			//printf("\nTime out %d",TEST_ENTERY_TIMEOUT);
			break;
		}

  		if(API_TIMER_Get_Timeout_Flag(USER_INACTIVE_TIMEOUT) == TRUE) // call get_time_out function
  		{
  			flash_data.sys_mode = DEVICE_HIBERNET_MODE;
  			count=0;
  			break;
  		}

  		if (is_firmware_data_available())
		{
		  count = 0;
           break;
		}

  		if(API_TIMER_Get_Timeout_Flag(DATE_TIME_FLIP_TIME) == TRUE) // call get_time_out function
  		{
			API_DISP_Toggle_Date_Time();
			API_TIMER_Register_Timer(DATE_TIME_FLIP_TIME);
  		}


  		if(Data_sync_in_progress)
  			{
  				count = 0xFF;
  				break;
  			}


  		if(Is_Device_Paired == BT_PAIRED) // Paired condition
  		{
  			API_Disp_BT_Icon(GREEN);
  			Is_Device_Paired = DEFAULT;// to avoid Redisplaying the same thing again
  		}

  		else if(Is_Device_Paired == BT_DISCONNECTED) // disconnected condition
		{
			API_Disp_BT_Icon(WHITE);
			Is_Device_Paired = DEFAULT;
		}

  		//printf("\nbtn_press=%d\n",btn_press);
  	}

  	switch(count)
  	{
		case 2 : ret_msg = QUICK_VITALS; break;
		case 3 : ret_msg = MULTI_VITALS; break;
		case 4 : ret_msg = TEST_EXIT; break;
		case 0xFF :ret_msg = DATA_SYNC; break;
		default: ret_msg = NO_TEST; break;
  	}

	//printf("\nTest type=%d",ret_msg);
  	printf("\ncount=%d  flash_data.sys_mode:%d",count,flash_data.sys_mode);
  	printf("\n\nExitting View Screen\n");

	return ret_msg;

  }


  bool API_DISP_Wait_Time(VITAL_TYPE_t test_type,uint8_t countdown_time)
  {
  	struct DISPLAY_ICON mid_sec_icon,btm_icon;
  	struct DISPLAY_TEXT mid_sec_text1,mid_sec_text2,mid_sec_text3,btm_text;
  	struct DISPLAY_TEXT mid_qt_text1,mid_qt_text2,mid_qt_text3,mid_qt_text4,mid_qt_text5,mid_qt_text6,mid_qt_text7;

  	uint8_t tick_num = 0;
  	char string_time[3]= {"\0"};
      uint8_t button = 0x00;
      bool ret_msg = FALSE;

      MemSet(string_time,'\0',Get_strlen(string_time));

  	switch(test_type)
  	{
  		case BP1:{
  			mid_sec_icon.icon_starting_addr=BPIcon1;
  		}
  			break;

  		case BG:{
  			mid_sec_icon.icon_starting_addr=BGIcon1;
  		}
  					break;

  		case ECG_6_Lead:{
  			mid_sec_icon.icon_starting_addr=ECGIcon1;
  		}
  					break;

  		case SPO2:{
  			mid_sec_icon.icon_starting_addr=SPO2Icon1;
  		}
  					break;

  		case SENSE_TEMP:{
  			mid_sec_icon.icon_starting_addr=temperature;
  		}
  					break;

  		case QUICK_TEST:
  			mid_qt_text4.text_status        = display;
  			mid_qt_text5.text_status        = display;

  			mid_qt_text4.text_starting_addr = "Test starts ";
  			mid_qt_text4.color       = BLUE;

  			mid_qt_text5.text_starting_addr = "     in      ";
  			mid_qt_text5.color       = BLUE;
  			mid_qt_text6.text_status        = FALSE;

  			break;
  		case ECG_12_LEAD:
  			mid_sec_icon.icon_starting_addr=ECGIcon1;

  			break;
  		default : break;

  	}

  	mid_sec_icon.color       = BLUE;
  	mid_sec_icon.icon_status = ON;
  	if(test_type == ECG_12_LEAD)
  	{
  		mid_sec_text1.text_starting_addr = "  Capture  ";
  		mid_sec_text1.color              = BLUE;
  		mid_sec_text1.text_status        = display;

  		mid_sec_text2.text_starting_addr = "  begins   ";
  		mid_sec_text2.color              = BLUE;
  		mid_sec_text2.text_status        = display;
  	}
  	else
  	{
  	mid_sec_text1.text_starting_addr = "Test starts ";
  	mid_sec_text1.color              = BLUE;
  	mid_sec_text1.text_status        = display;

  	mid_sec_text2.text_starting_addr = "     in      ";
  	mid_sec_text2.color              = BLUE;
  	mid_sec_text2.text_status        = display;
  	}
  	btm_icon.icon_status = FALSE;
  	btm_text.text_starting_addr = "    EXIT    ";
  	btm_text.color              = WHITE;
  	btm_text.text_status        = display;

  	for(tick_num = countdown_time; (tick_num > 0); tick_num--)
  	{
  		for(uint8_t loop=0;loop<10;loop++)
  		{

  		    Delay_ms(100);

  		    if(1)
  		    //if(WAKE_UP_Get_TestSkip_Flag() == TRUE)
  				{
  					ret_msg = TRUE;
  					break; //user exit
  				}
  		}

  		if(ret_msg == TRUE)
  		{
  			break;
  		}

  		if(tick_num == countdown_time)
  		{
  			if(test_type != QUICK_TEST)
  			{
  				API_Clear_Display(MIDDLE_SEC,WHITE);
  				API_Display_Bottom_Section(btm_icon , btm_text);
  				API_Display_Middle_Section(mid_sec_icon, mid_sec_text1, mid_sec_text2,mid_sec_text3);
  			}
  			if(test_type == QUICK_TEST)
  			{
  				API_Clear_Display(MIDDLE_SEC,WHITE);
  				API_Display_Bottom_Section(btm_icon , btm_text);
  				API_Disp_Display_Text(mid_qt_text1,mid_qt_text2,mid_qt_text3,mid_qt_text4,mid_qt_text5,mid_qt_text6,mid_qt_text7);
  			}
  		}

  		IntergerToString((char*)string_time,tick_num);
  		API_Disp_Dsplay_Char_With_Offset(120,55,string_time,BLUE);


  	 }

  	return ret_msg;
  }

  void API_Disp_Dsplay_Char_With_Offset(uint8_t row,uint8_t col,char *string,uint16_t color)
 {
 	char str_ptr[12] = {0};
 	if(Get_strlen(string) < 2) // padding the left char if only one char available
 	{
       str_ptr[0] = ' ';
       str_ptr[1] = *string;
       str_ptr[2] = '\0';
 	}

 	else
 	{
 		MemCpy((void*)str_ptr,(void*)string,Get_strlen(string));
 	}

     api_disp_display_char(str_ptr, font19x10, color, WHITE, col, row);

 }


  void API_Disp_Quick_test_screen(DISP_QUICK_TEST_SCREENS_t disp_qt_screen)
  {
		API_IO_Exp1_P1_write_pin(ECG_CSN,HIGH);

  	struct DISPLAY_TEXT mid_text1,mid_text2,mid_text3,mid_text4,mid_text5,mid_text6,mid_text7,btm_text;
  	struct DISPLAY_ICON btm_icon;

  	mid_text1.text_status = FALSE;
  	mid_text2.text_status = FALSE;
  	mid_text3.text_status = FALSE;
  	mid_text4.text_status = display;
  	mid_text5.text_status = display;
  	mid_text6.text_status = display;
  	mid_text7.text_status = FALSE;

	mid_text1.color = BLUE;
	mid_text2.color = BLUE;
	mid_text3.color = BLUE;
  	mid_text4.color = BLUE;
  	mid_text5.color = BLUE;
  	mid_text6.color = BLUE;

  	mid_icon.color          = BLUE;
  	mid_icon.icon_status    = ON;

  	switch(disp_qt_screen)
  	{
  		case DISP_QT_SCREEN:

  			mid_text5.text_starting_addr = " Quick Test ";
  			mid_text5.color              = BLUE;
  			mid_text6.text_status        = FALSE;
  			mid_text4.text_status        = FALSE;

  			break;


  		case DISP_QT_PLACE_FINGER :
  			mid_text4.text_starting_addr = "   Place    ";
  			mid_text5.text_starting_addr = " fingers on";
  			mid_text6.text_starting_addr = " electrodes ";
  			bottom_icon.icon_status		 = FALSE;

  			break;

  		case DISP_QT_PLEASE_REGISTER_PID:
			mid_text1.text_starting_addr = "   Please   ";
			mid_text2.text_starting_addr = "Register PID";

			bottom_icon.icon_status		 = FALSE;
		  	mid_text1.text_status = display;
		  	mid_text2.text_status = display;
		  	mid_text3.text_status = FALSE;
		  	mid_text4.text_status = FALSE;
		  	mid_text5.text_status = FALSE;
		  	mid_text6.text_status = FALSE;
		  	mid_text7.text_status = FALSE;

		  	API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);
			break;

  		case DISP_QT_DETECTING_FINGER :

  			mid_text4.text_starting_addr = " Detecting  ";
  			mid_text5.text_starting_addr = "   Finger   ";
  			mid_text6.text_starting_addr = "            ";
  			mid_text6.color = WHITE;
  			mid_text6.text_status=display;
  			bottom_icon.icon_status		 = FALSE;
  			break;

  		case DISP_QT_TEST_IN_PROGRESS :

  			mid_text4.text_starting_addr = "  Test in   ";
  			mid_text5.text_starting_addr = "  Progress  ";
  			mid_text6.text_starting_addr = "            ";
  			bottom_icon.icon_status		 = FALSE;

  			btm_icon.icon_status        = FALSE;
  			btm_text.text_status        = display;
  			btm_text.text_starting_addr = "    Exit    ";
  			btm_text.color              = WHITE;

  			break;

  		case DISP_QT_ECG_L1_TEST_IN_PROGRESS :

  	  			mid_text4.text_starting_addr = "ECG-L1 Test ";
  	  			mid_text5.text_starting_addr = "In Progress ";
  	  			mid_text6.text_starting_addr = "            ";
  	  			bottom_icon.icon_status		 = FALSE;

  	  			btm_icon.icon_status        = FALSE;
  	  			btm_text.text_status        = display;
  	  			btm_text.text_starting_addr = "    Exit    ";
  	  			btm_text.color              = WHITE;

  	  			break;

  		case DISP_QT_ECG_L2_TEST_IN_PROGRESS :

  		  	  			mid_text4.text_starting_addr = "ECG-L2 Test ";
  		  	  			mid_text5.text_starting_addr = "In Progress ";
  		  	  			mid_text6.text_starting_addr = "            ";
  		  	  			bottom_icon.icon_status		 = FALSE;

  		  	  			btm_icon.icon_status        = FALSE;
  		  	  			btm_text.text_status        = display;
  		  	  			btm_text.text_starting_addr = "    Exit    ";
  		  	  			btm_text.color              = WHITE;

  		  	  			break;

  		case DISP_QT_PPG_TEST_IN_PROGRESS :

				mid_text4.text_starting_addr = "PPG Test In ";
				mid_text5.text_starting_addr = " Progress   ";
				mid_text6.text_starting_addr = "            ";
				bottom_icon.icon_status		 = FALSE;

				btm_icon.icon_status        = FALSE;
				btm_text.text_status        = display;
				btm_text.text_starting_addr = "    Exit    ";
				btm_text.color              = WHITE;

				break;

  		case DISP_QT_PLACE_FINGER_PROPERLY :

  			mid_text4.text_starting_addr = "   Place    ";
  			mid_text5.text_starting_addr = "   Finger   ";
  			mid_text6.text_starting_addr = "  Properly  ";
  			bottom_icon.icon_status		 = FALSE;

  			break;

  		case DISP_12LEAD_CABLE_NOT_CONNECTED_PROPERLY :

			mid_text3.text_starting_addr = "  Lead 12   ";
			mid_text4.text_starting_addr = " Cables Not ";
			mid_text5.text_starting_addr = " Connected  ";
			mid_text6.text_starting_addr = "  Properly  ";

			mid_text3.text_status = display;

			bottom_icon.icon_status		 = FALSE;
			break;

  		case DISP_QT_RETAKE_TEST:
  			  mid_text4.text_starting_addr = " Retake the ";
  			  mid_text5.text_starting_addr = "    Test    ";
  			  mid_text6.text_status = erase;
  			  bottom_icon.icon_status		 = FALSE;
  			  break;

  		case DISP_QT_PLEASE_CARRYON_OTHER_VITALS:

  				mid_text4.text_starting_addr = "Please carry";
  				mid_text5.text_starting_addr = "  On with   ";
  				mid_text6.text_starting_addr = "Other vital ";
  				bottom_icon.icon_status		 = FALSE;
                  break;
  	}

  	if(disp_qt_screen == DISP_QT_SCREEN)
  	{
  	    api_disp_write_string("SPO2", 1, 6, WHITE, BLUE);
  	    api_disp_write_string(" HR ", 4, 7, WHITE, BLUE);
  	    api_disp_write_string("BP", 9, 6, WHITE, BLUE);

  	}

  	API_Disp_Display_Text(mid_text1,mid_text2,mid_text3,mid_text4,mid_text5,mid_text6,mid_text7);
  //	API_Display_Bottom_Section(btm_icon, btm_text);
  	if(disp_qt_screen == DISP_QT_SCREEN)
  	{
  		API_Clear_Display (DISP_BOTTOM_SEC ,BLUE);
  		btm_icon.icon_status = FALSE;
  		btm_icon.icon_starting_addr = STARTIcon1;
  		btm_icon.color = BLUE;
  		btm_text.text_status   = FALSE;

  	//	API_Display_Bottom_Section(btm_icon, btm_text);
  	}
  	else if((disp_qt_screen >= DISP_QT_PLACE_FINGER)&&(disp_qt_screen <= DISP_QT_PLEASE_CARRYON_OTHER_VITALS) &&(disp_qt_screen != DISP_QT_TEST_IN_PROGRESS))
  	{
  		API_Clear_Display(DISP_BOTTOM_SEC,BLUE);
  	}

  	Delay_ms(DISPLAY_TEST_SCEEN_TIMEOUT_MS);
  }

  void API_Disp_Display_Time(uint8_t data)
  {
  	struct DISPLAY_TEXT mid_text1,mid_text2,mid_text3,mid_text4,mid_text5,mid_text6,mid_text7;
  	char string1[12];
  	char string2[12] = "     ";

  	MemSet(string1,'\0',sizeof(string1));

  	if(data != 0)
  	{
  	 IntergerToString((char*)string1,data);
  	 StrCat(string2,string1);
  	}

  	else
  	{
  		StrCat(string2,"0");
  	}


  	mid_text6.text_starting_addr = "            ";
  	mid_text6.color              = BLUE;
  	mid_text6.text_status        = display;

  	mid_text1.text_status=FALSE;
  	mid_text2.text_status=FALSE;
  	mid_text3.text_status=FALSE;
  	mid_text4.text_status=FALSE;
  	mid_text5.text_status=FALSE;
  	mid_text7.text_status=FALSE;

  	API_Disp_Display_Text(mid_text1,mid_text2, mid_text3,mid_text4, mid_text5,mid_text6,mid_text7);


  	mid_text6.text_starting_addr = string2;

  	API_Disp_Display_Text(mid_text1,mid_text2, mid_text3,mid_text4, mid_text5,mid_text6,mid_text7);


  }


  void API_Disp_Quick_Test_Result(uint16_t result[])
  {
  	struct DISPLAY_TEXT mid_text1,mid_text2,mid_text3,mid_text4,mid_text5,mid_text6,mid_text7,btm_text;
  	struct DISPLAY_ICON btm_icon;

  	char string1[12] = {0};
  	char string2[12] = {0};
  	char string3[12] = {0};

  	//char string4[12] = {0};
  	char string5[12] = {0};
  	char string6[12] = {0};

  	MemSet(string1,'\0',sizeof(string1));
  	MemSet(string2,'\0',sizeof(string2));
  	MemSet(string3,'\0',sizeof(string3));
  	MemSet(string5,'\0',sizeof(string5));
  	MemSet(string6,'\0',sizeof(string6));

  	mid_text1.text_status = display;
  	mid_text2.text_status = display;
  	mid_text3.text_status = display;
  	mid_text4.text_status = display;
  	mid_text5.text_status = display;
  	mid_text6.text_status = display;
  	mid_text7.text_status = FALSE;


  	mid_text1.color       = BLUE;
  	mid_text2.color       = BLUE;
  	mid_text3.color       = BLUE;
  	mid_text4.color       = BLUE;
  	mid_text5.color       = BLUE;
  	mid_text6.color       = BLUE;



  	if((result[2] != 0) && (result[3] != 0))
  	{
  		mid_text5.text_starting_addr = "BP       ";

  		IntergerToString(string1,result[2]);
  		IntergerToString(string2,result[3]);
  		StrCat(string1, "/");
  		StrCat(string1,string2);
  		StrCat(string1," mmHg");
  	}
  	else
  	{
  		mid_text5.text_starting_addr = "BP          ";
  		StrCat(string1,"     X      ");
  		mid_text6.color  = RED;
  	}

  	MemSet(string2,'\0',sizeof(string2));

  	MemSet(string5,'\0',sizeof(string5));

  	if((result[1] != 0) && (result[1] != 0xFFFF))
  	{

  		mid_text3.text_starting_addr = "HR          ";

  		StrCat(string2,"   ");
  		IntergerToString(string5,result[1]);
  		StrCat(string2,string5);
  		StrCat(string2,"  bpm");
  	}
  	else
  	{
  		mid_text3.text_starting_addr = "HR          ";
  		StrCat(string2,"     X     ");
  		mid_text4.color = RED;

  	}

  	MemSet(string3,'\0',sizeof(string3));
  	MemSet(string6,'\0',sizeof(string6));

  	if((result[0] != 0))
  	{
  		mid_text1.text_starting_addr = "SpO2      ";

  		StrCat(string3,"    ");
  		IntergerToString(string6,result[0]);
  		StrCat(string3,string6);
  		StrCat(string3," %");
  	}
  	else
  	{
  		mid_text1.text_starting_addr = "SpO2        ";
  		StrCat(string3,"     X      ");
  		mid_text2.color       = RED;

  	}

  	mid_text2.text_starting_addr = string3;
  	mid_text4.text_starting_addr = string2;
  	mid_text6.text_starting_addr = string1;

  	API_Clear_Display(DISP_MIDDLE_SEC,WHITE);

  	#ifdef MARKETING_REQUIREMENT

  	API_Disp_Display_Text(mid_text1,mid_text2,mid_text3,mid_text4,mid_text5,mid_text6,mid_text7);

  	api_disp_draw_rectangle(0,20,128,57,BLACK);
  	api_disp_draw_rectangle(0,57,128,97,BLACK);
  	api_disp_draw_rectangle(0,97,128,140,BLACK);

  	btm_icon.icon_status   = FALSE;
  	btm_icon.color         = BLUE;

  	btm_text.text_starting_addr = "    Exit    ";
  	btm_text.text_status   		= TRUE;
  	btm_text.color         		= WHITE;
  	API_Clear_Display(DISP_BOTTOM_SEC , BLUE);
  	API_Display_Bottom_Section(btm_icon, btm_text);

  	#else
  	mid_text1.color       = WHITE;
  	mid_text2.color       = WHITE;
  	mid_text3.color       = WHITE;

#ifndef MARKETING_REQUIREMENT
  	mid_text1.text_starting_addr = " Test Done! ";
	mid_text1.color       = BLUE;
#endif
  	mid_icon.icon_status        = ON;
  	mid_icon.icon_starting_addr = ThumbsUpIcon1;
  	mid_icon.color              = BLUE;

  	API_Display_Middle_Section(mid_icon,mid_text1,mid_text2,mid_text3);

  	#endif


  }

  void API_Disp_Quick_Test_Icon(void)
  {
	  API_IO_Exp1_P1_write_pin(ECG_CSN,HIGH);

  	uint8_t left_offset = 10;
  	uint8_t top_offset  = 30;

  	API_Clear_Display(DISP_MIDDLE_SEC,WHITE);

  	api_disp_display_icon (SPO2Icon1, left_offset, top_offset, BLUE, WHITE);

  	left_offset = 90;
  	top_offset  = 30;

  	api_disp_display_icon (BPIcon1, left_offset, top_offset, BLUE, WHITE);

  	left_offset = 50;
  	top_offset  = 45;

  	api_disp_display_icon (ECGIcon1, left_offset, top_offset, BLUE, WHITE);


  }

  void API_DISP_Toggle_Date_Time(void)
  {
   	DATE_TIME disp_date_time;
   	char *str_month = '\0';
   	char date_time_info[15]={0};

   	MemSet(str_month,'\0',sizeof(str_month));
  	API_RTC_Get_Date_Time(date_time_info, "%S");

 // 	printf("\n Seconds = %s",date_time_info);

  	disp_date_time.Time.seconds = atoi(date_time_info);
  	API_RTC_Get_Date_Time(date_time_info, "%M");

  	disp_date_time.Time.minute = atoi(date_time_info);

  	API_RTC_Get_Date_Time(date_time_info, "%H");

  	disp_date_time.Time.hour = atoi(date_time_info);

  	API_RTC_Get_Date_Time(date_time_info, "%m");
  	disp_date_time.Date.month = atoi(date_time_info);;

  	API_RTC_Get_Date_Time(date_time_info, "%y");
  	disp_date_time.Date.year  = atoi(date_time_info);;

  	API_RTC_Get_Date_Time(date_time_info, "%d");
  	disp_date_time.Date.day   = atoi(date_time_info);;


   	 if(disp_date_time.Date.month == 1)       str_month = "Jan";
   	 else if(disp_date_time.Date.month == 2)  str_month = "Feb";
   	 else if(disp_date_time.Date.month == 3)  str_month = "Mar";
   	 else if(disp_date_time.Date.month == 4)  str_month = "Apr";
   	 else if(disp_date_time.Date.month == 5)  str_month = "May";
   	 else if(disp_date_time.Date.month == 6)  str_month = "Jun";
   	 else if(disp_date_time.Date.month == 7)  str_month = "July";
   	 else if(disp_date_time.Date.month == 8)  str_month = "Aug";
   	 else if(disp_date_time.Date.month == 9)  str_month = "Sep";
   	 else if(disp_date_time.Date.month == 10) str_month = "Oct";
   	 else if(disp_date_time.Date.month == 11) str_month = "Nov";
   	 else if(disp_date_time.Date.month == 12) str_month = "Dec";

   	if(Is_time_displayed == FALSE)
   	{
   		api_disp_display_time(disp_date_time.Time.hour,disp_date_time.Time.minute);
   		Is_time_displayed = TRUE;

   	}
   	else
   	{
   		Is_time_displayed = FALSE;
   		api_disp_display_date(disp_date_time.Date.day,str_month);
   	}

   }

  void API_Disp_Reset_Screen(void)
  {
	   API_Clear_Display (DISP_TOP_SEC, BLUE);
	   API_Clear_Display (DISP_MIDDLE_SEC, WHITE);
	   API_Clear_Display (DISP_BOTTOM_SEC, BLUE);
  }

  void API_DISP_Firmware_Version(void)
  {
		API_IO_Exp1_P1_write_pin(ECG_CSN,HIGH);

 		api_disp_display_char( " FW Version ", font19x10, BLUE, BLACK, 5, 80);
 		api_disp_display_char(FIRMWARE_VERSION, font19x10, BLUE, BLACK, 5, 100);
  }



bool API_DISP_Memory_Full_Status(void)
{
		struct DISPLAY_TEXT mid_sec_text1,mid_sec_text2,mid_sec_text3,mid_sec_text4,mid_sec_text5,mid_sec_text6,mid_sec_text7;

		bool check_status = TRUE;

		mid_sec_text2.text_starting_addr = "MEMORY FULL";
		mid_sec_text2.color = BLUE;
		mid_sec_text2.text_status = display;

		mid_sec_text3.text_starting_addr = "PLEASE SYNC ";
		mid_sec_text3.color = BLUE;
		mid_sec_text3.text_status = display;

		mid_sec_text4.text_starting_addr = "   DATA     ";
		mid_sec_text4.color = BLUE;
		mid_sec_text4.text_status = display;

		mid_sec_text1.text_status = FALSE;
		mid_sec_text5.text_status = FALSE;
		mid_sec_text6.text_status = FALSE;
		mid_sec_text7.text_status = FALSE;

		API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);

		API_Disp_Display_Text(mid_sec_text1,mid_sec_text2, mid_sec_text3,mid_sec_text4, mid_sec_text5,mid_sec_text6,mid_sec_text7);
		API_Clear_Display(DISP_BOTTOM_SEC , BLUE);
		Delay_ms(DISP_NOTIFICATION_TIME);

		return check_status;
}


  void API_DISP_Update_Battery_Status(BATTERY_PONINTS_t points,bool charging, bool empty,uint16_t color)
  {

  	if(charging == TRUE)
  	{
  		api_disp_display_icon(batteryCharging, DISP_TOP_SEC_BAT_COL_START_ADDR, DISP_TOP_SEC_BAT_ROW_START_ADDR, color, BLUE);
  	}

  	else if(empty == TRUE)
  	{
  		api_disp_display_icon(batteryEmpty, DISP_TOP_SEC_BAT_COL_START_ADDR, DISP_TOP_SEC_BAT_ROW_START_ADDR, color, BLUE);
  	}

  	else
  	{
  		switch(points)
  			{
  				case  DISP_ONE_STICK :
  						{
  							api_disp_display_icon(batteryLow, DISP_TOP_SEC_BAT_COL_START_ADDR, DISP_TOP_SEC_BAT_ROW_START_ADDR, color, BLUE);
  							break;
  						}

  				case  DISP_TWO_STICK :
  						{
  							api_disp_display_icon(batteryMid, DISP_TOP_SEC_BAT_COL_START_ADDR, DISP_TOP_SEC_BAT_ROW_START_ADDR, color, BLUE);
  							break;
  						}

  				case  DISP_THREE_STICK :
  						{
  							api_disp_display_icon(batteryFull, DISP_TOP_SEC_BAT_COL_START_ADDR, DISP_TOP_SEC_BAT_ROW_START_ADDR, color, BLUE);
  							break;
  						}
  			}
  	}

  }

  void API_DISP_Error_Code(uint32_t Error)
	{

	  char text[15];

	struct DISPLAY_TEXT text1,text2,text3,text4,text5,text6,text7;


	IntergerToString(text, Error);
	text6.text_starting_addr = text;
	text6.color = RED;
	text6.text_status = display;

	text1.text_status = 0;
	text2.text_status = 0;
	text3.text_status = 0;
	text4.text_status = 0;
	text5.text_status = 0;
	text7.text_status = 0;
	  API_Clear_Display (DISP_MIDDLE_SEC, WHITE);
	API_Disp_Display_Text(text1,text2,text3,text4,text5,text6,text7);
	//Delay_ms(5000);

}

  bool API_Check_USB_Charger_Connection_Display_Notification(void)
  {
	  bool status = false;

       if(IsUSB_Charger_Connected())
       {

   		struct DISPLAY_TEXT mid_sec_text1,mid_sec_text2,mid_sec_text3,mid_sec_text4,mid_sec_text5,mid_sec_text6,mid_sec_text7;

   		mid_sec_text2.text_starting_addr = "   PLEASE   ";
   		mid_sec_text2.color = BLUE;
   		mid_sec_text2.text_status = display;

   		mid_sec_text3.text_starting_addr = " REMOVE THE ";
   		mid_sec_text3.color = BLUE;
   		mid_sec_text3.text_status = display;

   		mid_sec_text4.text_starting_addr ="   CHARGER  ";
   		mid_sec_text4.color = BLUE;
   		mid_sec_text4.text_status = display;

   		mid_sec_text1.text_status = FALSE;
   		mid_sec_text5.text_status = FALSE;
   		mid_sec_text6.text_status = FALSE;
   		mid_sec_text7.text_status = FALSE;

   		API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);

   		API_Disp_Display_Text(mid_sec_text1,mid_sec_text2, mid_sec_text3,mid_sec_text4, mid_sec_text5,mid_sec_text6,mid_sec_text7);
   		API_Clear_Display(DISP_BOTTOM_SEC , BLUE);
   		Delay_ms(DISP_NOTIFICATION_TIME);
   		status = true;
       }

       return status;
  }

  void API_Disp_BT_Icon(uint16_t icon_color)
  {
	  struct DISPLAY_ICON display_BT_icon;
	  struct DISPLAY_TEXT display_time_icon;
	  struct DISPLAY_ICON display_batt_icon;

	  display_BT_icon.icon_starting_addr=BTIcon1;
	  display_BT_icon.icon_status=ON;
	  display_BT_icon.color=icon_color;

	  display_time_icon.text_status = FALSE;
	  display_batt_icon.icon_status = FALSE;

	  API_Display_Top_Section (display_BT_icon,display_time_icon,display_batt_icon);

  }

void API_Disp_Lead_Count(uint8_t lead)
{
	struct DISPLAY_TEXT mid_text1,mid_text2,mid_text3,bottom_text;
 	char string1[12] = {0};
 	char string2[12] = {0};

	mid_icon.icon_status    = false;
	bottom_icon.icon_status = false;

	mid_text1.color  = BLUE;
	mid_text2.color  = BLUE;
	mid_text3.color  = BLUE;

	mid_text1.text_status = display;
	mid_text2.text_status = display;
	mid_text3.text_status = display;
	bottom_text.text_status = FALSE;

	IntergerToString(string1,lead);
	StrCat(string2, "   Lead:");
	StrCat(string2,string1);
	StrCat(string2,"  ");

	mid_text1.text_starting_addr =string2;
	mid_text2.text_starting_addr = " Capture In ";
	mid_text3.text_starting_addr = "  Progress  ";


  	API_Display_Middle_Section(mid_icon,mid_text1,mid_text2,mid_text3);

}


void API_DISP_SenseSemi_Logo(SENSESEMI_LOGO_t moving_or_static)
{
	API_IO_Exp1_P1_write_pin(ECG_CSN,HIGH);


	struct DISPLAY_TEXT mid_text1,mid_text2,mid_text3;
	struct DISPLAY_ICON mid_icon;
	mid_text1.text_status = FALSE;
	mid_text2.text_status = FALSE;
	mid_text3.text_status = FALSE;

	uint8_t row_count = 0x00;
	uint8_t row_start = 20;
	uint8_t row_end   = 80;
	uint8_t col_start = 0;

	uint8_t buff_nbf_rows = logo[0];
	uint8_t buff_nbf_cols = logo[1];

		if(moving_or_static==MOVING_IMAGE)
		{
			for(row_count=0;row_count<row_end;row_count++)
			 {
				 api_disp_set_pointer_driver_side(col_start,DISP_MAX_COLS,row_start,DISP_MAX_ROWS);
				// api_disp_tx_buffer(logo+2,buff_nbf_rows * buff_nbf_cols);
				 row_start++;
			 }

			for(row_count=0;row_count<row_end;row_count++)
			{
				api_disp_set_pointer_driver_side(col_start,DISP_MAX_COLS,row_start,DISP_MAX_ROWS);
				// api_disp_tx_buffer(logo+2,buff_nbf_rows * buff_nbf_cols);
				row_start--;
			}

		}

		if(moving_or_static==STATIC_IMAGE)
		{
			 API_Clear_Display (DISP_TOP_SEC,BLACK);
			 API_Clear_Display (DISP_MIDDLE_SEC,BLACK);
			 API_Clear_Display (DISP_BOTTOM_SEC,BLACK);

			 api_disp_set_pointer_driver_side(col_start,DISP_MAX_COLS,row_start,DISP_MAX_ROWS);
		 	 api_disp_display_icon (logo, 40, 30,BLUE, BLACK);

			 //api_disp_tx_buffer(logo+2,buff_nbf_rows * buff_nbf_cols);
		}
 }


void API_Disp_Display_Lead_Connection_Status(uint8_t leadNumber)
{
	struct DISPLAY_TEXT mid_sec_text1,mid_sec_text2,mid_sec_text3,mid_sec_text4,mid_sec_text5,mid_sec_text6,mid_sec_text7;


	if(leadNumber == 1)
	{
		mid_sec_text2.text_starting_addr = " Lead1 Not  ";
	}

	else if(leadNumber == 2)
	{
		mid_sec_text2.text_starting_addr = " Lead2 Not  ";

	}

	else if(leadNumber == 12)
	{
		mid_sec_text2.text_starting_addr = " Lead12 Not ";
	}

	mid_sec_text3.text_starting_addr = " Connected  ";
	mid_sec_text4.text_starting_addr = " Properly   ";

	mid_sec_text2.color = BLUE;
	mid_sec_text2.text_status = display;

	mid_sec_text3.color = BLUE;
	mid_sec_text3.text_status = display;

	mid_sec_text4.color = BLUE;
	mid_sec_text4.text_status = display;

	mid_sec_text1.text_status = FALSE;
	mid_sec_text5.text_status = FALSE;
	mid_sec_text6.text_status = FALSE;
	mid_sec_text7.text_status = FALSE;

	API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);

	API_Disp_Display_Text(mid_sec_text1,mid_sec_text2, mid_sec_text3,mid_sec_text4, mid_sec_text5,mid_sec_text6,mid_sec_text7);
	API_Clear_Display(DISP_BOTTOM_SEC , BLUE);
	Delay_ms(DISP_NOTIFICATION_TIME);
}

void API_Disp_Display_Exit_Bottom_Section(void)
{
	  struct DISPLAY_TEXT mid_sec_text1,mid_sec_text2,mid_sec_text3,mid_sec_text4,mid_sec_text5,mid_sec_text6,mid_sec_text7;

		mid_sec_text7.text_starting_addr = "    Exit    ";
		mid_sec_text7.color = WHITE;
		mid_sec_text7.text_status = display;


		mid_sec_text1.text_status = FALSE;
		mid_sec_text2.text_status = FALSE;
		mid_sec_text3.text_status = FALSE;
		mid_sec_text4.text_status = FALSE;

		mid_sec_text5.text_status = FALSE;
		mid_sec_text6.text_status = FALSE;

		API_Clear_Display(DISP_BOTTOM_SEC , BLUE);

		API_Disp_Display_Text(mid_sec_text1,mid_sec_text2, mid_sec_text3,mid_sec_text4, mid_sec_text5,mid_sec_text6,mid_sec_text7);
}

VITAL_TYPE_t API_Disp_Select_PID_Screen(void)
{

	char pidArray[12]={'\0'};

	VITAL_TYPE_t ret_msg=0xFF;
	uint8_t top_offset = 20;
	uint8_t count = 1;
	uint8_t left_offset = 2;
	uint8_t btn_press;

	bool is_Pid_Displayed = false;

  	struct DISPLAY_TEXT text1,text2,text3,text4,text5,text6,text7;

  	text1.color= BLUE;
  	text1.text_starting_addr = "  Register  ";
  	text1.text_status = display;

  	text2.color = BLUE;
  	text2.text_starting_addr = "    PID     ";

	text2.text_status = display;

  	text4.color = BLUE;
  	text4.text_starting_addr = " Enter      ";
  	text4.text_status = 0;

	text5.color = BLUE;
  	text5.text_starting_addr = " Exit       ";
  	text5.text_status = display;


	text3.text_status = 0;
  	text6.text_status = 0;
  	text7.text_status = 0;
	if(Selected_PID_type != VALID_PID)
	{
		API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);
		API_Disp_Display_Text(text1, text2, text3, text4, text5, text6, text7);
	  	API_Clear_Display(DISP_BOTTOM_SEC,BLUE);
	}

  	API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);
  	API_TIMER_Kill_Timer(TEST_ENTERY_TIMEOUT);
  	printf("\n\nIn View Screen\n");

	btn_press = API_Push_Btn_Get_Buttton_Press(); // Dummy read
	api_disp_display_icon(star,left_offset,top_offset,RED,WHITE);

  	while(1)
  	{

  		btn_press = API_Push_Btn_Get_Buttton_Press();


		if(btn_press == 3)
		{
			esp_restart();
		}

  		if((btn_press == 1) || ((btn_press == 2)))
			{
			    API_TIMER_Register_Timer(USER_INACTIVE_TIMEOUT);
				API_TIMER_Register_Timer(TEST_ENTERY_TIMEOUT);//timer register to refresh timer in order to start timer freshly

				Test_Exit_Flag = FALSE;
				api_disp_display_icon(star,left_offset,top_offset,WHITE,WHITE);
				count++;

				if(Selected_PID_type != VALID_PID)
				{

					if(count == 2)
					{
						top_offset = top_offset + 80;
					}

					else
					{
						count=1;
						top_offset=20;
						API_TIMER_Kill_Timer(TEST_ENTERY_TIMEOUT);
					}
				}

				else
				{
					if(count == 2)
					{
						top_offset = top_offset + 40;
					}
					if (count == 4)
					{
						count = 1;
						left_offset = 2;
						top_offset = 20;
						API_TIMER_Kill_Timer(TEST_ENTERY_TIMEOUT);
					}

					else
					{
						top_offset = top_offset + 20;
					}
				}

				api_disp_display_icon(star,left_offset,top_offset,RED,WHITE);
			}

  		if(API_TIMER_Get_Timeout_Flag(TEST_ENTERY_TIMEOUT)) // call get_time_out function
		{
			break;
		}

  		if(API_TIMER_Get_Timeout_Flag(USER_INACTIVE_TIMEOUT) == TRUE) // call get_time_out function
  		{
  			flash_data.sys_mode = DEVICE_HIBERNET_MODE;
  			count=0;
  			break;
  		}

  		if (is_firmware_data_available())
		{
		  count = 0;
           break;
		}

  		if(API_TIMER_Get_Timeout_Flag(DATE_TIME_FLIP_TIME) == TRUE) // call get_time_out function
  		{
			API_DISP_Toggle_Date_Time();
			API_TIMER_Register_Timer(DATE_TIME_FLIP_TIME);
  		}


  		if(Data_sync_in_progress)
  			{
  				count = 0xFF;
  				break;
  			}


  		if(Is_Device_Paired == BT_PAIRED) // Paired condition
  		{
  			API_Disp_BT_Icon(GREEN);
  			Is_Device_Paired = DEFAULT;// to avoid Redisplaying the same thing again
  		}

  		else if(Is_Device_Paired == BT_DISCONNECTED) // disconnected condition
		{
			API_Disp_BT_Icon(WHITE);
			Is_Device_Paired = DEFAULT;
		}

  		if((Selected_PID_type == VALID_PID) && (!is_Pid_Displayed))
  		{
  			pidArray[0]=' ';
  			MemCpy(pidArray+1,(char*)(PatientID.pid+10),8);
  			pidArray[11]='\0';

  		  	text1.text_starting_addr = "   Select   ";

			text3.text_starting_addr =pidArray;
			text3.text_status = display;
		  	text4.text_status = display;
		  	text5.text_status = display;
		  	API_Clear_Display (DISP_MIDDLE_SEC ,WHITE);

		  	API_Disp_Display_Text(text1, text2, text3, text4, text5, text6, text7);
			api_disp_display_icon(star,left_offset,top_offset,RED,WHITE);

			is_Pid_Displayed = true;

			printf("\n Char Select PID............................................................\n");
			for(int i=0;i<10;i++)
			{
				printf("%c",pidArray[i]);
			}
			printf("\nInt Select PID............................................................\n");
			for(int i=0;i<10;i++)
			{
				printf("%d",pidArray[i]);
			}

  		}
  	}

  	if((count==2) && (Selected_PID_type != VALID_PID))
  	{
  		return TEST_EXIT;
  	}
  	switch(count)
  	  	{
  			case 2 : ret_msg = VIEW_SCREEN; break;
  			case 3 : ret_msg = TEST_EXIT; break;
  			case 0xFF :ret_msg = DATA_SYNC; break;
  			default: ret_msg = NO_TEST; break;
  	  	}

	return ret_msg;

}
