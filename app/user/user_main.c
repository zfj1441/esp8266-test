/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_devicefind.h"
#include "user_webserver.h"
#include "user_sensor.h"
#include "user_plug.h"

#if ESP_PLATFORM
#include "user_esp_platform.h"
#endif

int zt=0;

void user_rf_pre_init(void)
{
}

LOCAL void ICACHE_FLASH_ATTR
user_plug_long_press(void)
{
	user_esp_platform_set_active(0);
    system_restore();
    system_restart();
}

LOCAL void ICACHE_FLASH_ATTR
user_plug_short_press(void)
{
	if(zt == 0)
	{
		GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1);
		zt = 1;
	}
	else
	{
		GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);
		zt = 0;
	}
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	//set wifi 
	struct station_config  stConfig;
	os_memcpy(stConfig.ssid, SSID, os_strlen(SSID));
	os_memcpy(stConfig.password, PASSWORD, os_strlen(PASSWORD));
	wifi_set_opmode(STATIONAP_MODE);
	wifi_station_set_config(&stConfig);
	
	// key gpio0 init;
	struct keys_param keys;
	struct single_key_param *single_key[PLUG_KEY_NUM];
	single_key[0] = key_init_single(PLUG_WIFI_LED_IO_NUM, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0,
                                    user_plug_long_press, user_plug_short_press);
	keys.key_num = PLUG_KEY_NUM;
	keys.single_key = single_key;
	key_init(&keys);
	
	// led gpio2 init;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);
	return ;
}
