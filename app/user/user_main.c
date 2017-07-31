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

LOCAL int zt=0;

void user_rf_pre_init(void)
{
}

void ICACHE_FLASH_ATTR
short_press(void)
{
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	if (gpio_status & BIT(0)) 
	{
		//disable interrupt
	        gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_DISABLE);
		if(GPIO_INPUT_GET(0)==1)
			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1);
		else
			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);
		//clear interrupt status
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(0));
	}
}

static
void gpio_intr_handler()
{
    uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);

    if(gpio_status & (BIT(0)))
    {
		if(zt == 1)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);
			zt = 0;
		}
		else
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1);
			zt = 1;
		}
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
	wifi_set_opmode(STATION_MODE);
        struct station_config stConf;
        stConf.bssid_set = 0; //need not check MAC address of AP
        os_memcpy(&stConf.ssid, SSID, 32);
        os_memcpy(&stConf.password, PASSWORD, 64);
        wifi_station_set_config(&stConf);

	gpio_init();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(0));
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
	ETS_GPIO_INTR_DISABLE();
	ETS_GPIO_INTR_ATTACH(&gpio_intr_handler, NULL);
	gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_LOLEVEL);
	ETS_GPIO_INTR_ENABLE();

	// led gpio2 init;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);
	return ;
}
