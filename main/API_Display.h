#ifndef API_DISPLAY_H
#define API_DISPLAY_H
#include <stdbool.h>
#include <stdint.h>

#include "API_Flash_org.h"

#define LCD_ORIENTATION0    0
#define LCD_ORIENTATION1    96
#define LCD_ORIENTATION2    160
#define LCD_ORIENTATION3    192

#define DISPLAY_SLEEP_MODE_CMD 0x10

#define BLACK           0x0000      //  0, 0, 0
#define WHITE           0xFFFF      // 31,31,31
#define BLUE         0xF800//  0x001F       // 31, 0, 0
#define GREEN           0x07C0      //  0,31, 0
#define NAVY            0x000F      //  0, 0,15
#define DARKGREEN       0x03C0      //  0,15, 0
#define DARKCYON        0x03EF      //   0, 128,128
#define MAROON          0x7800      // 15, 0, 0
#define PURPLE          0x780F      // 128,   0, 128
#define OLIVE           0x7BE0      // 128, 128,   0
#define LIGHTGREY       0xC618      // 192, 192, 192
#define DARKGREY        0x7BEF      // 128, 128, 128
#define RED            0x001F// 0xF800      // 0, 0,31
#define CYON            0x07DF      //  0,31,31
#define YELLOW          0xF81F       // 31, 0,31
#define MAGENTA         0xFFC0      // 31,31, 0
#define ORANGE          0xFD20      // 255, 165,   0
#define GREENYELLOW     0xAFE5      // 173, 255,  47
#define LIGHTBLUE       0x04FF

#define TOTAL_ASCII_CHARACTERS  95

#define DISP_RESULT_TIME		  		5
#define DISP_QUICK_TEST_RESULT_TIME		12
#define DISP_NOTIFICATION_TIME	  		3
#define DISP_TOTAL_CHARACTERS_PER_LINE 12


typedef enum{
	BT_ICON = 0,
	BATTERY_ICON_UNDER_CHG,
	BATTERY_ICON_WITHOUT_CHG,
	MIDDLE_SECTION_ICON,
	BOTTOM_SECTION_ICON,
	TOTAL_NBR_ICON_TYPE
} ICON_TYPE;

typedef enum {
	TIME_TEXT = 0,
	BATTERY_CHARGE_TEXT,
	MIDDLE_SECTION_FIRST_ROW,
	MIDDLE_SECTION_SECOND_ROW,
	MIDDLE_SECTION_THIRD_ROW,
	BOTTOM_SECTION_TEXT,
	PATIENT_SELECTION_TEXT,
	TOTAL_NBR_TEXT_TYPE
} TEXT_TYPE;

typedef enum
{
	ON = 1,
	OFF,

} ICON_STATUS;

typedef enum
{
	display = 1,
	erase
}TEXT_STATUS;

// structure definition
struct DISPLAY_ICON
{
   const	uint8_t	*icon_starting_addr;
	 uint16_t	color;
	ICON_TYPE	icon_type;
	ICON_STATUS	icon_status;

};

struct DISPLAY_TEXT
{
	char	*text_starting_addr;

	 uint16_t	color;
	TEXT_TYPE	text_type;
	TEXT_STATUS	text_status;
};

typedef enum
{

	DISP_ONE_STICK = 1,
	DISP_TWO_STICK,
	DISP_THREE_STICK,

} BATTERY_PONINTS_t;

typedef enum
{
    MOVING_IMAGE = 0,
	STATIC_IMAGE
} SENSESEMI_LOGO_t;
typedef enum
{
  TOP_SEC = 1,
  MIDDLE_SEC,
  BOTTOM_SEC

} CLEAR_SEC_t;


typedef enum
{
  BP_TEST   = 1,
  ECG_TEST  = 2,
  BG_TEST   = 4,
  TEMP_TEST = 8,
  SPO2_TEST = 16
} TEST_COMPLETED_t;


typedef enum
{
   DISP_TOP_SEC = 1,
   DISP_MIDDLE_SEC,
   DISP_BOTTOM_SEC

}DISP_SEC_t;


typedef enum
{
	GUEST_PID = 1,
	VALID_PID ,
	PID_NOT_SELECTED = 0xFF,

} PID_TYPE_t;


typedef enum
{
	DISP_PID_SCREEN = 1,
	DISP_BP_SCREEN,
	DISP_ECG_SCREEN,
	DISP_BG_SCREEN,
	DISP_SPO2_SCREEN,
	DISP_TEMP_SCREEN,
	DISP_ECG_12_LEAD_SCREEN,
	DISP_PLACE_FINGERS_ON_ELECTRODES,
	DISP_UNSUCCESSFULL,
	DISP_SUCCESSFULL,
	DISP_TEST_IN_PROGRESS,
	V1_LEAD_CONNECTED,
	V2_LEAD_CONNECTED,
	V3_LEAD_CONNECTED,
	V4_LEAD_CONNECTED,
	V5_LEAD_CONNECTED,
	V6_LEAD_CONNECTED,
	DISP_V_LEAD_CAPTURE_IN_PROGRESS,
	DISP_V1_LEAD_CAPTURE_IN_PROGRESS,
	DISP_V2_LEAD_CAPTURE_IN_PROGRESS,
	DISP_V3_LEAD_CAPTURE_IN_PROGRESS,
	DISP_V4_LEAD_CAPTURE_IN_PROGRESS,
	DISP_V5_LEAD_CAPTURE_IN_PROGRESS,
	DISP_V6_LEAD_CAPTURE_IN_PROGRESS,
	DISP_V_LEAD_CAPTURE_SUCCESSFULL,
	DISP_V1_LEAD_CAPTURE_SUCCESSFULL,
	DISP_V2_LEAD_CAPTURE_SUCCESSFULL,
	DISP_V3_LEAD_CAPTURE_SUCCESSFULL,
	DISP_V4_LEAD_CAPTURE_SUCCESSFULL,
	DISP_V5_LEAD_CAPTURE_SUCCESSFULL,
	DISP_V6_LEAD_CAPTURE_SUCCESSFULL,
	DISP_INSERT_STRIP_PALCE_A_DROP_OF_BLOOD,
	DISP_STRIP_OR_BLOOD_NOT_DETECTED,
	DISP_PLACE_FINGER,
	DISP_PLACE_FINGER_PROPERLY,
	DISP_REPLACE_LEAD2_PROPERLY,
	DISP_DETECTING_FINGER,
	DISP_PALCE_ELECTRODE_ON_FOREHEAD,
	DISP_DEVICE_UPGRADING,
	DISP_DEVICE_UPGRADED,
	DISP_DEVICE_UPGRADATION_FAIL,
	DISP_PID_NOT_SELECTED,
	DISP_PLEASE_CARRYON_OTHER_VITALS,
	DISP_DATA_SYNC_IN_PROGRESS,
	DISP_FINGER_NOT_DETECTED,
	DISP_TAKE_TEST_FROM_STD_DEVICE,
	DISP_LOW_VOLTAGE_PLEASE_CHARGE,
	DISP_VERY_LOW_VOLTAGE_PLEASE_CHARGE,
	DISP_DEVICE_RESETTING,
	DISP_RETAKE_TEST,
	DISP_INSERT_TEST_STRIP,
	DISP_TEST_STRIP_INSERTED,
	DISP_STRIP_REMOVED,
	DISP_BLOOD_DETECTED,
	COUNTDOWN,
	DISP_RESULT_SCREEN,
	PALCE_A_DROP_OF_BLOOD,
	DISP_DEVICE_SLEEP,
	DISP_DEVICE_ACTIVE,
	DISP_PLEASE_SYNC_TIME,
	DISP_PLACE_ELECTRODE_ON_V_LEAD,
	DISP_PLACE_ELECTRODE_ON_V1_LEAD,
	DISP_PLACE_ELECTRODE_ON_V2_LEAD,
	DISP_PLACE_ELECTRODE_ON_V3_LEAD,
	DISP_PLACE_ELECTRODE_ON_V4_LEAD,
	DISP_PLACE_ELECTRODE_ON_V5_LEAD,
	DISP_PLACE_ELECTRODE_ON_V6_LEAD,
	DISP_RETRY,
	DISP_CONNECT_ELECTRODE_PROPERLY,
	DISP_TEST_START,
	DISP_DATA_SYNC_FAIL,
	DISP_DATA_SYNC_COMPLETED,

}DISP_SCREENS_t;
typedef enum
{
	DISP_QT_SCREEN,
	DISP_QT_PLACE_FINGER,
	DISP_QT_TEST_IN_PROGRESS,
	DISP_QT_PLACE_FINGER_PROPERLY,
	DISP_QT_DETECTING_FINGER,
	DISP_QT_RETAKE_TEST,
	DISP_QT_PLEASE_REGISTER_PID,
	DISP_QT_PLEASE_CARRYON_OTHER_VITALS,
	DISP_12LEAD_CABLE_NOT_CONNECTED_PROPERLY,
	DISP_QT_ECG_L1_TEST_IN_PROGRESS,
	DISP_QT_ECG_L2_TEST_IN_PROGRESS,
	DISP_QT_PPG_TEST_IN_PROGRESS

}DISP_QUICK_TEST_SCREENS_t;


typedef enum{
	QUICK_VITALS=1,
	MULTI_VITALS,
	NO_TEST,
	TEST_ENTER,
	TEST_EXIT,
	DATA_SYNC,
	VIEW_SCREEN
}SELECTED_TEST_t;


extern bool Test_Exit_Flag;
extern PID_TYPE_t   Selected_PID_type;
extern bool Is_time_displayed;


void disp_gpio_config();
uint8_t api_disp_display_char (const char *string, const uint8_t *Font, uint16_t FontColor, uint16_t BGColor, uint8_t x, uint8_t y);

void API_Display_spi_init(void);
void API_Display_spi_Deinit(void);

void API_Display_setup(void);
bool API_Display_interface_init(void);
void API_DISP_Clear_Full_Screen(uint16_t color);
void API_DISP_Clear_Screen_Fast(uint16_t color);
void api_icon_test(void);
void API_Disp_Display_Text(struct DISPLAY_TEXT display_first_row,struct  DISPLAY_TEXT display_second_row, struct DISPLAY_TEXT display_third_row,struct  DISPLAY_TEXT display_fourth_row, struct DISPLAY_TEXT display_fifth_row,struct DISPLAY_TEXT display_six_row,struct DISPLAY_TEXT display_seventh_row);
void API_DISP_Clear_Full_Screen_3_Wire(uint16_t color);

void api_disp_write_com(uint8_t cmd);
void api_disp_write_data(uint8_t data);
void disp_text();

/*****************************************************************************************/
void API_Display_Top_Section (struct DISPLAY_ICON display_BT_icon, struct DISPLAY_TEXT display_time_icon, struct DISPLAY_ICON display_batt_icon);
void API_Display_Middle_Section (struct DISPLAY_ICON display_middle_section_icon, struct DISPLAY_TEXT display_mid_sec_first_row,struct  DISPLAY_TEXT display_mid_sec_second_row, struct DISPLAY_TEXT display_mid_sec_third_row);
void API_Display_Bottom_Section (struct DISPLAY_ICON display_bot_sec_icon, struct DISPLAY_TEXT display_bot_sec_text);
void API_DISP_Display_Screen(DISP_SCREENS_t display_screen);
void API_Clear_Display (DISP_SEC_t disp_sec ,uint16_t color);
void API_Select_PID();
void API_Hold_Select_PID_State();
VITAL_TYPE_t API_Display_View_Screen();
bool API_DISP_Wait_Time(VITAL_TYPE_t test_type,uint8_t countdown_time);
void API_Disp_Dsplay_Char_With_Offset(uint8_t row,uint8_t col,char *string,uint16_t color);
void API_Disp_Quick_test_screen(DISP_QUICK_TEST_SCREENS_t disp_qt_screen);
void API_Disp_Display_Time(uint8_t data);
void API_Disp_Quick_Test_Result(uint16_t result[]);
void API_Disp_Reset_Screen(void);
void API_DISP_Toggle_Date_Time(void);
void API_DISP_Firmware_Version(void);
bool API_DISP_Memory_Full_Status(void);
void API_DISP_Update_Battery_Status(BATTERY_PONINTS_t points,bool charging, bool empty,uint16_t color);

void API_DISP_Error_Code(uint32_t Error);
void API_Disp_Quick_Test_Icon(void);
bool API_Check_USB_Charger_Connection_Display_Notification(void);
void API_Disp_BT_Icon(uint16_t icon_color);
void API_Disp_Lead_Count(uint8_t lead);
void API_DISP_SenseSemi_Logo(SENSESEMI_LOGO_t moving_or_static);
void API_Disp_Display_Lead_Connection_Status(uint8_t leadNumber);
void API_Disp_Display_Exit_Bottom_Section(void);
VITAL_TYPE_t API_Disp_Select_PID_Screen(void);

#endif
