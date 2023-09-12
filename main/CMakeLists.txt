# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "API_ADS.c" "API_Flash.c" "API_Flash_org.c" "rtc.c" 
	 "API_timer.c" "API_adc.c" "API_Display.c" "API_utility.c" "bpf.c"
	 "ecg_config.c" "max86150.c" "API_IO_Exp.c" "push_button.c" "API_Battery_monitor.c"
	 "main.c" "MainFlow.c" "Error_Handling.c" "Quick_Test.c" "API_Buzzer.c"
	 "OTA_Upgrade.c" "bluetooth.c" "API_Bluetooth.c" "crc_16.c" "ECG_12_Lead.c"
	 "Battery_management.c")
	 
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
